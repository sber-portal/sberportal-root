# SberPortal (stargate) — kernel artifacts from OTA

Источник: `release-1.102.34-b20851-stargate_payload.bin` (Android OTA `payload.bin`).
Партиции: `dt, boot, bootloader, bootloader0, bootloader1, logo, system, vendor, vbmeta, sdvtz`.

## Устройство (из /vendor/build.prop)
- brand/model: **SberDevices / SberPortal**, device `stargate`, board `g12b` (Amlogic A311D / S922)
- platform: Android 9 (`ro.product.first_api_level=28`)
- fingerprint: `SberDevices/stargate/stargate:9/PQ3B.190801.002/eng.jenkin.20260327.091927:user/release-keys`
- build date: **Fri Mar 27 09:19:07 UTC 2026**, security patch пусто
- `ro.board.platform=g12b`

## Ядро (boot)
- `boot` — обычный Android bootimg, но поле kernel = Amlogic **`@AML`** (secure-boot) образ.
- Тело зашифровано AES (энтропия ~7.99, магии gzip/lz4/xz/zstd/bz2 отсутствуют) → **в открытом виде ядра нет**.
- `dt` и `bootloader*` — тоже `@AML`/энтропия ~8.0 (зашифрованы).

## Что где лежит
| партиция | тип | содержимое |
|---|---|---|
| `system` | ext4 (`/system`) | 829 ELF, Android framework, приложения |
| `vendor` | ext4 (`/vendor`) | 359 ELF, HAL'ы Amlogic, **39 `.ko`** в `/vendor/lib/modules` |
| `sdvtz` | squashfs (xz, 994 inode) | аварийное окружение **StarOS** (busybox, Qt, qemu-system-aarch64, `vendor/staros/*`), **9 `.ko`** для 4.9.232 |

## Версия ядра
```
vendor/lib/modules/*.ko  ->  vermagic=4.9.268 SMP preempt mod_unload modversions aarch64
sdvtz/lib/modules/4.9.232 ->  4.9.232
```

## Артефактов vmlinux НЕТ
Проверено по всем извлечённым образам (`system`, `vendor`, `sdvtz`):
- `vmlinux` — 0
- `System.map` — 0
- `Image.gz` / `vmlinuz` / `zImage` / `ARM\x64` (raw kernel) — 0
- `.config` (`IKCFG`) / `CONFIG_LOCALVERSION` — 0
- gzip-потоки, распаковывающиеся в ядро (с «Linux version») — 0
  (все найденные gzip — это webui-ресурсы Chromium, tflite/adblock и т.п.)

## Извлечённые артефакты
- `out/vendor_modules/modules/` — 39 `.ko` (ядро 4.9.268), ~33 MB
  (amvdec_* (Amlogic video), vpu.ko, encoder.ko, galcore.ko, mali_kbase.ko,
   cfg80211/compat-backports/brcmfmac/dhd (Wi-Fi), optee*.ko, iv009_isp*.ko, …)
- `out/sdvtz/` — распакованный squashfs StarOS (155 MB)

## Вывод
vmlinux из этой прошивки получить нельзя — ядро лежит только в `boot`
внутри зашифрованного Amlogic secure-boot образа. Единственные «артефакты ядра» в
открытом виде — `.ko`-модули. Чтобы получить vmlinux: собрать из GPL-исходников
ядра Amlogic 4.9.268 (g12b) либо снять уже расшифрованное ядро с устройства.
