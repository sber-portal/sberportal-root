# Ядро SberPortal: артефакты, конфиг, пересборка

Здесь всё, что связано с ядром Linux 4.9.268 (`g12b`), кроме самого эксплойта:
инвентарь артефактов из OTA, конфиг с устройства, скрипты пересборки и инструменты
для оффсетов/структур.

**Короткий итог:** точных оффсетов пересборка **не дала** (47 % modversions,
`module_layout` не совпал), поэтому эксплойт адреса не использует — работает
kernel-shellcode'ом. Собранный `vmlinux` при этом полезен: DWARF дал раскладку
`task_struct`/`cred`. Подробная история — `research-notes` → `docs/11-kernel.md`.

## Файлы

| файл | что |
|---|---|
| `kernel_config.txt` | **настоящий** конфиг работающего ядра, 4823 строки, снят из `/proc/config.gz` |
| `KERNEL_ARTIFACTS.md` | что есть в OTA (модули/`@AML`/пусто по vmlinux), версии, разделы |
| `BUILD_REPORT.md` | отчёт о сборке: база, тулчейн, отклонения, метрика modversions, оффсеты |
| `build_kernel.sh` | вариант **A**: `LineageOS/android_kernel_amlogic_linux-4.9 @ lineage-19.1` (4.9.269) + наш `.config` |
| `build_kernel_b.sh` | вариант **B** (FAILED): `khadas/linux @ khadas-vims-pie` + stable 114→268 |
| `build_kernel_b2.sh` | вариант **B2** (FAILED): `khadas/linux @ khadas-vims-4.9.y` + stable 242→268 |
| `continue_build.sh` | добор сборки A после правок конфига (`scripts/config` + `make -k`) |
| `provision.sh` | подготовка Lima-гостя: зависимости, `dpkg --add-architecture amd64`, тулчейн в `/opt/tc` |
| `sber.yaml` | конфиг Lima: `vz`, aarch64, Rosetta, 8 CPU / 12 GiB |
| `compare_modversions.sh` | сверка CRC modversions: наш `Module.symvers` vs `__versions` вендорских `.ko` |
| `offsets.py` | `sym − _head` (+ low12) из `System.map`/kallsyms-дампа |
| `gen_poc_config.py` | из `System.map`/kallsyms → готовый `SBERPORTAL`-блок `#define` + ADD-иммедиаты + env-строка для PoC |
| `compare_offsets.sh` | сравнить оффсеты сборок A и B |
| `correct_offsets.py` | piecewise-линейная коррекция по 21 якорю из bugreport (CMA-таблица `addr:… + off, symbol`) |
| `correct_offsets_v2.py` | коррекция по границам секций + 21 text-анкору (наша сборка крупнее вендорской) |
| `data_calibrate.py` | по факту записи в `.data` (изменилось поле `init_uts_ns`) считает реальный сдвиг секции и пересчитывает data-цели |
| `parse_backtrace.py` | вытащить якоря `symbol/addr` из логов и bugreport, сверить с `System.map` |

## Порядок работы

```bash
# 0. окружение (на macOS)
limactl start --name=sber ./sber.yaml
limactl shell sber sudo bash provision.sh     # тулчейн ожидается в ~/sberportal/build/tc
# рабочая копия архива (OTA, kernel_config.txt, распакованные разделы):
export SBER=~/sberportal

# 1. сборка (внутри Lima)
bash build_kernel.sh            # → $SBER/build/out/{vmlinux,System.map,Module.symvers}

# 2. годится ли сборка для оффсетов
bash compare_modversions.sh     # критерий: module_layout должен совпасть

# 3. если да — точные define для PoC
python3 gen_poc_config.py $SBER/build/out/System.map

# 4. диагностика/коррекция, если сборка «похожа, но не та»
python3 offsets.py $SBER/build/out/System.map
python3 correct_offsets_v2.py $SBER/build/out/System.map
bash compare_offsets.sh
python3 data_calibrate.py <T_hex> <field>     # после калибровочной записи на устройстве
```

`build_kernel*.sh` по умолчанию берут `SBER=~/sberportal` и `SRC=~/linux`; переопределяются
переменными окружения.

## Грабли (проверено)

- **`-O2` для эксплойта нельзя** — сборка `raven_sber` идёт без него (см. корневой `build.sh`).
- Вариант A без правок не собирается: конфликт mainline-драйверов Meson с вендорскими
  `drivers/amlogic/*`; нужны dummy-заглушки и включение `AMLOGIC_VOUT_CLK_SERVE`,
  `AMLOGIC_VIDEO_COMPOSER`, `AMLOGIC_UVM_CORE`, `AMLOGIC_V4L_VIDEO3`.
- `khadas-vims-pie` (B) и `khadas-vims-4.9.y` (B2) — **тупик**: stable-патчи не ложатся
  (тысячи `.rej`), `SUBLEVEL` не двигается, дерево не соответствует вендорскому.
- Точный признак «сборка не та» — не количество совпавших символов, а **`module_layout`**.
- Версия ядра у вендора 4.9.268, у A получилась 4.9.269 (один stable-патч).
- `vmlinux` (231 МБ), `System.map` (4.7 МБ), `Module.symvers`, NDK r21d и распакованные
  разделы в git не входят — воспроизводятся скриптами.

## Где точные адреса на самом деле

Их даёт не `System.map`, а само устройство: `compat_sys_call_table[189]`-хиджек +
kernel-shellcode, `current` из `sp_el0`, а `selinux_enforcing` (`0xffffff800a2f21dc`)
найден сканом `.bss`. См. `EXPLOIT.md` и `src/shellcode.S`.
