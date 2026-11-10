#!/bin/sh
# Сборка apk/adbroot — device-side авто-`adb root`.
# Требует Android SDK (build-tools + platforms/android-28).
set -e
SDK=${ANDROID_HOME:-$HOME/Library/Android/sdk}
BT=$(ls -d "$SDK"/build-tools/* | sort -V | tail -1)
AJ=$SDK/platforms/android-28/android.jar
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$HERE/build
rm -rf "$OUT"; mkdir -p "$OUT/classes"

"$BT/aapt2" link -o "$OUT/base.apk" -I "$AJ" --manifest "$HERE/AndroidManifest.xml"
javac -source 8 -target 8 -bootclasspath "$AJ" -d "$OUT/classes" "$HERE"/src/com/user/adbroot/*.java
"$BT/d8" --lib "$AJ" --min-api 28 --output "$OUT" "$OUT"/classes/com/user/adbroot/*.class

cp "$OUT/base.apk" "$OUT/app.apk"
(cd "$OUT" && zip -q -j app.apk classes.dex)
"$BT/zipalign" -f 4 "$OUT/app.apk" "$OUT/app-aligned.apk"

KS=${KS:-$HOME/.android/debug.keystore}
[ -f "$KS" ] || KS=$HERE/debug.keystore
[ -f "$KS" ] || keytool -genkeypair -keystore "$KS" -alias a -storepass android -keypass android \
    -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=adbroot" >/dev/null 2>&1
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android --out "$HERE/adbroot.apk" "$OUT/app-aligned.apk"
echo "OK: $HERE/adbroot.apk"
