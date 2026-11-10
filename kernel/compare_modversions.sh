#!/usr/bin/env bash
SBER=${SBER:-$HOME/sberportal}   # рабочая копия архива (OTA, разделы, kernel_config.txt)
# Сверка modversions CRC: наши Module.symvers vs вендорские .ko (секция __versions).
set -euo pipefail
export PATH=/opt/tc/bin:$PATH
SYMV=${SBER}/build/out/Module.symvers
MODS=${SBER}/extracted_20261008_095607/parts/out/vendor_modules/modules

if [ ! -f "$SYMV" ]; then echo "нет $SYMV (сначала сборка)"; exit 1; fi

# наши CRC: 0x<crc>\t<name>\t<mod>\t<export>
declare -A OURS
while read -r crc name rest; do OURS["$name"]="$crc"; done < "$SYMV"

tmp=$(mktemp -d)
tot=0; match=0; miss=0
for ko in "$MODS"/*.ko; do
  n=$(basename "$ko" .ko)
  if ! aarch64-linux-gnu-objcopy --dump-section __versions="$tmp/v.bin" "$ko" 2>/dev/null; then
    echo "[skip] $n: нет __versions"; continue
  fi
  # struct modversion_info { u64 crc; char name[56]; } размера 64
  python3 - "$tmp/v.bin" <<'PY'
import struct,sys
d=open(sys.argv[1],'rb').read()
for i in range(0,len(d)-63,64):
    crc,=struct.unpack_from('<Q',d,i)
    name=d[i+8:i+8+56].split(b'\x00',1)[0].decode('latin1')
    if name: print(f"{crc:#x}\t{name}")
PY
done > "$tmp/vendor.txt"

echo "=== сверка (только символы, экспортируемые ядром) ==="
python3 - "$SYMV" "$tmp/vendor.txt" <<'PY'
import sys
ours={}
for line in open(sys.argv[1]):
    p=line.split()
    if len(p)>=2: ours[p[1]]=p[0]
seen={}
for line in open(sys.argv[2]):
    c,n=line.rstrip("\n").split("\t",1)
    seen.setdefault(n,set()).add(c)
tot=match=miss=0
bad=[]
for n,cs in sorted(seen.items()):
    if n in ours:
        tot+=1
        if ours[n] in cs: match+=1
        else: bad.append((n,ours[n],sorted(cs)))
    else:
        miss+=1
print(f"сверено экспортируемых: {tot}  совпало: {match}  не совпало: {len(bad)}")
print(f"в модулях, но не экспортируется ядром (пропущено): {miss}")
for n,o,v in bad[:25]:
    print(f"  MISMATCH {n}: ours={o} vendor={v}")
PY
