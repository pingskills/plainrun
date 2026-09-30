#!/bin/sh
# Renders the PNG icon sizes from the SVG source. Run after editing the SVG and
# commit the results (so builds do not need an SVG renderer).
# Requires: rsvg-convert (librsvg)
set -eu
cd "$(dirname "$0")/.."
src=resources/icons/io.github.pingskills.plainrun.svg
for size in 16 32 48 64 128 256; do
    mkdir -p "resources/icons/png/$size"
    rsvg-convert -w "$size" -h "$size" "$src" -o "resources/icons/png/$size/io.github.pingskills.plainrun.png"
done
echo "Rendered icons from $src"
