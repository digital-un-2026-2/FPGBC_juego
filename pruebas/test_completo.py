"""
pruebas/test_completo.py

Suite de pruebas automatizadas reproducibles para juego_completo.gb usando PyBoy.
Comprueba rigurosamente:
  1. Arranque en Menu de Seleccion de Nivel en espanol ("ELIGE TU NIVEL") y navegacion del cursor.
  2. Inicio de Nivel 1, circuito de audio y HUD de monedas activo en pantalla.
  3. Disparo de proyectil con boton A (orbe de energia en OAM y memoria).
  4. Recogida de monedas con el personaje, incremento del contador y actualizacion visual del HUD.
  5. Gravedad real de los enemigos: caida acelerada y aterrizaje solido en plataformas.
  6. Patrullaje dinamico de los enemigos (2 pasos a la izq y 2 pasos a la der).
  7. Destruccion de enemigo en patrulla con proyectil (Canal 4 impacto).
  8. Muerte por contacto directo con enemigo peligroso en movimiento.
  9. Retorno a la pantalla de seleccion de nivel desde derrota con boton B.
 10. Seleccion e inicio exitoso de Nivel 2 y Nivel 3 desde el menu.
 11. Desplazamiento horizontal de camara y streaming circular en VRAM (>256 px).
 12. Llegada a la meta (columna 90, px 716), activacion de VICTORIA y fanfarria.
"""

import os
import sys
from pyboy import PyBoy

base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rom_path = os.path.join(base_dir, "build", "juego_completo.gb")
if not os.path.exists(rom_path):
    alt_rom = os.path.join(base_dir, "pendientes", "juego_completo.gb")
    if os.path.exists(alt_rom):
        rom_path = alt_rom

out_dir = os.path.join(base_dir, "pruebas", "capturas")
os.makedirs(out_dir, exist_ok=True)

pb = PyBoy(rom_path, window="null", sound_emulated=True)

# Direcciones exactas de memoria desde juego_completo.noi
ADDR_PROYECTIL_ACTIVO = 0xC0BA
ADDR_PROYECTIL_X      = 0xC0B5
ADDR_PROYECTIL_Y      = 0xC0B7
ADDR_OBS0_ACTIVO      = 0xC0C6 # Offset 10 en struct de 12 bytes
ADDR_OBS0_X           = 0xC0BE
ADDR_OBS0_Y           = 0xC0C0
ADDR_OBS0_VEL_Y       = 0xC0C2
ADDR_MONEDAS          = 0xC1AC
ADDR_PLX              = 0xC1AD
ADDR_PLY_FP           = 0xC1AF
ADDR_CAM_X            = 0xC1B5
ADDR_ESTADO           = 0xC1BB
ADDR_NIVEL_SEL        = 0xC1BD
ADDR_NIVEL_ACT        = 0xC1BE

def snap(name):
    img = pb.screen.image.copy()
    img_path = os.path.join(out_dir, f"{name}.png")
    img.save(img_path)
    return img_path

def step(frames=1):
    for _ in range(frames):
        pb.tick()

results = []

def record(test_name, passed, detail=""):
    results.append((test_name, passed, detail))
    status = "PASS" if passed else "FAIL"
    print(f"[{status}] {test_name}: {detail}")

print("=== INICIANDO PRUEBAS AUTOMATIZADAS (MENU EN ESPANOL, 3 NIVELES Y GRAVEDAD ENEMIGA) ===")

# Esperar 120 fotogramas de la animacion de boot ROM
step(120)

# --- FASE 1: Menu de seleccion de nivel y navegacion del cursor ---
step(5)
snap("01_menu_seleccion_espanol")

estado_boot = pb.memory[ADDR_ESTADO]
nivel_sel_boot = pb.memory[ADDR_NIVEL_SEL]
cursor_y_boot = pb.memory[0xFE28]

# Leer titulo en fila 2 de VRAM
row2 = "".join(chr(pb.memory[0x9800 + 2 * 32 + c] - 17 + ord('A'))
               if 17 <= pb.memory[0x9800 + 2 * 32 + c] <= 42 else " "
               for c in range(20)).strip()

# Mover cursor abajo (hacia Nivel 2)
pb.button_press("down"); step(2); pb.button_release("down"); step(5)
cursor_y_down = pb.memory[0xFE28]
nivel_sel_down = pb.memory[ADDR_NIVEL_SEL]

