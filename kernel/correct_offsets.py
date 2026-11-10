#!/usr/bin/env python3
"""Коррекция оффсетов по 21 якорю из bugreport (piecewise-linear delta = vendor - ours).
Печатает скорректированные адреса для 4 символов и оценку неопределённости.
"""
import re, glob, os, sys

# BUGREP — каталог с распакованным bugreport (файл bugreport-*.txt),
# MAP    — System.map нашей сборки. Оба можно задать аргументами или env.
BUG = os.environ.get("BUGREP", os.path.expanduser("~/sberportal/bugreport"))
MAP = os.environ.get("SYSMAP", os.path.expanduser("~/sberportal/build/out/System.map"))
if len(sys.argv) >= 3:
    BUG, MAP = sys.argv[1], sys.argv[2]
TARGETS = ["commit_creds", "init_cred", "selinux_enforcing", "sel_read_handle_unknown"]

# anchors
anch = []
for l in open(glob.glob(BUG + "/bugreport-*.txt")[0], errors="replace"):
    m = re.search(r"addr:(ffffff[0-9a-f]+)\s*\+\s*([0-9a-f]+),\s*(\S+)", l)
    if m:
        anch.append((int(m.group(1), 16) - int(m.group(2), 16), m.group(3)))  # (vendor_fn_addr, name)

syms = {}
for l in open(MAP):
    p = l.split()
    if len(p) >= 3:
        try: syms[p[2]] = int(p[0], 16)
        except: pass

pts = []  # (our_addr, delta)
for v, name in anch:
    o = syms.get(name)
    if o: pts.append((o, v - o))
pts.sort()
base = syms.get("_head") or syms.get("_text")
print("# anchors: our_addr delta")
for a, d in pts: print(f"  {hex(a)}  {d}")

def interp(x):
    if x <= pts[0][0]: return pts[0][1], 0
    if x >= pts[-1][0]: return pts[-1][1], 0
    for i in range(1, len(pts)):
        x0, d0 = pts[i-1]; x1, d1 = pts[i]
        if x <= x1:
            f = (x - x0) / (x1 - x0)
            d = d0 + f * (d1 - d0)
            # локальная крутизна => оценка неопределённости (половина изменения дельты на участке)
            return d, abs(d1 - d0)
    return pts[-1][1], 0

print()
print(f"{'symbol':26} {'our_off':>10} {'corr_off':>10} {'delta':>9} {'uncert~':>8}")
for t in TARGETS:
    o = syms.get(t)
    if not o: print(f"{t:26} MISSING"); continue
    d, u = interp(o)
    corr = int(o + d)
    print(f"{t:26} {hex(o-base):>10} {hex(corr-base):>10} {d:>9} {int(u):>8}")
