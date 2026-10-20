# Videojuego de Plataformas para Game Boy (Proyecto FPGBC)

Videojuego de plataformas en 2D desarrollado en **C** con la suite de desarrollo **GBDK-2020**, diseñado para su ejecución en hardware de Game Boy / Game Boy Color y consolas de hardware abierto implementadas sobre **FPGA** (proyecto universitario FPGBC).

---

## 1. Características Técnicas Implementadas y Verificadas

- **Selector de Nivel Interactivo:** Menú principal con interfaz completa en español (`ELIGE TU NIVEL`), selector visual con gema y soporte para 3 niveles distintos.
- **Arquitectura de 3 Niveles Extensibles (96×16 tiles / 768×128 px):**
  - *Nivel 1 (Pradera e Islas):* Plataformas escalonadas, saltos de aprendizaje, abismos y patrullas enemigas.
  - *Nivel 2 (Fortaleza y Torres):* Estructuras escalonadas con desniveles suaves de máximo 1 tile (8 px) franqueables mediante salto.
  - *Nivel 3 (Desafío de Abismos y Precisión):* Plataformas intermedias sobre el vacío que requieren saltos precisos.
- **Motor Físico:** Gravedad progresiva con aritmética de punto fijo subpíxel (`GRAVITY_SUBPX = 36`), impulso de salto parabólico (`JUMP_IMPULSE = -730`), detección y resolución de colisiones AABB contra el mapa de fondo.
- **Inteligencia y Físicas Enemigas:**
  - Los enemigos experimentan **gravedad real** y aterrizan en plataformas sólidas (no flotan en el vacío).
  - **Colisión frontal con bloques:** Comprobación de solidez previa al desplazamiento; los enemigos rebotan al encontrar muros o escalones sin traspasarlos.
  - Patrullaje horizontal delimitado de 2 pasos por lado.
- **Mecánica de Combate:** Disparo de orbe de energía (botón A) capaz de destruir enemigos en pantalla.
- **Coleccionables y HUD:** Gemas coleccionables registradas mediante máscara de bits en WRAM (192 bytes, impidiendo que reaparezcan al desplazar la cámara) y contador gráfico superior `[Gema] x [00]`.
- **Motor de Audio Chiptune Nativo:**
  - Canal 1 (Pulso con barrido): Efecto de salto y tono descendente de muerte.
  - Canal 2 (Pulso agudo): Campanilla de recogida de monedas.
  - Canal 4 (Ruido blanco): Explosión de impacto y derrota de enemigos.
  - Fanfarria triunfal de victoria al cruzar la meta.
- **Control de Estados y Flujo:** Pantallas de `FIN DEL JUEGO` y `VICTORIA`, reinicio de nivel con `START` y retorno al menú principal con botón `B`.

---

## 2. Estructura del Proyecto

```text
FPGBC_GITHUB_FINAL/
├── README.md                           # Documentación general del proyecto
├── INFORME_REVISION_TECNICA.md         # Auditoría técnica e inventario de entrega
├── compile.bat                         # Acceso directo para compilar el juego
├── .gitignore                          # Configuración de exclusiones para Git
├── codigo/                             # Código fuente C y cabeceras aprobadas
│   ├── juego_completo.c                # Núcleo y bucle principal del videojuego
│   ├── Mapa.c / Mapa.h                 # Definición de tilesets del escenario y tipografía
│   ├── Tile1.c / Tile1.h               # Sprites del personaje principal
│   ├── Tile2.c / Tile2.h               # Sprites de enemigos, proyectiles y marcadores
│   └── niveles/
│       ├── NivelesData.c               # Arreglos de datos de los 3 niveles (96x16 tiles)
│       └── NivelesData.h               # Cabeceras y constantes de niveles
├── recursos/                           # Recursos gráficos originales y vistas previas
│   ├── Mapa.gbr, Tile1.gbr, Tile2.gbr  # Proyectos originales de Game Boy Tile Designer
│   └── preview/                        # Vistas panorámicas completas (PNG)
│       ├── nivel_1.png
│       ├── nivel_2.png
│       └── nivel_3.png
├── herramientas/                       # Scripts de soporte y generación
│   ├── compile.bat                     # Script de compilación parametrizado con GBDK
│   └── make_todos_niveles.py           # Generador matricial de los 3 niveles
└── pruebas/                            # Batería de pruebas automatizadas y evidencias
    ├── test_completo.py                # Suite de 13 pruebas automatizadas con PyBoy
    └── capturas/                       # 13 capturas de pantalla de los estados verificados
```

---

## 3. Requisitos del Entorno

1. **Compilador C:** GBDK-2020 versión 4.5.0 (o superior).
   - Variable de entorno `GBDK` o `GBDK_HOME` apuntando al directorio base de GBDK (por defecto `C:\Users\ASUS\tools\gbdk-2020-4.5.0\gbdk`).
2. **Entorno de Pruebas Automatizadas (opcional):** Python 3.10+ con `PyBoy` y `Pillow`.
3. **Emulador de Referencia:** BGB (v1.5.8 o superior) o emulador compatible con Game Boy DMG.

---

## 4. Instrucciones de Compilación

Para compilar la ROM desde la raíz del proyecto, ejecute en una consola de comandos (o PowerShell):

```cmd
compile.bat
```

El script invocará automáticamente `lcc.exe` con las directivas de ensamblador y enlazador optimizadas:
```cmd
lcc -Wa-l -Wl-m -Wl-j -o build\juego_completo.gb codigo\juego_completo.c
```
La ROM resultante se ubicará en `build\juego_completo.gb` (32.768 bytes, ROM MBC1/None estándar de 32 KB).

---

## 5. Controles del Videojuego

| Acción | Control Game Boy | Tecla en BGB (Mapeo Estándar) |
| :--- | :--- | :--- |
| **Menú: Cambiar selección de nivel** | D-Pad Arriba / Abajo | Flechas Arriba / Abajo |
| **Menú: Iniciar nivel seleccionado** | Botón A / START | Tecla `S` / `Z` / `Enter` |
| **Juego: Mover personaje** | D-Pad Izquierda / Derecha | Flechas Izquierda / Derecha |
| **Juego: Saltar** | Botón B / D-Pad Arriba | Tecla `A` / `X` / Flecha Arriba |
| **Juego: Disparar proyectil** | Botón A | Tecla `S` / `Z` |
| **Fin de Partida: Reintentar nivel** | START | Tecla `Enter` |
| **Fin de Partida: Volver al menú** | Botón B / SELECT | Tecla `A` / `X` / Barra Espaciadora |

---

## 6. Verificación de Calidad

El proyecto cuenta con una suite de pruebas automatizadas reproducibles (`pruebas/test_completo.py`) que ejecuta el binario sobre el emulador `PyBoy` sin intervención manual, validando:
- Arranque del menú y navegación del cursor.
- Carga de los 3 niveles y streaming circular de VRAM (>256 px).
- Gravedad, salto y colisión con plataformas.
- Patrullaje de enemigos y rebote sin atravesar bloques.
- Sistema de disparo, impacto de proyectil y recolección de monedas.
- Condiciones de derrota y victoria.

Para ejecutar las pruebas:
```cmd
python pruebas/test_completo.py
```
Resultado verificado: **13/13 pruebas aprobadas (100% PASS)**.

---

## 7. Limitaciones y Validación en Hardware
El videojuego ha sido probado y verificado al 100% mediante emulación de precisión de ciclo (BGB) y pruebas automatizadas (PyBoy). La validación física en la placa FPGA de la consola FPGBC queda reservada para las pruebas presenciales de laboratorio del equipo.
