#!/usr/bin/env python3
"""Разбор kallsyms-подобных строк из лога/bugreport -> якоря (symbol -> addr).

Поддерживает форматы:
  [<ffffff8009xxxxxx>] symbol+0x12/0x340
  symbol+0x12/0x340                       (без адреса)
  addr:ffffff8009xxxxxx + 148, symbol     (CMA-таблицы Sber)
  ffffff8009xxxxxx symbol
Запуск:
  parse_backtrace.py < log.txt
  parse_backtrace.py bugreport.txt [--map build/out/System.map]
"""
import re, sys

R_BRACK = re.compile(r"\[<(ffffff[0-9a-f]+)>\]\s+([A-Za-z_][\w.@$\-]*)\+(0x[0-9a-f]+)")
R_ADDR_SYM = re.compile(r"(ffffff[0-9a-f]{6,})\s+([A-Za-z_][\w.@$\-]*)\+(0x[0-9a-f]+)")
R_CMA = re.compile(r"addr:(ffffff[0-9a-f]+)\s*\+\s*([0-9a-f]+),\s*(\S+)")
R_SYM_ONLY = re.compile(r"\[\S*\]\s+([A-Za-z_][\w.@$\-]*)\+(0x[0-9a-f]+)/0x[0-9a-f]+")

def main():
    path = None
    mapf = None
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--map":
            mapf = args[i+1]; i += 2; continue
        if not a.startswith("-"):
            path = a
        i += 1
    text = open(path, errors="replace").read() if path else sys.stdin.read()

    anchors = {}          # name -> address
    noaddr = {}           # name -> off (адрес неизвестен)
    for m in R_CMA.finditer(text):
        anchors[m.group(3)] = int(m.group(1), 16) - int(m.group(2), 16)
    for m in R_BRACK.finditer(text):
        anchors.setdefault(m.group(2), int(m.group(1), 16) - int(m.group(3), 16))
    for m in R_ADDR_SYM.finditer(text):
        anchors.setdefault(m.group(2), int(m.group(1), 16) - int(m.group(3), 16))
    for m in R_SYM_ONLY.finditer(text):
        noaddr.setdefault(m.group(1), m.group(2))

    print("# якорей с адресом:", len(anchors))
    for n, a in sorted(anchors.items(), key=lambda x: x[1]):
        print(f"  0x{a:016x}  {n}")
    if noaddr:
        print("# символы без адреса (только имя+смещение):")
        for n, o in sorted(noaddr.items()):
            print(f"  {n}+{o}")

    # если задан System.map — посчитать дельты и подсказать оффсеты
    if mapf:
        syms = {}
        for l in open(mapf):
            p = l.split()
            if len(p) >= 3:
                try: syms[p[2]] = int(p[0], 16)
                except: pass
        base = syms.get("_head") or syms.get("_text")
        print("\n# дельты (vendor - ours) по найденным якорям:")
        for n, a in sorted(anchors.items(), key=lambda x: x[1]):
            o = syms.get(n)
            if o: print(f"  {n:40} delta={a-o}")

if __name__ == "__main__":
    main()