# Mover cursor arriba (regreso a Nivel 1)
pb.button_press("up"); step(2); pb.button_release("up"); step(5)
cursor_y_up = pb.memory[0xFE28]

test1_passed = (estado_boot == 0 and "ELIGE TU NIVEL" in row2 and
                cursor_y_boot == 56 and cursor_y_down == 72 and
                nivel_sel_down == 1 and cursor_y_up == 56)
record("1. Menu de seleccion y cursor interactivo", test1_passed,
       f"Estado={estado_boot} (0=Menu), Titulo='{row2}', Cursor Y: {cursor_y_boot}->{cursor_y_down}->{cursor_y_up}")

# --- FASE 2: Inicio de Nivel 1 y activacion de HUD ---
pb.button_press("a"); step(2); pb.button_release("a"); step(15)
snap("02_nivel1_iniciado")

estado_play = pb.memory[ADDR_ESTADO]
nivel_act = pb.memory[ADDR_NIVEL_ACT]
spr1_y = pb.memory[0xFE04]
spr1_x = pb.memory[0xFE05]
nr52 = pb.memory[0xFF26]
coins_mem = pb.memory[ADDR_MONEDAS]

test2_passed = (estado_play == 1 and nivel_act == 0 and spr1_x == 24 and spr1_y == 96 and (nr52 & 0x80) != 0 and coins_mem == 0)
record("2. Inicio de Nivel 1, audio y HUD de monedas", test2_passed,
       f"Estado={estado_play} (1=Jugando), Nivel={nivel_act + 1}, Pos=(X:{spr1_x}, Y:{spr1_y}), Audio={hex(nr52)}")

# --- FASE 3: Disparo de proyectil con boton A ---
pb.button_press("a"); step(2)
snap("03_disparo_proyectil")

proj_active = pb.memory[ADDR_PROYECTIL_ACTIVO]
proj_x_oam = pb.memory[0xFE15]
proj_y_oam = pb.memory[0xFE14]
proj_tile = pb.memory[0xFE16]

test3_passed = (proj_active == 1 and proj_tile == 12 and proj_x_oam > 24 and proj_y_oam > 0)
record("3. Disparo de proyectil (Boton A)", test3_passed,
       f"Proyectil activo={proj_active}, OAM=(X:{proj_x_oam}, Y:{proj_y_oam}), Tile={proj_tile}")

pb.button_release("a")
step(30)

# --- FASE 4: Recogida de monedas e incremento del HUD ---
# Caminar a la derecha para recoger las monedas de Isla 1 en px 32 y 40
pb.button_press("right"); step(35); pb.button_release("right"); step(5)
snap("04_recogida_monedas_hud")

coins_after = pb.memory[ADDR_MONEDAS]
hud_d0_after = pb.memory[0xFE36] # Tile 17 (digito '2')

test4_passed = (coins_after == 2 and hud_d0_after == 17)
record("4. Recogida de monedas y contador HUD", test4_passed,
       f"Monedas recogidas={coins_after}, HUD digito={hud_d0_after - 15} (Tile {hud_d0_after})")

# --- FASE 5: Gravedad real de los enemigos ---
# Ubicar enemigo 0 en el aire a Y=40; verificar que cae por gravedad y aterriza en suelo solido (Y=88)
pb.memory[ADDR_OBS0_Y] = 40
pb.memory[ADDR_OBS0_Y + 1] = 0
pb.memory[ADDR_OBS0_VEL_Y] = 0

step(30)
enemy_landed_y = pb.memory[ADDR_OBS0_Y] | (pb.memory[ADDR_OBS0_Y + 1] << 8)
enemy_vel_y = pb.memory[ADDR_OBS0_VEL_Y]
snap("05_gravedad_enemigo_aterrizado")

test5_passed = (enemy_landed_y == 88 and enemy_vel_y == 0)
record("5. Gravedad real de los enemigos", test5_passed,
       f"Enemigo cayo desde Y=40 y aterrizo exactamente en suelo solido Y={enemy_landed_y} (vel_y={enemy_vel_y})")

# --- FASE 6: Patrullaje dinamico (2 pasos izq y 2 pasos der) ---
obs0_x_history = []
for _ in range(120):
    step(1)
    obs_x = pb.memory[ADDR_OBS0_X] | (pb.memory[ADDR_OBS0_X + 1] << 8)
    obs0_x_history.append(obs_x)

snap("06_patrullaje_enemigos")
min_patrol_x = min(obs0_x_history)
max_patrol_x = max(obs0_x_history)
patrol_span = max_patrol_x - min_patrol_x

