#!/usr/bin/env bash
# Сборка эксплойта (armv7a, Android) и hog. Нужен Android NDK r21+ (проверено на r26).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
NDK=${NDK:-$HOME/Library/Android/sdk/ndk/26.1.10909125}
for pre in darwin-x86_64 linux-x86_64; do
  CC="$NDK/toolchains/llvm/prebuilt/$pre/bin/armv7a-linux-androideabi30-clang"
  [ -x "$CC" ] && break
done
[ -x "$CC" ] || { echo "Не найден clang NDK. Задай NDK=<path>"; exit 1; }

mkdir -p "$HERE/prebuilt"
"$CC" -DSHELL -I "$HERE/src" "$HERE/src/mali_shrinker_mmap32.c" -o "$HERE/prebuilt/raven_sber"
"$CC" "$HERE/src/hog.c" -o "$HERE/prebuilt/hog"
"$CC" "$HERE/src/ubootenv.c" -o "$HERE/prebuilt/ubootenv"
file "$HERE/prebuilt/raven_sber" "$HERE/prebuilt/hog"
echo "OK: prebuilt/raven_sber  prebuilt/hog"
