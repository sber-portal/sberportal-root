#!/bin/sh
# Постоянный root на SberPortal через u-boot env.
#
# Требует уже работающего root на устройстве (`adb root` → uid=0):
# либо разовый пролёт эксплойтом (scripts/getroot.sh), либо root уже включён.
#
# Что делает:
#  1. снимает текущий 64 КБ блок /dev/block/env с устройства и сохраняет его
#     локально (это резервная копия для отката);
#  2. добавляет в preboot флаги rabbith/SELinux (scripts/make_env.py);
#  3. пишет блок обратно, сверяет побайтово;
#  4. перезагружает устройство и проверяет результат.
#
# После этого: `adb root` → uid=0, SELinux Permissive. Переживает cold power-cycle.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
IP=${IP:?укажи IP портала: IP=192.168.1.x}
WORK=${WORK:-$HERE/.env-work}
A="adb -s $IP:5555"

$A connect "$IP:5555" >/dev/null 2>&1 || true
$A wait-for-device
$A root >/dev/null 2>&1 || true
sleep 3
$A connect "$IP:5555" >/dev/null 2>&1 || true

[ "$($A shell id -u | tr -d '\r')" = "0" ] || {
  echo "нет root: сначала выполните scripts/getroot.sh (или adb root)" >&2; exit 1; }

mkdir -p "$WORK"
echo ">> снимаю текущий env (бэкап: $WORK/env_orig.bin)"
$A shell 'dd if=/dev/block/env of=/data/local/tmp/env_orig.bin bs=4096 count=16' >/dev/null 2>&1
$A pull /data/local/tmp/env_orig.bin "$WORK/env_orig.bin" >/dev/null

python3 "$HERE/scripts/make_env.py" "$WORK/env_orig.bin" "$WORK/env_new.bin"

echo ">> пишу env"
$A push "$WORK/env_new.bin" /data/local/tmp/env_new.bin >/dev/null
$A shell 'dd if=/data/local/tmp/env_new.bin of=/dev/block/env bs=4096 && sync && \
           dd if=/dev/block/env of=/data/local/tmp/env_chk.bin bs=4096 count=16 && \
           cmp -s /data/local/tmp/env_new.bin /data/local/tmp/env_chk.bin && echo ENV_OK || echo ENV_MISMATCH'

$A reboot
echo ">> ждём загрузку"
sleep 45
$A connect "$IP:5555" >/dev/null 2>&1 || true
$A root >/dev/null 2>&1 || true
sleep 3
$A shell 'getprop ro.boot.rabbith.adb_root; getprop ro.boot.selinux; id; getenforce'
echo
echo "Откат: сохранить $WORK/env_orig.bin обратно в /dev/block/env и перезагрузить."
