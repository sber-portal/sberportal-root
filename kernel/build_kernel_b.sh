#!/usr/bin/env bash
SBER=${SBER:-$HOME/sberportal}   # рабочая копия архива (OTA, разделы, kernel_config.txt)
# Вариант B: era-совместимое дерево (khadas-vims-pie, Android9/4.9.113) + stable до 4.9.268.
set -uxo pipefail
export PATH=/opt/tc/bin:$PATH
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

SRC=$HOME/linux_b
OUT=${SBER}/build/out_b
CFG=${SBER}/kernel_config.txt
LOG=$OUT/build.log

mkdir -p "$OUT"
rm -rf "$SRC"
git clone --depth 1 -b khadas-vims-pie https://github.com/khadas/linux "$SRC"
cd "$SRC"
echo "=== base version ===" | tee "$LOG"
grep -E '^(VERSION|PATCHLEVEL|SUBLEVEL)' Makefile | tee -a "$LOG"

fails=""
for n in $(seq 114 268); do
  curl -fsSL "https://cdn.kernel.org/pub/linux/kernel/v4.x/patch-4.9.$n.xz" -o /tmp/p.xz || { fails="$fails dl$n"; continue; }
  xz -dc /tmp/p.xz > /tmp/p.patch
  patch -p1 -N --fuzz=3 --silent < /tmp/p.patch >/dev/null 2>&1 || fails="$fails $n"
done
echo "=== stable patch fails:${fails:- none} ===" | tee -a "$LOG"
grep -E '^(VERSION|PATCHLEVEL|SUBLEVEL)' Makefile | tee -a "$LOG"

cp "$CFG" .config
make olddefconfig >>"$LOG" 2>&1
echo "=== config diff (our vs accepted) ===" | tee -a "$LOG"
diff <(sort "$CFG") <(sort .config) | grep -cE '^[<>]' | tee -a "$LOG" || true

make -j"$(nproc)" KCFLAGS="-Wno-error" vmlinux >>"$LOG" 2>&1
cp -v vmlinux System.map "$OUT"/ 2>&1 | tee -a "$LOG" || echo "NO vmlinux" | tee -a "$LOG"
cp -v Module.symvers "$OUT"/ 2>&1 | tee -a "$LOG" || true
echo DONE | tee -a "$LOG"
