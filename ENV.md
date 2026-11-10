# Постоянный root через u-boot env

Устройство: SberPortal (`stargate`), Amlogic A311D / G12B, Android 9, слот `_b`,
bootloader locked, AVB 1.1, dm-verity enforcing.

## Где лежит env

| | |
|---|---|
| раздел | `/dev/block/env` (`mmcblk0p4`), 8 МБ |
| формат | `crc32(LE, 4 байта) | data`, `CONFIG_ENV_SIZE=0x10000` (64 КБ) |
| валидная копия | одна, в offset 0 |
| запись | `dd if=env.bin of=/dev/block/env bs=4096` + `sync` |

## Разделы (MMC)

```
p19 vendor_a   p20 vendor_b     1204 МБ
p21 system_a   p22 system_b      800 МБ   активный слот _b, корень dm-0 "vroot"
p23 sdvtz_a    p24 sdvtz_b       300 МБ   StarOS (SquashFS)
p25 data       p26 dt_sdsr ... p31 cache
```

## Ключевые переменные

```
bootcmd   = ddr_auto_fast_boot_check 6 0 0 50; if efuse test 0x0; then fastboot; reboot; else run storeboot; fi;
preboot   = run bcb_cmd; run init_display;run factory_reset_key;run storeargs;forceupdate;bcb uboot-command;run check_sdts_mode;run switch_bootmode;
bcb_cmd   = get_avb_mode;get_valid_slot;
avb2=1    EnableSelinux=enforcing    rabbith_has_adb_root=yes
```

`storeboot`, `storeargs` и `EnableSelinux` u-boot восстанавливает сам при каждой
загрузке — править их в env смысла нет. Редактируются и сохраняются `preboot`,
`bootcmd`, `bcb_cmd`.

## Вендорский механизм rabbith

`adbd` (32-бит) не сбрасывает привилегии, если одновременно:

* `rabbith_enabled()` ← `ro.vendor.has_rabbith` (в `vendor/default.prop` уже `true`);
* `rabbith_has_adb_root()` ← `ro.boot.rabbith.adb_root` (из cmdline
  `androidboot.rabbith.adb_root`).

Дополнительно нужен `ro.boot.selinux=permissive`, иначе adbd отказывается работать
root'ом при включённом SELinux.

## Рабочая правка (`preboot`)

```
setenv EnableSelinux permissive;setenv rabbith_has_adb_root yes;run bcb_cmd; run init_display;
run factory_reset_key;run storeargs;forceupdate;bcb uboot-command;run check_sdts_mode;run switch_bootmode;
setenv bootargs ${bootargs} androidboot.rabbith.adb_root=1;
```

Почему именно такой порядок:

* `EnableSelinux` и `rabbith_has_adb_root` выставляются **до** `run storeargs`:
  `storeargs` кладёт `androidboot.selinux=permissive` первым, а `ro.boot.selinux`
  write-once — иначе останется `enforcing`.
* `switch_bootmode` для normal-загрузки снова вызывает `storeargs` и перетирает
  `bootargs`, поэтому `androidboot.rabbith.adb_root=1` добавляется **после**
  `run switch_bootmode`.
* условия в `preboot` пишутся inline (`if ...; then ...; fi`), а не через `run`-переменную.

Блок env генерируется из текущего env самого устройства: `scripts/persistent_root.sh`
снимает `/dev/block/env`, `scripts/make_env.py` меняет только `preboot` и пересчитывает
CRC32, затем блок пишется обратно и проверяется побайтово. Серийники и прочие
переменные остаются родными.

## Если `adb root` ещё не работает (первый раз)

`scripts/persistent_root.sh` требует `adb root` -> `uid=0`, а на «чистом»
устройстве adbd работает как `shell`. Эксплойт же даёт root только своему
процессу, поэтому env пишется в два прохода через `SP_CMD`:

```bash
IP=<IP портала> ./scripts/persistent_root_no_adb.sh
```

Что происходит:

1. эксплойт с `SP_CMD="sh /data/local/tmp/ravn_env_dump.sh"` снимает
   `/dev/block/env` в `/data/local/tmp/env_orig.bin` (бэкап остаётся на хосте
   в `.env-work/env_orig.bin`);
2. `scripts/make_env.py` меняет только `preboot` и пересчитывает CRC32;
3. второй проход с `SP_CMD="sh /data/local/tmp/ravn_env_write.sh"` пишет блок
   обратно и проверяет его побайтово (`ENV_OK`);
4. перезагрузка, затем `adb root` -> `uid=0`.

Ключевой момент — холст шелл-кода: `SP_FUNC=0x19bbf0`
(`SP_FUNC_ABS=0xffffff800921bbf0`, `vmalloc`). Дефолт у `VERIFY=13` —
`__alloc_page_frag` (`0x162f50`), горячий сетевой путь: пока в нём лежит
шелл-код, пакет с мусорными аргументами роняет ядро. `getroot.sh` и
`persistent_root_no_adb.sh` передают `vmalloc` явно.

## Проверка после загрузки

```
adb root
adb shell id                          # uid=0(root) ... context=u:r:su:s0
adb shell getenforce                  # Permissive
getprop ro.boot.selinux               # permissive
getprop ro.boot.rabbith.adb_root      # 1
```

Переживает cold power-cycle (полное отключение питания). Эксплойт после этого не нужен.

## Откат

`persistent_root.sh` сохраняет исходный блок в `scripts/../.env-work/env_orig.bin`
(каталог в `.gitignore`). Вернуть:

```sh
adb push .env-work/env_orig.bin /data/local/tmp/env_orig.bin
adb shell 'dd if=/data/local/tmp/env_orig.bin of=/dev/block/env bs=4096; sync'
adb reboot
```

Если бэкап потерян — переменную `preboot` можно вернуть вручную в редакторе env
(CRC пересчитывается `scripts/make_env.py`).
