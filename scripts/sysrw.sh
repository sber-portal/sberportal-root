#!/system/bin/sh
# Делает /system записываемым (systemless overlay, upper в /data/ravn_sys).
# Запускать как root (внутри VERIFY=13 через SP_CMD="sh /data/local/tmp/sysrw.sh").
# Живёт до перезагрузки; реальная (verity) system не меняется.
UP=/data/ravn_sys/up
WO=/data/ravn_sys/wo
mkdir -p "$UP" "$WO"
# снять возможные прошлые оверлеи над подкаталогами
umount -l /system/bin 2>/dev/null
mount -t overlay overlay -o lowerdir=/system,upperdir="$UP",workdir="$WO" /system 2>&1 && \
  echo "OK: /system writable (upper=$UP)  $(mount | grep ' /system ')" || \
  echo "FAIL: overlay not mounted (already mounted?)"
