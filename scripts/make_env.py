#!/usr/bin/env python3
"""Правка u-boot environment SberPortal: добавляет persistent-root в preboot.

Использование:
    make_env.py <env_orig.bin> <env_new.bin>

На вход — 64 КБ блок из /dev/block/env (формат: crc32 LE | data).
Все переменные, включая серийники, остаются своими: скрипт меняет только preboot.
"""
import struct
import sys
import zlib

SIZE = 0x10000
DATA = SIZE - 4

PREBOOT = (
    "setenv EnableSelinux permissive;setenv rabbith_has_adb_root yes;"
    "run bcb_cmd; run init_display;run factory_reset_key;run storeargs;"
    "forceupdate;bcb uboot-command;run check_sdts_mode;run switch_bootmode;"
    "setenv bootargs ${bootargs} androidboot.rabbith.adb_root=1;"
)


def main(src: str, dst: str) -> int:
    blob = open(src, "rb").read()
    if len(blob) != SIZE:
        print(f"ожидался блок {SIZE} байт, получено {len(blob)}", file=sys.stderr)
        return 1

    data = bytearray(blob[4:])
    marker = b"preboot="
    pos = data.find(marker)
    if pos < 0:
        print("в env нет переменной preboot", file=sys.stderr)
        return 1
    end = data.find(b"\x00", pos)
    if end < 0:
        print("не найден конец preboot", file=sys.stderr)
        return 1

    # Собираем записи key=value до первой пустой/некорректной записи:
    # хвост блока может содержать мусор от прошлых записей.
    entries = []
    for chunk in bytes(data).split(b"\x00"):
        if not chunk or b"=" not in chunk:
            break
        entries.append(chunk)

    replaced = False
    for i, e in enumerate(entries):
        if e.startswith(marker):
            entries[i] = marker + PREBOOT.encode()
            replaced = True
            break
    if not replaced:
        entries.append(marker + PREBOOT.encode())

    new_data = b"\x00".join(entries) + b"\x00"
    if len(new_data) > DATA:
        print(f"env не помещается: {len(new_data)} > {DATA} байт", file=sys.stderr)
        return 1
    new_data += b"\x00" * (DATA - len(new_data))

    crc = zlib.crc32(new_data) & 0xFFFFFFFF
    out = struct.pack("<I", crc) + new_data
    open(dst, "wb").write(out)

    check = zlib.crc32(out[4:]) & 0xFFFFFFFF
    print(f"переменных: {len(entries)}, preboot: {PREBOOT.count(';') + 1} команд, "
          f"crc32={crc:#010x}{' ok' if check == crc else ' MISMATCH'}")
    return 0 if check == crc else 1


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        raise SystemExit(2)
    raise SystemExit(main(sys.argv[1], sys.argv[2]))