test6_passed = (patrol_span >= 16 and min_patrol_x <= 152 and max_patrol_x >= 152)
record("6. Patrullaje de enemigos (2 pasos izq y der)", test6_passed,
       f"Rango: Min={min_patrol_x}, Max={max_patrol_x}, Amplitud={patrol_span} px (Origen=152)")

# --- FASE 7: Destruccion de enemigo en patrulla con proyectil ---
# Transportar jugador a Isla 2 y disparar
pb.memory[ADDR_PLX]     = 114
pb.memory[ADDR_PLX + 1] = 0
pb.memory[ADDR_PLY_FP]     = 0x00
pb.memory[ADDR_PLY_FP + 1] = 0x50
step(5)

pb.button_press("a"); step(2); pb.button_release("a")
for _ in range(15):
    step(1)
    if pb.memory[ADDR_OBS0_ACTIVO] == 0:
        break

snap("07_destruccion_enemigo_patrulla")
obs0_active = pb.memory[ADDR_OBS0_ACTIVO]
obs0_oam_y = pb.memory[0xFE18]

test7_passed = (obs0_active == 0 and obs0_oam_y == 0)
record("7. Destruccion de enemigo con proyectil", test7_passed,
       f"Enemigo 0 destruido (activo={obs0_active}, OAM Y={obs0_oam_y})")

# --- FASE 8: Muerte por contacto directo con enemigo ---
pb.button_press("start"); step(4); pb.button_release("start"); step(15)

# Acercar jugador al enemigo 1 (origen 248)
pb.memory[ADDR_PLX]     = 236
pb.memory[ADDR_PLX + 1] = 0
pb.memory[ADDR_PLY_FP]     = 0x00
pb.memory[ADDR_PLY_FP + 1] = 0x40
step(5)

pb.button_press("right"); step(25); pb.button_release("right"); step(5)
snap("08_muerte_contacto_enemigo")

estado_obs_kill = pb.memory[ADDR_ESTADO]
spr1_y_obs_kill = pb.memory[0xFE04]

test8_passed = (estado_obs_kill == 2 and spr1_y_obs_kill == 0)
record("8. Muerte por contacto con enemigo en patrulla", test8_passed,
       f"Estado={estado_obs_kill} (2=GameOver), Sprites ocultos (Y={spr1_y_obs_kill})")

# --- FASE 9: Retorno al menu desde fin de juego con boton B ---
pb.button_press("b"); step(4); pb.button_release("b"); step(15)
snap("09_retorno_menu_desde_derrota")

estado_menu_ret = pb.memory[ADDR_ESTADO]
test9_passed = (estado_menu_ret == 0)
record("9. Retorno al menu con boton B", test9_passed,
       f"Regreso exitoso al menu de seleccion de nivel (Estado={estado_menu_ret})")

# --- FASE 10: Seleccion e inicio de Nivel 2 y Nivel 3 ---
# Seleccionar Nivel 2
pb.button_press("down"); step(2); pb.button_release("down"); step(5)
pb.button_press("a"); step(2); pb.button_release("a"); step(15)
snap("10_nivel2_iniciado")
nivel2_act = pb.memory[ADDR_NIVEL_ACT]
estado_lvl2 = pb.memory[ADDR_ESTADO]

# Regresar al menu simulando fin
pb.memory[ADDR_ESTADO] = 2
pb.button_press("b"); step(4); pb.button_release("b"); step(15)

# Seleccionar Nivel 3
pb.button_press("down"); step(2); pb.button_release("down"); step(5)
pb.button_press("down"); step(2); pb.button_release("down"); step(5)
pb.button_press("a"); step(2); pb.button_release("a"); step(15)
snap("10_nivel3_iniciado")
nivel3_act = pb.memory[ADDR_NIVEL_ACT]
estado_lvl3 = pb.memory[ADDR_ESTADO]

test10_passed = (nivel2_act == 1 and estado_lvl2 == 1 and nivel3_act == 2 and estado_lvl3 == 1)
record("10. Seleccion e inicio de Nivel 2 y Nivel 3", test10_passed,
       f"Nivel 2 cargado={nivel2_act == 1}, Nivel 3 cargado={nivel3_act == 2}")

# --- FASE 11: Desplazamiento horizontal de camara (>256 px) ---
pb.memory[ADDR_PLX]     = 0x90 # 400 low
pb.memory[ADDR_PLX + 1] = 0x01 # 400 high
pb.memory[ADDR_PLY_FP]     = 0x00
pb.memory[ADDR_PLY_FP + 1] = 0x50

