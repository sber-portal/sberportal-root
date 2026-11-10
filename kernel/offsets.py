#!/usr/bin/env python3
"""Извлечь sym - _head (и _text) из System.map.
usage: offsets.py System.map [sym1 sym2 ... | symbols.txt]
"""
import sys

def load(path):
    syms = {}
    for line in open(path, errors="replace"):
        p = line.split()
        if len(p) >= 3:
            try:
                syms[p[2]] = int(p[0], 16)
            except ValueError:
                pass
    return syms

def first(syms, *names):
    for n in names:
        if n in syms:
            return n, syms[n]
    return None, None

def main():
    syms = load(sys.argv[1])
    base_name, base = first(syms, "_head", "_stext", "stext", "_text")
    print(f"# kernel size: {len(syms)} syms")
    print(f"# base: {base_name} = {hex(base) if base is not None else None}")
    for alt in ("_head", "_text", "_stext", "stext", "_etext", "__bss_start", "_end"):
        if alt in syms:
            off = syms[alt] - base if base is not None else 0
            print(f"#   {alt:16} {hex(syms[alt])}  (+{hex(off)})")
    want = sys.argv[2:]
    if len(want) == 1 and want[0].endswith(".txt"):
        want = [l.strip() for l in open(want[0]) if l.strip()]
    if not want:
        want = ["commit_creds", "init_cred", "selinux_enforcing", "sel_read_handle_unknown"]
    print("# symbol\tvaddr\toff_from_base\tlow12")
    for w in want:
        if w in syms:
            a = syms[w]
            print(f"{w}\t0x{a:x}\t0x{a-base:x}\t0x{a & 0xfff:03x}")
        else:
            print(f"{w}\tNOT_FOUND")

if __name__ == "__main__":
    main()
