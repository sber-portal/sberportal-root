#!/usr/bin/env bash
SBER=${SBER:-$HOME/sberportal}   # рабочая копия архива (OTA, разделы, kernel_config.txt)
set -uxo pipefail
export PATH=/opt/tc/bin:$PATH
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
cd "$HOME/linux"

scripts/config \
  --enable AMLOGIC_VOUT_CLK_SERVE \
  --enable AMLOGIC_VIDEO_COMPOSER \
  --enable AMLOGIC_UVM_CORE \
  --enable AMLOGIC_V4L_VIDEO3

make -k -j"$(nproc)" KCFLAGS="-Wno-error" vmlinux 2>&1 | tee ${SBER}/build/out/build.log

mkdir -p ${SBER}/build/out
cp -v vmlinux System.map ${SBER}/build/out/ 2>/dev/null || echo "NO vmlinux yet"
cp -v Module.symvers ${SBER}/build/out/ 2>/dev/null || true
echo DONE