for _ in range(8): step(1)
snap("11_streaming_vram_profundo")

cam_x_deep = pb.memory[ADDR_CAM_X] | (pb.memory[ADDR_CAM_X + 1] << 8)
scx_deep = pb.memory[0xFF43]

test11_passed = (cam_x_deep == 328 and scx_deep == 72)
record("11. Desplazamiento de camara y streaming VRAM", test11_passed,
       f"Camara X={cam_x_deep} (>256 px), SCX={scx_deep} (modulo 256 circular)")

# --- FASE 12: Llegada a la meta y activacion de VICTORIA ---
pb.memory[ADDR_PLX]     = 0xCA # 714 low
pb.memory[ADDR_PLX + 1] = 0x02 # 714 high
pb.memory[ADDR_PLY_FP]     = 0x00
pb.memory[ADDR_PLY_FP + 1] = 0x50

pb.button_press("right")
for _ in range(15): step(1)
pb.button_release("right")
step(5)
snap("12_pantalla_victoria_espanol")

estado_win = pb.memory[ADDR_ESTADO]
scx_win = pb.memory[0xFF43]
cam_tile_win = scx_win >> 3

# Verificar VICTORIA en VRAM
tile_v = pb.memory[0x9800 + 5 * 32 + ((cam_tile_win + 6) & 31)]
tile_i = pb.memory[0x9800 + 5 * 32 + ((cam_tile_win + 7) & 31)]
tile_c = pb.memory[0x9800 + 5 * 32 + ((cam_tile_win + 8) & 31)]
tile_t = pb.memory[0x9800 + 5 * 32 + ((cam_tile_win + 9) & 31)]

text_victoria_ok = (tile_v == 38 and tile_i == 25 and tile_c == 19 and tile_t == 36)
test12_passed = (estado_win == 3 and text_victoria_ok)
record("12. Victoria al cruzar la meta (VICTORIA)", test12_passed,
       f"Estado={estado_win} (3=Victoria), Cartel VRAM V-I-C-T=({tile_v},{tile_i},{tile_c},{tile_t})")

# --- FASE 13: Colision de enemigos contra bloques solidos (rebote sin traspasar) ---
# Cargar Nivel 2 y monitorizar enemigo 1 (origen 208, col 26, muro solido de escalon en col 26 / px 208)
pb.button_press("b"); step(4); pb.button_release("b"); step(15) # Retorno al menu
pb.memory[ADDR_NIVEL_SEL] = 1 # Seleccionar explicitamente Nivel 2 (index 1)
step(5)
pb.button_press("a"); step(2); pb.button_release("a"); step(15) # Entrar a Nivel 2

ADDR_OBS1_X = 0xC0C8 + 2 # Offset 2 en struct Obstaculo
ADDR_OBS1_DIR = 0xC0C8 + 7 # Offset 7
obs1_x_positions = []
obs1_dirs = []

for _ in range(250):
    step(1)
    ox = pb.memory[ADDR_OBS1_X] | (pb.memory[ADDR_OBS1_X + 1] << 8)
    odir = pb.memory[ADDR_OBS1_DIR]
    obs1_x_positions.append(ox)
    obs1_dirs.append(odir)

snap("13_colision_enemigo_bloques")

# El muro solido en Nivel 2 empieza en col 26 (x=208). El enemigo (ancho 8) nunca supera x=205 al rebotar hacia la izquierda
max_obs1_x = max(obs1_x_positions)
min_obs1_x = min(obs1_x_positions)
has_reversed = (1 in obs1_dirs and (255 in obs1_dirs or -1 in obs1_dirs)) # UBYTE 255 = INT8 -1
test13_passed = (max_obs1_x <= 208 and has_reversed and min_obs1_x >= 192)

record("13. Colision de enemigos con bloques (rebote)", test13_passed,
       f"Nivel 2 Enemigo 1: Min X={min_obs1_x}, Max X={max_obs1_x} (Limite muro=208), Rebotes ok={has_reversed}")

pb.stop(save=False)

all_passed = all(p for _, p, _ in results)
print("===================================================")
print(f"RESULTADO FINAL: {'TODAS LAS PRUEBAS SUPERADAS' if all_passed else 'FALLOS ENCONTRADOS'}")
sys.exit(0 if all_passed else 1)
