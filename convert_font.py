import os

input_file = "consola.ttf"
output_file = "font_data.h"
var_name = "consola_ttf_data"

if not os.path.exists(input_file):
    print(f"Error: {input_file} not found!")
    exit()

with open(input_file, "rb") as f:
    data = f.read()

with open(output_file, "w") as f:
    f.write(f"// Generated from {input_file}\n")
    f.write(f"#include <stddef.h>\n\n")
    f.write(f"const unsigned char {var_name}[] = {{\n")
    
    # Write hex data
    for i, byte in enumerate(data):
        f.write(f"0x{byte:02x}, ")
        if (i + 1) % 12 == 0:
            f.write("\n")
            
    f.write("\n};\n\n")
    f.write(f"const int {var_name}_len = {len(data)};\n")

print(f"Success! {output_file} created.")