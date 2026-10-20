"""
herramientas/make_todos_niveles.py

Genera los 3 niveles completos (96x16 tiles = 768x128 px cada uno):
  - Nivel 1: Pradera e Islas (Nivel introductorio con escalones y gemas)
  - Nivel 2: Fortaleza y Torres (Estructuras escalonadas accesibles, sin muros impasables)
  - Nivel 3: Desafio de Abismos (Plataformas intermedias sobre vacios y saltos de precision)
"""

import os
from PIL import Image

W, H = 96, 16

# -------------------------------------------------------------------------
# NIVEL 1: Pradera e Islas
# -------------------------------------------------------------------------
LAYOUT_1 = [
    # 0..15 (Y=0..15)
    "................................................................................................", # 0
    "................................................................................................", # 1
    "................................................................................................", # 2
    "......................................................oooo............................####......", # 3
    "......................................................................................#oo#......", # 4
    "..........ooo...................oooo..................................................#oo#......", # 5
    "......................................................................................#oo#......", # 6
    "......................................................................................#oo#......", # 7
    "......................................................##..................##..........####......", # 8
    "............................####......................##....######........##....................", # 9
    "........##..................####....................######..######......######..................", # 10
    "....oo####....##..........########....##....##......######..######......######..................", # 11
    "GGGGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGGGG..GGGGGGGG..GGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGG", # 12
    "GGGGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGGGG..GGGGGGGG..GGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGG", # 13
    "GGGGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGGGG..GGGGGGGG..GGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGG", # 14
    "GGGGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGG..GGGGGGGGGGGG..GGGGGGGG..GGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGG", # 15
]

# -------------------------------------------------------------------------
# NIVEL 2: Fortaleza y Torres Escalonadas (100% franqueables con saltos de 1-2 tiles)
# -------------------------------------------------------------------------
LAYOUT_2 = [
    "................................................................................................", # 0
    "................................................................................................", # 1
    "................................................................................................", # 2
    "......................................................................................####......", # 3
    "......................................................................................#oo#......", # 4
    "......................................................................................#oo#......", # 5
    "......................................................................................#oo#......", # 6
    "......................................................................................#oo#......", # 7
    "........oo..................oo................oo......................oo..............####......", # 8
    "........##..................##..........####..##............####......##........................", # 9
    "....oo######..............######......##########..........########..######..oo..................", # 10
    "..##########............##########..##############......######################..................", # 11
    "GGGGGGGGGGGGGG..GGGGGGGGGGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG", # 12
    "GGGGGGGGGGGGGG..GGGGGGGGGGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG", # 13
    "GGGGGGGGGGGGGG..GGGGGGGGGGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG", # 14
    "GGGGGGGGGGGGGG..GGGGGGGGGGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG", # 15
]

# -------------------------------------------------------------------------
# NIVEL 3: Desafio de Abismos y Precision (Plataformas intermedias accesibles)
# -------------------------------------------------------------------------
LAYOUT_3 = [
    "................................................................................................", # 0
    "................................................................................................", # 1
    "................................................................................................", # 2
    "......................................................................................####......", # 3
    "......................................................................................#oo#......", # 4
    "......................................................................................#oo#......", # 5
    "......................................................................................#oo#......", # 6
    "......................................................................................#oo#......", # 7
    "..............oo..........................oo..............................oo..........####......", # 8
    "..............##..........................##............####..............##....................", # 9
    "....oo......######..........oo..........######........########..........######..................", # 10
    "....####..##########......######......##########....############......##########................", # 11
    "GGGGGGGG..GGGGGGGGGG..GG..GGGGGG..GG..GGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGG", # 12
    "GGGGGGGG..GGGGGGGGGG..GG..GGGGGG..GG..GGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGG", # 13
    "GGGGGGGG..GGGGGGGGGG..GG..GGGGGG..GG..GGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGG", # 14
    "GGGGGGGG..GGGGGGGGGG..GG..GGGGGG..GG..GGGGGGGGGG..GGGGGGGGGGGGGG..GG..GGGGGGGGGGGGGGGGGGGGGGGGGG", # 15
]

