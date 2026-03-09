#!/bin/bash

# Configuration
FONT_FILE="assets/fonts/Montserrat-Regular.ttf"

if [ -z "$FONT_FILE" ]; then
    echo "Error: Montserrat-Regular.ttf not found in assets/fonts/"
    exit 1
fi

generate_font() {
    local size=$1
    local output="src/ui/lv_font_german_${size}.c"
    
    echo "Generating ${size}px font from $FONT_FILE..."
    npx lv_font_conv --font "$FONT_FILE" --size "$size" --bpp 4 --range 0x20-0x7F,0xA0-0xFF --format lvgl --no-compress --no-prefilter --output "$output"
    
    # Fix LVGL includes to use brackets and be portable
    # We replace the entire #ifdef block with a simple #include <lvgl.h>
    # to avoid path issues in different environments.
    sed -i '/#ifdef LV_LVGL_H_INCLUDE_SIMPLE/,/#endif/c\#include <lvgl.h>' "$output"
    
    echo "Successfully generated $output"
}

# Generate both sizes
generate_font 14
generate_font 20

echo "Done! You can now rebuild the project."
