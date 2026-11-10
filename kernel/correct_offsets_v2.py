#!/usr/bin/env python3
# Секционная коррекция оффсетов: наша сборка крупнее вендорской, поэтому
# .rodata/.data/.bss сдвинуты ВВЕРХ на накопленную разницу размеров секций.
# Сдвиги границ секций ТОЧНЫЕ (адреса вендора из bugreport + наши из System.map).
import sys, re

BASE = 0xffffff8009080000

# наши (из System.map): off = sym-BASE
OURS = dict(
    text=[0x0, 0xb70000],          # _text .. _etext
    rodata=[0xb70000, 0x1020000],  # __start_rodata .. __init_begin
    init=[0x1020000, 0x1110000],   # __init_begin .. __init_end
    data=[0x1110000, 0x129f808],   # _sdata .. __bss_start
    bss=[0x129f808, 0x13f2e24],    # __bss_start .. __bss_stop
)
# вендорские (из bugreport "Virtual kernel memory layout"), off
VEND = dict(
    text=[0x0, 0xac0000],
    rodata=[0xac0000, 0xf40000],
    init=[0xf40000, 0x1030000],
    data=[0x1030000, 0x122b008],
    bss=[0x122b008, 0x12fc994],
)
# сдвиг на ГРАНИЦАХ секций = вендорская граница - наша граница (одинаковый объём до неё)
SHIFT_AT_BOUNDARY = {  # off -> shift
    0x0: 0x0,
    0xb70000: 0xb0000,      # .rodata start (== .text end)
    0x1020000: 0xe0000,     # .init start == роdatа end
    0x1110000: 0xe0000,     # .data start
    0x129f808: 0xf4800,     # .bss start
    0x13f2e24: 0xf6490,     # .bss end
}

# .text: реальные сдвиги из 21 мм-анкора (off -> ours-vendor)
TEXT_ANCHORS = [
    (0x1d8e8, 680),
    (0x1646c8, 12872),
    (0x1654a0, 9512), (0x1654f8, 9472), (0x165528, 9528), (0x165590, 9792),
    (0x263088, 10072),
    (0x2631f8, 9976), (0x29dff0, 11736), (0x29e7c0, 11216), (0x29e870, 11216),
    (0x31ca18, 12160), (0x32cc30, 12120), (0x32cfc0, 11384), (0x32d030, 11848),
    (0x32d280, 11872), (0x32d5a0, 12008), (0x32d870, 12024), (0x32eb48, 12856),
    (0x461970, 14104),
]
TEXT_ANCHORS.sort()

def sec_of(off):
    for s,(a,b) in OURS.items():
        if a <= off < b: return s
    return 'bss' if off >= OURS['bss'][0] else '?'

def shift_linear(off, a, b):
    # интерполируем сдвиг между границами секции (SHIFT_AT_BOUNDARY[a], SHIFT_AT_BOUNDARY[b])
    sa, sb = SHIFT_AT_BOUNDARY[a], SHIFT_AT_BOUNDARY[b]
    if b == a: return sa
    return sa + (off - a) * (sb - sa) // (b - a)

def shift(off):
    s = sec_of(off)
    if s == 'text':
        # piecewise по анкорам, экстраполяция плоская
        if off <= TEXT_ANCHORS[0][0]: return TEXT_ANCHORS[0][1]
        if off >= TEXT_ANCHORS[-1][0]: return TEXT_ANCHORS[-1][1]
        for i in range(1, len(TEXT_ANCHORS)):
            x0,y0 = TEXT_ANCHORS[i-1]; x1,y1 = TEXT_ANCHORS[i]
            if off <= x1:
                return int(y0 + (off-x0)*(y1-y0)/(x1-x0))
    a,b = OURS[s]
    return shift_linear(off, a, b)

def main():
    base = BASE
    syms = {}
    for line in open(sys.argv[1] if len(sys.argv)>1 else 'build/out/System.map'):
        p = line.split()
        if len(p)==3: syms.setdefault(p[2], int(p[0],16))
    targets = sys.argv[2:] or ['init_uts_ns','linux_banner','core_pattern','modprobe_path',
                               'selinux_enforcing','sel_read_handle_unknown','init_cred','commit_creds']
    print(f"{'symbol':30s} {'our_off':>10s} {'sec':>7s} {'shift':>9s} {'corrected':>10s}")
    for t in targets:
        if t not in syms:
            print(f"{t:30s} (нет в System.map)"); continue
        off = syms[t]-base
        sh = shift(off)
        print(f"{t:30s} 0x{off:08x} {sec_of(off):>7s} 0x{sh:07x} 0x{off-sh:08x}")

main()
