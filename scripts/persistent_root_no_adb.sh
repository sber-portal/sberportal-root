#!/usr/bin/env bash
# Постоянный root БЕЗ уже работающего `adb root` (самый первый раз).
#
# Обычный scripts/persistent_root.sh требует `adb root` -> uid=0, чтобы снять
# и записать /dev/block/env. На новом устройстве root ещё не включён, и adbd
# работает как shell. Здесь env правится в два прохода эксплойтом:
#
#   1) эксплойт даёт uid=0 в домене shell + SELinux permissive; через SP_CMD
#      снимаем текущий блок /dev/block/env на хост;
#   2) make_env.py меняет только preboot и пересчитывает CRC32;
#   3) второй проход эксплойта записывает блок обратно и проверяет побайтово;
#   4) перезагрузка, затем `adb root` -> uid=0, SELinux Permissive.
#
# usage: IP=<ip портала> ./scripts/persistent_root_no_adb.sh
# Env: HOG=<MB> (по умолчанию 900), WORK=<каталог для env_*.bin>
set -eu

IP=${IP:?укажи IP портала: IP=192.168.1.x}
D="$IP:5555"
HOG=${HOG:-900}
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
WORK=${WORK:-$ROOT/.env-work}
FUNC=0x19bbf0              # vmalloc: редкий вызов, безопасный «холст»
FABS=0xffffff800921bbf0

mkdir -p "$WORK"

say() { printf '%s\n' "$*"; }
adb connect "$D" >/dev/null 2>&1 || true
if [ "$(adb -s "$D" get-state 2>/dev/null)" != "device" ]; then
  say "Устройство $D не в состоянии 'device'. Проверь adb connect."; exit 2
fi

if [ "$(adb -s "$D" shell id -u 2>/dev/null | tr -d '\r')" = "0" ]; then
  say "adb уже root — используй scripts/persistent_root.sh"
  exit 0
fi

# --- device-side helpers -----------------------------------------------------
DUMP=/data/local/tmp/ravn_env_dump.sh
WRITE=/data/local/tmp/ravn_env_write.sh
DLOG=/data/local/tmp/ravn_env.log

cat > "$WORK/ravn_env_dump.sh" <<'EOS'
#!/system/bin/sh
LOG=/data/local/tmp/ravn_env.log
exec >>"$LOG" 2>&1
id
dd if=/dev/block/env of=/data/local/tmp/env_orig.bin bs=4096 count=16
sync
sha256sum /data/local/tmp/env_orig.bin
chmod 644 /data/local/tmp/env_orig.bin "$LOG"
EOS

cat > "$WORK/ravn_env_write.sh" <<'EOS'
#!/system/bin/sh
LOG=/data/local/tmp/ravn_env.log
exec >>"$LOG" 2>&1
id
getenforce
dd if=/data/local/tmp/env_new.bin of=/dev/block/env bs=4096
sync
dd if=/dev/block/env of=/data/local/tmp/env_chk.bin bs=4096 count=16
sync
if cmp -s /data/local/tmp/env_new.bin /data/local/tmp/env_chk.bin; then
  echo ENV_OK
else
  echo ENV_MISMATCH
fi
chmod 644 /data/local/tmp/env_chk.bin "$LOG"
EOS

# --- one exploit pass with a root command -----------------------------------
# $1 = device-side command, $2 = attempt count
exploit_run() {
  local cmd="$1" n="${2:-30}" i
  for i in $(seq 1 "$n"); do
    adb connect "$D" >/dev/null 2>&1 || true
    if [ "$(adb -s "$D" get-state 2>/dev/null)" != "device" ]; then
      say "[$i] устройство offline — ждём загрузку"
      sleep 20
      continue
    fi
    adb -s "$D" shell "pidof hog >/dev/null || (nohup /data/local/tmp/hog $HOG >/dev/null 2>&1 &)" >/dev/null 2>&1 || true
    adb -s "$D" shell "rm -f $DLOG; cd /data/local/tmp && timeout 180 env RAVEN_VERIFY=13 SP_FUNC=$FUNC SP_FUNC_ABS=$FABS SP_CMD='$cmd' ./raven_sber > $DLOG 2>&1" >/dev/null 2>&1 || true
    if adb -s "$D" shell "grep -qa 'uid=0 euid=0' $DLOG" 2>/dev/null; then
      say ">>> root-прогон удался (попытка $i)"
      return 0
    fi
    say "[$i] не повезло, пробую ещё"
  done
  return 1
}

say ">> раскладываю эксплойт и помощники"
adb -s "$D" push "$ROOT/prebuilt/raven_sber" /data/local/tmp/raven_sber >/dev/null
adb -s "$D" push "$ROOT/prebuilt/hog"        /data/local/tmp/hog        >/dev/null
adb -s "$D" push "$WORK/ravn_env_dump.sh"  "$DUMP"  >/dev/null
adb -s "$D" push "$WORK/ravn_env_write.sh" "$WRITE" >/dev/null
adb -s "$D" shell "chmod 755 /data/local/tmp/raven_sber /data/local/tmp/hog $DUMP $WRITE" >/dev/null
adb -s "$D" shell "nohup /data/local/tmp/hog $HOG >/dev/null 2>&1 &" >/dev/null 2>&1 || true

say ">> проход 1: снимаю /dev/block/env"
exploit_run "sh $DUMP" 30 || { say "root не получен — повтори запуск"; exit 1; }
adb -s "$D" pull /data/local/tmp/env_orig.bin "$WORK/env_orig.bin" >/dev/null
say "   бэкап: $WORK/env_orig.bin"

say ">> правлю preboot (make_env.py)"
python3 "$HERE/make_env.py" "$WORK/env_orig.bin" "$WORK/env_new.bin"

say ">> проход 2: записываю env"
adb -s "$D" push "$WORK/env_new.bin" /data/local/tmp/env_new.bin >/dev/null
adb -s "$D" shell "chmod 644 /data/local/tmp/env_new.bin" >/dev/null
exploit_run "sh $WRITE" 30 || { say "root не получен — повтори запуск"; exit 1; }
adb -s "$D" pull "$DLOG" "$WORK/env_write.log" >/dev/null 2>&1 || true
if ! grep -qa ENV_OK "$WORK/env_write.log" 2>/dev/null; then
  say "!! запись env не подтверждена ($WORK/env_write.log). Не перезагружайся зря:"
  say "   повтори 'sh $WRITE' через эксплойт или восстанови env_orig.bin."
  exit 1
fi
say "   ENV_OK (побайтовое совпадение)"

say ">> перезагружаю"
adb -s "$D" reboot >/dev/null 2>&1 || true
sleep 45
for i in $(seq 1 30); do
  adb connect "$D" >/dev/null 2>&1 || true
  [ "$(adb -s "$D" get-state 2>/dev/null)" = "device" ] && break
  sleep 5
done
adb -s "$D" root >/dev/null 2>&1 || true
sleep 3
adb connect "$D" >/dev/null 2>&1 || true
say ">> проверка:"
adb -s "$D" shell 'id; getenforce; getprop ro.boot.rabbith.adb_root'
say
say "Откат: adb push $WORK/env_orig.bin /data/local/tmp/env_orig.bin && \\"
say "        adb shell 'dd if=/data/local/tmp/env_orig.bin of=/dev/block/env bs=4096; sync' && adb reboot"
