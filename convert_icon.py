import os
import sys

# --- 修改這裡：讀取 .png 而不是 .ico ---
input_file = "icon.png" 
output_file = "icon_data.h"
var_name = "icon_data"

if not os.path.exists(input_file):
    print(f"Error: {input_file} not found!")
    sys.exit(1)

with open(input_file, "rb") as f:
    data = f.read()

with open(output_file, "w") as f:
    f.write(f"// Generated from {input_file}\n")
    f.write(f"#ifndef ICON_DATA_H\n#define ICON_DATA_H\n\n")
    f.write(f"#include <stddef.h>\n\n")
    f.write(f"const unsigned char {var_name}[] = {{\n")
    
    for i, byte in enumerate(data):
        f.write(f"0x{byte:02x}, ")
        if (i + 1) % 12 == 0:
            f.write("\n")
            
    f.write("\n};\n\n")
    f.write(f"const int {var_name}_len = {len(data)};\n\n")
    f.write(f"#endif // ICON_DATA_H\n")

print(f"Success! {output_file} created from PNG.")