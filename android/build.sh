#!/bin/bash

set -e

CUR_DIR="$(dirname "$0")"
ROOT_DIR=$CUR_DIR/..

NAME="com.makryl.myaownes"
MANIFEST=$CUR_DIR/AndroidManifest.xml
ICON_FG=$CUR_DIR/icon.png
ICON_BG="800080"
SDL_DIR=$ROOT_DIR/external/SDL

OUT_DIR=$ROOT_DIR/build/android
PROJ_DIR=$OUT_DIR/$NAME
RES_DIR=$PROJ_DIR/app/src/main/res/

# Generate project
$SDL_DIR/build-scripts/create-android-project.py $NAME $ROOT_DIR/src/main.c $ROOT_DIR/src/core/* $ROOT_DIR/include/* -o $OUT_DIR

# Overwrite configs
sed -i 's/add_library/set(CMAKE_C_STANDARD 23)\nset(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG -flto")\nset(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "-flto")\nadd_library/' $PROJ_DIR/app/jni/src/CMakeLists.txt
sed -i 's/release {/release {\n            signingConfig signingConfigs.debug/' $PROJ_DIR/app/build.gradle

# Overwrite manifest
cp -f $MANIFEST $PROJ_DIR/app/src/main/AndroidManifest.xml

# Overwrite icons
mkdir -p $RES_DIR/mipmap-anydpi-v26
mkdir -p $RES_DIR/mipmap-mdpi
mkdir -p $RES_DIR/mipmap-hdpi
mkdir -p $RES_DIR/mipmap-xhdpi
mkdir -p $RES_DIR/mipmap-xxhdpi
mkdir -p $RES_DIR/mipmap-xxxhdpi

cat <<EOT > $RES_DIR/mipmap-anydpi-v26/ic_launcher.xml
<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@mipmap/ic_launcher_background" />
    <foreground android:drawable="@mipmap/ic_launcher_foreground" />
    <monochrome android:drawable="@mipmap/ic_launcher_foreground" />
</adaptive-icon>
EOT

cat <<EOT > $RES_DIR/mipmap-anydpi-v26/ic_launcher_background.xml
<?xml version="1.0" encoding="utf-8"?>
<color xmlns:android="http://schemas.android.com/apk/res/android" android:color="#$ICON_BG" />
EOT

convert $ICON_FG -resize  48x48  $RES_DIR/mipmap-mdpi/ic_launcher.png
convert $ICON_FG -resize  72x72  $RES_DIR/mipmap-hdpi/ic_launcher.png
convert $ICON_FG -resize  96x96  $RES_DIR/mipmap-xhdpi/ic_launcher.png
convert $ICON_FG -resize 144x144 $RES_DIR/mipmap-xxhdpi/ic_launcher.png
convert $ICON_FG -resize 192x192 $RES_DIR/mipmap-xxxhdpi/ic_launcher.png

convert $ICON_FG -gravity center -background none -resize  72x72  -extent 108x108 $RES_DIR/mipmap-mdpi/ic_launcher_foreground.png
convert $ICON_FG -gravity center -background none -resize 108x108 -extent 162x162 $RES_DIR/mipmap-hdpi/ic_launcher_foreground.png
convert $ICON_FG -gravity center -background none -resize 144x144 -extent 216x216 $RES_DIR/mipmap-xhdpi/ic_launcher_foreground.png
convert $ICON_FG -gravity center -background none -resize 216x216 -extent 324x324 $RES_DIR/mipmap-xxhdpi/ic_launcher_foreground.png
convert $ICON_FG -gravity center -background none -resize 288x288 -extent 432x432 $RES_DIR/mipmap-xxxhdpi/ic_launcher_foreground.png

# Run build
(cd $PROJ_DIR && ./gradlew assembleRelease -PBUILD_WITH_CMAKE)

mkdir -p $ROOT_DIR/dist/Android-Release
cp $PROJ_DIR/app/build/outputs/apk/release/app-release.apk $ROOT_DIR/dist/myaownes_latest_android_arm64-v8a.apk