def build_data(layout):
    assert len(layout) == H, f"Alto incorrecto: {len(layout)}"
    data = []
    for y, row in enumerate(layout):
        assert len(row) == W, f"Fila {y} tiene {len(row)} cols, se esperaban {W}"
        for x, c in enumerate(row):
            if c == ".":
                data.append(0)
            elif c == "o":
                data.append(1)
            elif c == "#":
                data.append(2)
            elif c == "G":
                tile_val = [[3, 4], [5, 6]][y % 2][x % 2]
                data.append(tile_val)
            else:
                raise ValueError(f"Caracter no valido: {c} en ({x}, {y})")

    # Verificar que los bloques de suelo G sean pares de columnas
    for y, row in enumerate(layout):
        for x in range(0, W, 2):
            assert (row[x] == "G") == (row[x + 1] == "G"), f"Bloque suelo impar en fila {y}, col {x}"

    return data

def export_all():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = os.path.join(root, "codigo", "niveles")
    os.makedirs(out_dir, exist_ok=True)

    d1 = build_data(LAYOUT_1)
    d2 = build_data(LAYOUT_2)
    d3 = build_data(LAYOUT_3)

    def format_array(name, data):
        lines = []
        for i in range(0, len(data), 12):
            chunk = data[i:i + 12]
            lines.append("  " + ",".join(f"0x{v:02X}" for v in chunk))
        body = ",\n".join(lines)
        return (
            f"const unsigned char {name}[] = {{\n"
            f"{body}\n"
            f"}};\n\n"
        )

    c_code = (
        "/*\n"
        " * NivelesData.c\n"
        " * Definicion de mapas de 96x16 tiles (768x128 px) para Niveles 1, 2 y 3.\n"
        " * Generado automaticamente por tools/make_todos_niveles.py.\n"
        " */\n\n"
        + format_array("Nivel1_Map", d1)
        + format_array("Nivel2_Map", d2)
        + format_array("Nivel3_Map", d3)
        + "const unsigned char* const niveles_mapas[3] = {\n"
        "    Nivel1_Map,\n"
        "    Nivel2_Map,\n"
        "    Nivel3_Map\n"
        "};\n\n"
        "/* Fin de NivelesData.c */\n"
    )

    h_code = (
        "/* NivelesData.h */\n"
        "#ifndef NIVELES_DATA_H\n"
        "#define NIVELES_DATA_H\n\n"
        "#define TOTAL_NIVELES 3\n"
        "#define MAP_W_TILES   96\n"
        "#define MAP_H_TILES   16\n\n"
        "extern const unsigned char Nivel1_Map[];\n"
        "extern const unsigned char Nivel2_Map[];\n"
        "extern const unsigned char Nivel3_Map[];\n"
        "extern const unsigned char* const niveles_mapas[3];\n\n"
        "#endif\n"
    )

    c_path = os.path.join(out_dir, "NivelesData.c")
    h_path = os.path.join(out_dir, "NivelesData.h")

    with open(c_path, "w", encoding="ascii") as f:
        f.write(c_code)
    with open(h_path, "w", encoding="ascii") as f:
        f.write(h_code)

    print(f"Exportado: {c_path}")
    print(f"Exportado: {h_path}")

    # Generar vistas previas graficas PNG
    preview_dir = os.path.join(root, "recursos", "preview")
    os.makedirs(preview_dir, exist_ok=True)

    PALETTE = {
        0: (224, 248, 208),
        1: (136, 192, 112),
        2: (52,  104,  86),
        3: (8,    24,  32),
        4: (52,  104,  86),
        5: (136, 192, 112),
        6: (8,    24,  32)
    }

    def render_png(data, filename):
        img = Image.new("RGB", (W * 8, H * 8))
        pixels = img.load()
        for ty in range(H):
            for tx in range(W):
                tval = data[ty * W + tx]
                col = PALETTE.get(tval, (224, 248, 208))
                for py in range(8):
                    for px in range(8):
                        if tval != 0 and (px == 0 or py == 0):
                            edge_col = (max(0, col[0] - 30), max(0, col[1] - 30), max(0, col[2] - 30))
                            pixels[tx * 8 + px, ty * 8 + py] = edge_col
                        else:
                            pixels[tx * 8 + px, ty * 8 + py] = col
        p_path = os.path.join(preview_dir, filename)
        img.save(p_path)
        print(f"Preview generada: {p_path}")

    render_png(d1, "nivel_1.png")
    render_png(d2, "nivel_2.png")
    render_png(d3, "nivel_3.png")

if __name__ == "__main__":
    export_all()
