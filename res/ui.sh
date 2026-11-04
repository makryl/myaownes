magick -background none -fill white -font "tmp/oldschool_pc_font_pack_v2.2_FULL/ttf - Mx (mixed outline+bitmap)/Mx437_IBM_BIOS.ttf" -pointsize 8 +antialias -kerning 2 -interline-spacing 2 label:$'\ !"#$%%&\'()*+,-./012345678\n9:;<=>?@ABCDEFGHIJKLMNOPQ\nRSTUVWXYZ[\\\\]^_`abcdefghij\nklmnopqrstuvwxyz{|}~' -gravity NorthWest -splice 1x1 +repage res/ui.png

xxd -i res/ui.png src/app/res_ui.h
