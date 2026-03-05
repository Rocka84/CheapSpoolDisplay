Import("env")

import os
from PIL import Image

def generate_logo():
    input_path = "web/logo.png"
    output_path = "src/assets/openspool_logo.c"
    name = "img_openspool_logo"
    bg_hex = "#0f1118"
    target_size = (80, 80)

    print(f"Pre-Build: Converting {input_path} to LVGL C array ({output_path})...")

    if not os.path.exists(input_path):
        print(f"Warning: {input_path} not found. Skipping automation.")
        return

    # Extract RGB from hex
    bg_color = tuple(int(bg_hex.lstrip('#')[i:i+2], 16) for i in (0, 2, 4))

    # Read the original logo (with transparency)
    img_orig = Image.open(input_path).convert("RGBA")
    
    # Resize to target size using LANCZOS
    img_resized = img_orig.resize(target_size, Image.LANCZOS)
    
    # Create the solid background matching the device UI
    img = Image.new("RGB", target_size, bg_color)
    
    # Composite the logo onto the background using its alpha channel as a mask
    img.paste(img_resized, mask=img_resized.split()[3])

    width, height = img.size

    with open(output_path, "w") as f:
        f.write('#include "lvgl.h"\n\n')
        f.write(f'static const uint8_t img_data_{name}[] = {{\n')
        
        data = list(img.getdata())
        for r, g, b in data:
            # Convert RGB888 to RGB565 (Hardware color depth)
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            # Little Endian byte order [LSB, MSB]
            f.write(f'    0x{rgb565 & 0xFF:02x}, 0x{(rgb565 >> 8) & 0xFF:02x},\n')
            
        f.write('};\n\n')
        f.write(f'const lv_image_dsc_t {name} = {{\n')
        f.write(f'  .header = {{ .cf = LV_COLOR_FORMAT_RGB565, .magic = LV_IMAGE_HEADER_MAGIC, .w = {width}, .h = {height}, .stride = {width * 2} }},\n')
        f.write('  .data_size = ' + str(width * height * 2) + ',\n')
        f.write(f'  .data = img_data_{name}\n')
        f.write('};\n')
        
    print(f"Pre-Build: Image conversion successful.")

generate_logo()
