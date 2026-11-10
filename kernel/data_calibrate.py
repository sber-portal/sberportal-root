#!/usr/bin/env python3
# По результату скана (абсолютный адрес T, в котором изменилось поле uts) вычисляет
# реальный сдвиг .data и пересчитывает оффсеты data-таргетов.
#
# Использование:  python3 build/data_calibrate.py <T_hex> <field>
#   field in {sysname,nodename,release,version,machine,domainname}
#
# Адреса T — вендорские (то, куда писали). our_off = символ в НАШЕЙ сборке.
import sys

OUR = {
    'init_uts_ns':    0x0112deb8,
    'init_cred':      0x01143478,
    'modprobe_path':  0x01141e58,
    'core_pattern':   0x01179930,
    'uevent_helper':  0x01196000,
}
FIELD = {'sysname':0x04,'nodename':0x45,'release':0x86,'version':0xc7,'machine':0x108,'domainname':0x149}
DATA_START_SHIFT = 0xe0000     # точный сдвиг .data start
VDATA_OFF        = 0x5000      # vdso_data_store = .data+0x5000, там сдвиг тоже 0xe0000

def main():
    if len(sys.argv) < 3:
        print(__doc__); sys.exit(1)
    T = int(sys.argv[1], 16)
    field = sys.argv[2]
    if field not in FIELD:
        print("field must be one of", list(FIELD)); sys.exit(1)
    V = T - FIELD[field]                      # истинная база init_uts_ns (вендор)
    off_uts = OUR['init_uts_ns']
    S1 = off_uts - V                          # сдвиг на позиции init_uts_ns
    pos = off_uts                               # позиция в .data (наша)
    print(f"T(попадание)      = 0x{T:x}  поле {field}")
    print(f"init_uts_ns vendor= 0x{V:x}")
    print(f"сдвиг .data в т. init_uts_ns = 0x{S1:x}  (при .data start 0x{DATA_START_SHIFT:x})")
    # локальный наклон накопления по двум точкам: .data+VDATA_OFF(=0xe0000) и init_uts_ns
    slope = (S1 - DATA_START_SHIFT) / (pos - VDATA_OFF)
    print(f"накопление на байт .data = {slope:.6f}  (0 = нет роста)")
    print()
    print(f"{'symbol':16s} {'our_off':>10s} {'est.shift':>10s} {'est.vendor':>11s}")
    for k, o in OUR.items():
        if k == 'init_uts_ns':
            sh = S1
        else:
            sh = int(DATA_START_SHIFT + slope * (o - VDATA_OFF))
        print(f"{k:16s} 0x{o:08x} 0x{sh:08x} 0x{o-sh:08x}")

main()
