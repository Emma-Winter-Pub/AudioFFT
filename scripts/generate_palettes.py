import os
import sys
import glob

def interpolate(pos, nodes):
    if pos <= nodes[0][0]: return nodes[0][1:]
    if pos >= nodes[-1][0]: return nodes[-1][1:]
    for i in range(len(nodes) - 1):
        p1, p2 = nodes[i], nodes[i+1]
        if p1[0] <= pos <= p2[0]:
            t = (pos - p1[0]) / (p2[0] - p1[0]) if p2[0] != p1[0] else 0
            r = int(p1[1] + t * (p2[1] - p1[1]))
            g = int(p1[2] + t * (p2[2] - p1[2]))
            b = int(p1[3] + t * (p2[3] - p1[3]))
            return (max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b)))
    return (0, 0, 0)

def process_file(filepath, is_linear):
    palette_id = os.path.splitext(os.path.basename(filepath))[0]
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = [l.strip() for l in f.readlines() if l.strip()]
    if not lines: return None
    name = lines[0]
    data_lines = lines[1:]
    if data_lines and data_lines[0].lower() in ["scientific", "linear"]:
        data_lines = data_lines[1:]
    colors = []
    if is_linear:
        nodes = []
        for line in data_lines:
            parts = line.split(',')
            if len(parts) == 4:
                nodes.append((float(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])))
        if not nodes: return None
        nodes.sort(key=lambda x: x[0])
        for i in range(256):
            pos = i / 255.0
            r, g, b = interpolate(pos, nodes)
            colors.append(f"0xFF{r:02X}{g:02X}{b:02X}")
    else:
        parsed_hex = []
        for line in data_lines:
            hx = line.replace('#', '')
            if len(hx) == 6:
                parsed_hex.append(hx.upper())
        if not parsed_hex: return None
        for i in range(256):
            idx = int(i * (len(parsed_hex) - 1) / 255)
            colors.append(f"0xFF{parsed_hex[idx]}")
    return palette_id, name, colors

def generate_cpp_header(output_path, scientific_dir, linear_dir):
    palettes = []
    if os.path.exists(scientific_dir):
        for fp in glob.glob(os.path.join(scientific_dir, "*.txt")):
            res = process_file(fp, is_linear=False)
            if res: palettes.append(res)
    if os.path.exists(linear_dir):
        for fp in glob.glob(os.path.join(linear_dir, "*.txt")):
            res = process_file(fp, is_linear=True)
            if res: palettes.append(res)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("// 此文件自动生成，请勿修改。\n")
        f.write("// AUTO-GENERATED FILE. DO NOT EDIT.\n\n")
        f.write("#pragma once\n\n")
        f.write("#include <array>\n")
        f.write("#include <vector>\n")
        f.write("#include <cstdint>\n")
        f.write("#include <QString>\n\n")
        f.write("struct BuiltinPaletteData {\n")
        f.write("    QString id;\n")
        f.write("    QString name;\n")
        f.write("    std::array<uint32_t, 256> colors;\n")
        f.write("};\n\n")
        f.write("inline const std::vector<BuiltinPaletteData>& getBuiltinPalettes() {\n")
        f.write("    static const std::vector<BuiltinPaletteData> palettes = {\n")
        for pid, name, colors in palettes:
            color_str = ", ".join(colors)
            f.write(f'        {{ QStringLiteral("{pid}"), QStringLiteral("{name}"), {{ {color_str} }} }},\n')
        f.write("    };\n")
        f.write("    return palettes;\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_cpp_header(sys.argv[1], sys.argv[2], sys.argv[3])
