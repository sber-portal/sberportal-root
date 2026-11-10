# Rebuild ядра SberPortal (g12b, 4.9.268) — результат

## Что собрано
- **База:** `LineageOS/android_kernel_amlogic_linux-4.9 @ lineage-19.1` (Amlogic vendor, 4.9.269)
- **Тулчейн:** Linaro GCC 6.3.1 20170109 (6.3-2017.02) — **ровно как у вендора** (`/proc/version`)
- **Конфиг:** ваш `kernel_config.txt` (`/proc/config.gz`)
- **Сборка:** в Lima VM (Ubuntu 24.04 arm64) + Rosetta для x86_64-тулчейна

Артефакты (`build/out/`):
- `vmlinux` (~231 MB, с DWARF — годно для реверса/структур)
- `System.map` (106 770 символов)
- `Module.symvers`

## Оффсеты (`sym - _head`), base `_head = 0xffffff8009080000`
| символ | off_from_head | low12 |
|---|---|---|
| `commit_creds` | `0x6fd58` | `0xd58` |
| `init_cred` | `0x1143478` | `0x478` |
| `prepare_kernel_cred` | `0x70468` | `0x468` |
| `selinux_enforcing` | `0x1365c9c` | `0xc9c` |
| `sel_read_handle_unknown` | `0x3042d0` | `0x2d0` |

Макросы (по вашей формуле):
```
ADD_INIT_SG   = 0x9111e000
ADD_COMMIT_SG = 0x91356108
```

## ВАЖНО: точность (fidelity)
- Сверка modversions с вендорскими `.ko`: **совпало 392 / 833 (47 %)**, не совпало 441.
- Ключевой индикатор — `module_layout`: vendor `0xe04fb9ea` vs наш `0x35e23c3f` → **ABI не совпадает**.

Вывод: собранное ядро **не байт-в-байт и не ABI-совместимо** с вендорским — дерево
(LineageOS-2021 + наши фиксы) отличается от дерева Sber (Amlogic-2019 + приватные патчи).
Поэтому **оффсеты выше — оценочные, не гарантированные**.

## Что пришлось изменить, чтобы собралось (отклонения от вендора)
- Отключены mainline-драйверы Meson (конфликт с вендорскими `drivers/amlogic/*`):
  `drivers/pinctrl/meson`, `drivers/clk/meson`, `drivers/soc/amlogic`, `drivers/gpu/drm/meson` (заменены dummy).
- Включены `AMLOGIC_VOUT_CLK_SERVE`, `AMLOGIC_VIDEO_COMPOSER`, `AMLOGIC_UVM_CORE`, `AMLOGIC_V4L_VIDEO3`
  (в дереве они гейтят символы, которые вызываются в других драйверах).
- Мелкие фиксы: include `linux/amlogic/pm.h` в `bt_device.c`; гейт notifier в `hdmi_tx_main.c`;
  стабы `pci_*` в `wifi_dt.c` (при `CONFIG_PCI=n`); `KCFLAGS=-Wno-error`.
- Версия ядра получилась **4.9.269** (ровно 268 у вендора; один stable-патч разницы).

## Как получить точные оффсеты
1. `kallsyms` на устройстве — **заблокирован SELinux** (`shell` → `proc_kallsyms` deny).
2. Реальный вендорский `vmlinux` — зашифрован (`@AML`), недоступен.
3. Точный `sym - _head` → только из **одной рантайм-утечки** (её даёт сам эксплойт) **или** из
   исходников Sber (запросить по GPL: `devrel_sberdevices@sberbank.ru`).

Собранный `vmlinux`/`System.map`/DWARF при этом полезны: структуры (`pahole`), дизасм,
контекст функций, оценка порядка величин оффсетов.
