#!/bin/bash
set -e -x

script=$(readlink -f "$0")
base=$(dirname "$script")
echo "base=$base"
tmp="$base/fonts.tmp"
rm -rf "$tmp"
mkdir -p "$tmp"
(cd "$tmp" && tar -xf "$base/dejavu-fonts-ttf-2.37.tar.bz2")
cp "$tmp/dejavu-fonts-ttf-2.37/ttf/"*.ttf "$tmp/"
(cd "$tmp" && unzip -q "$base/ProggyClean.ttf.zip")
(cd "$tmp" && unzip -q "$base/Fork-Awesome-1.1.7.zip")
cp "$tmp/Fork-Awesome-1.1.7/fonts/forkawesome-webfont.ttf" "$tmp/ForkAwesome.ttf"

g++ -o "$tmp/binary_to_compressed_c" "$base/binary_to_compressed_c.cpp"
c="$tmp/gui_fonts.cpp"
h="$tmp/gui_fonts.hpp"

echo "// Automatically generated. Do not edit." >> "$h"
echo "// Automatically generated. Do not edit." >> "$c"
echo "#ifndef __GUI_FONTS_HPP__" >> "$h"
echo "#define __GUI_FONTS_HPP__" >> "$h"
# ProggyClean
for F in DejaVuSansMono DejaVuSans DejaVuSans-Bold DejaVuSans-Oblique ForkAwesome; do
    FF=${F//-/}
    (cd $tmp && ./binary_to_compressed_c -base85 $F.ttf $FF >> "$c");
    echo "const char *guiGetFont${FF}(void) { return ${FF}_compressed_data_base85; }" >> "$c"
    echo "const char *guiGetFont${FF}(void);" >> "$h"
done
echo "#endif // __GUI_FONTS_HPP__" >> "$h"
cp "$h" "$base/../../cfggui/gui/gui_fonts.hpp"
cp "$c" "$base/../../cfggui/gui/gui_fonts.cpp"
rm -rf "$tmp"