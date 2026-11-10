# SberPortal — root

Инструменты и документация для получения на **SberPortal** (`stargate`, Amlogic A311D,
Android 9, ядро 4.9.268) постоянного root, который переживает перезагрузку.

| Что | Как |
|---|---|
| SELinux Permissive всегда | правка `preboot` в u-boot env → `androidboot.selinux=permissive` |
| `adb root` разрешён всегда | `androidboot.rabbith.adb_root=1` (вендорский механизм `rabbith` в `adbd`) |
| Авто-root после загрузки | APK `prebuilt/adbroot.apk`: `BOOT_COMPLETED` → `setprop service.adb.root 1` + `ctl.restart adbd` |
| Root без перезагрузки (первый раз) | эксплойт Mali **CVE-2022-38181** → `uid=0`, SELinux permissive |

AVB/dm-verity и разделы `system`/`boot` не изменяются — устройство грузится штатно.

---

## Быстрый старт

Нужен `adb` и сеть до портала (ADB по Wi-Fi на `:5555` уже включён).

```bash
D=<IP портала>:5555

# 1. Постоянный root: записать env и перезагрузить.
#    Вариант А — adb root уже работает (env правили раньше):
IP=<IP портала> ./scripts/persistent_root.sh

#    Вариант Б — ПЕРВЫЙ раз, adbd работает как shell:
IP=<IP портала> ./scripts/persistent_root_no_adb.sh
#    (два прохода эксплойтом: снять env -> make_env.py -> записать env -> reboot)

# 2. Авто-root после каждой загрузки (ставится один раз)
IP=<IP портала> ./scripts/install_adbroot_apk.sh
```

> `scripts/getroot.sh` даёт `uid=0` только процессу эксплойта (домен `shell`,
> SELinux Permissive). `adb root` начнёт работать **только после записи env** —
> adbd смотрит на `ro.boot.rabbith.adb_root`, а он появляется из `preboot`.

После шага 2 устройство при каждой загрузке отвечает на `adb root` → `uid=0` и
работает в Permissive. После шага 3 даже `adb root` вводить не нужно: adbd сразу
поднимается root.

Проверка:

```bash
adb connect <IP>:5555
adb shell id                 # uid=0(root) ... context=u:r:su:s0
adb shell getenforce         # Permissive
adb shell getprop ro.boot.rabbith.adb_root   # 1
```

## Что внутри

| путь | что |
|---|---|
| `ENV.md` | как устроен u-boot env и что именно правится для постоянного root |
| `EXPLOIT.md` | CVE-2022-38181: как работает, точные параметры запуска, сборка |
| `src/`, `build.sh` | исходники эксплойта (`shellcode.S` — эскалация cred+SELinux, `kallsyms.c/h` — заготовка рантайм-резолвера) и утилиты env (Android NDK, **без `-O2`**) |
| `kernel/` | `kernel_config.txt`, скрипты пересборки ядра, offsets/структуры-инструменты, отчёт о сборке (см. `kernel/README.md`) |
| `prebuilt/` | `raven_sber`, `hog`, `ubootenv`, `adbroot.apk` |
| `apk/adbroot/` | исходники device-side авто-root APK |
| `scripts/getroot.sh` | авто-получение root: hog + повтор до успеха |
| `scripts/persistent_root.sh` | снять env, добавить флаги, записать, проверить, перезагрузить (нужен `adb root`) |
| `scripts/persistent_root_no_adb.sh` | то же, но когда `adb root` ещё недоступен: два прохода эксплойтом через `SP_CMD` |
| `scripts/make_env.py` | хост-скрипт: правит `preboot` в 64 КБ блоке env и пересчитывает CRC32 |
| `scripts/install_adbroot_apk.sh` | установка APK авто-root |
| `scripts/sysrw.sh` | записываемый `/system` через overlayfs (до перезагрузки) |

## Требования и ограничения

- `adb root` до установки `adbroot.apk` нужно вызывать вручную после каждой загрузки.
- Эксплойт нужен **только** для первой записи env; после `persistent_root.sh` — не нужен.
- Эксплойт флаки: при неудаче ядро может уйти в перезагрузку — просто повторите
  `getroot.sh`.
- После OTA env может сброситься (`defenv`) — повторите `persistent_root.sh`
  (эксплойт для этого понадобится снова).
- После обновления `adbroot.apk` его нужно один раз запустить —
  `scripts/install_adbroot_apk.sh` делает это сам.

## Ограничения применения

Инструменты предназначены для исследования собственного устройства.
