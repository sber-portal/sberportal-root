#!/bin/sh
# Установить на устройство APK, который сам поднимает adbd в root при загрузке.
# Нужен один раз (и после каждого обновления APK): install + первый запуск
# (первый запуск снимает «stopped»-состояние, иначе BOOT_COMPLETED не придёт).
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
IP=${IP:?укажи IP портала: IP=192.168.1.x}
adb connect $IP:5555 >/dev/null 2>&1 || true
adb install -r "$HERE/prebuilt/adbroot.apk"
adb shell 'am start -n com.user.adbroot/.MainActivity'
sleep 4
adb shell 'id; getenforce; getprop service.adb.root'
