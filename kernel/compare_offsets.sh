#!/usr/bin/env bash
SBER=${SBER:-$HOME/sberportal}   # рабочая копия архива (OTA, разделы, kernel_config.txt)
# Сравнить оффсеты (sym - _head) между сборками A (out) и B (out_b).
HERE=$(cd "$(dirname "$0")" && pwd)
B=${SBER}/build
for d in out out_b; do
  echo "==== $d ===="
  if [ -f "$B/$d/System.map" ]; then
    python3 "$HERE/offsets.py" "$B/$d/System.map" commit_creds init_cred prepare_kernel_cred selinux_enforcing sel_read_handle_unknown
  else
    echo "нет $B/$d/System.map"
  fi
done
