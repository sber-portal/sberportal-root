#!/usr/bin/env bash
SBER=${SBER:-$HOME/sberportal}   # рабочая копия архива (OTA, разделы, kernel_config.txt)
# Сборка ядра внутри Lima.
# База: LineageOS/android_kernel_amlogic_linux-4.9 @ lineage-19.1 (Amlogic vendor, 4.9.269).
# + наш .config -> vmlinux / System.map / Module.symvers.
set -euxo pipefail
export PATH=/opt/tc/bin:$PATH
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

SRC=$HOME/linux
OUT=${SBER}/build/out
CFG=${SBER}/kernel_config.txt

rm -rf "$SRC"
git clone --depth 1 -b lineage-19.1 \
  https://github.com/LineageOS/android_kernel_amlogic_linux-4.9 "$SRC"
cd "$SRC"
echo "=== версия базы ==="; grep -E '^(VERSION|PATCHLEVEL|SUBLEVEL)' Makefile

cp "$CFG" .config
make olddefconfig

echo "=== сколько строк конфига разошлось (наш vs принятый) ==="
diff <(sort "$CFG") <(sort .config) | grep -cE '^[<>]' || true
echo "=== версия после конфига ==="; grep -E '^(VERSION|PATCHLEVEL|SUBLEVEL)' Makefile; cat include/config/kernel.release 2>/dev/null || true

# без -g<hash> в release
echo "" > localversion

make -j"$(nproc)" vmlinux

mkdir -p "$OUT"
cp -v vmlinux System.map "$OUT"/ 2>/dev/null || true
cp -v Module.symvers "$OUT"/ 2>/dev/null || echo "no Module.symvers"
echo DONE
