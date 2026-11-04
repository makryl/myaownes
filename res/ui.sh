#!/bin/bash

set -e

CUR_DIR="$(dirname "$0")"
ROOT_DIR=$CUR_DIR/..
BUILD_DIR=$ROOT_DIR/build/res

mkdir -p $BUILD_DIR

font() {
  magick +antialias -background none -fill white -font "$1" -pointsize 8 -kerning 2 -interline-spacing 2 \
    label:$'\ !"#$%%&\'()*+,-./012345678\n9:;<=>?@ABCDEFGHIJKLMNOPQ\nRSTUVWXYZ[\\\\]^_`abcdefghij\nklmnopqrstuvwxyz{|}~' \
    -gravity NorthWest -splice 1x1 +repage \
    "$2"
}

button() {
  magick -size $4x$4 xc:none \
    -fill none -stroke "#80808080" -strokewidth 1 \
    -draw "circle $5,$5 $5,1" \
    -fill "#80808080" -stroke none -font "$1" -pointsize 8 -gravity center \
    -draw "text 1,1 '$3'" \
    "$2"
}

dpad() {
  magick -size 256x256 xc:none \
    -fill none -stroke "#80808080" -strokewidth 1 \
    -draw "circle 127.5,127.5 127.5,1" \
    -draw "circle 127.5,127.5 127.5,95" \
    -fill "#80808080" -stroke none -font "$1" -pointsize 8 -gravity center \
    -draw "text 0,-80 '▲'" \
    -draw "text 0,80 '▼'" \
    -draw "text 80,0 '►'" \
    -draw "text -80,0 '◄'" \
    "$2"
}

atlas() {
  font "$1" $BUILD_DIR/font.png
  dpad "$1" $BUILD_DIR/dpad.png
  button "$1" $BUILD_DIR/button_a.png A 128 63.5
  button "$1" $BUILD_DIR/button_b.png B 128 63.5
  button "$1" $BUILD_DIR/button_start.png Start 64 31.5
  button "$1" $BUILD_DIR/button_select.png Select 64 31.5
  button "$1" $BUILD_DIR/button_menu.png Menu 64 31.5
  button "$1" $BUILD_DIR/button_ff.png ">>" 64 31.5

  magick -size 512x256 xc:none \
    $BUILD_DIR/font.png -geometry +0+0 -composite \
    $BUILD_DIR/dpad.png -geometry +256+0 -composite \
    $BUILD_DIR/button_a.png -geometry +0+128 -composite \
    $BUILD_DIR/button_b.png -geometry +128+128 -composite \
    $BUILD_DIR/button_start.png -geometry +0+64 -composite \
    $BUILD_DIR/button_select.png -geometry +64+64 -composite \
    $BUILD_DIR/button_menu.png -geometry +128+64 -composite \
    $BUILD_DIR/button_ff.png -geometry +192+64 -composite \
    "$2"
}

atlas "$ROOT_DIR/tmp/oldschool_pc_font_pack_v2.2_FULL/ttf - Mx (mixed outline+bitmap)/Mx437_IBM_BIOS.ttf" "$CUR_DIR/ui_default.png"
atlas "$ROOT_DIR/tmp/oldschool_pc_font_pack_v2.2_FULL/ttf - Mx (mixed outline+bitmap)/Mx437_EagleSpCGA_Alt1.ttf" "$CUR_DIR/ui_sans.png"
atlas "$ROOT_DIR/tmp/oldschool_pc_font_pack_v2.2_FULL/ttf - Mx (mixed outline+bitmap)/Mx437_EagleSpCGA_Alt2.ttf" "$CUR_DIR/ui_scifi.png"
atlas "$ROOT_DIR/tmp/oldschool_pc_font_pack_v2.2_FULL/ttf - Mx (mixed outline+bitmap)/Mx437_EagleSpCGA_Alt3.ttf" "$CUR_DIR/ui_fantasy.png"

xxd -i -n res_ui_png $CUR_DIR/ui_default.png $ROOT_DIR/src/app/res_ui.h
