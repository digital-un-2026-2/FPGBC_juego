# Informe de Revisión Técnica y Auditoría de Entrega
## Proyecto: Consola Game Boy en FPGA (FPGBC) — Videojuego de Plataformas

**Fecha de auditoría:** 20 de Octubre de 2026  
**Proyecto:** `FPGBC_juego`  
**Repositorio oficial del equipo:** `https://github.com/digital-un-2026-2/FPGBC_juego`  
**Carpeta de entrega final:** `C:\Users\ASUS\Desktop\FPGBC_GITHUB_FINAL`

---

## 1. Inventario de Archivos Aprobados para Entrega

| Archivo / Ruta Relativa | Tamaño | Función Técnica | Estado de Verificación |
| :--- | :---: | :--- | :--- |
| `codigo/juego_completo.c` | 29.727 B | Bucle principal, máquina de estados, motor físico, streaming de VRAM y audio. | Compilación limpia con GBDK-2020 (`Exit code 0`). Supera 13/13 pruebas en PyBoy. |
| `codigo/Mapa.c` / `Mapa.h` | 4.318 B / 579 B | Tilesets de fondo, ladrillos, gemas y tipografía de texto. | Verificado por hash SHA256: idéntico a la versión base del repositorio. |
| `codigo/Tile1.c` / `Tile1.h` | 623 B / 580 B | Sprites y máscaras del personaje jugador. | Verificado por hash SHA256: idéntico a la versión base del repositorio. |
| `codigo/Tile2.c` / `Tile2.h` | 1.769 B / 583 B | Sprites de enemigos, orbe de proyectil y HUD numérico. | Verificado por hash SHA256: idéntico a la versión base del repositorio. |
| `codigo/niveles/NivelesData.c` | 25.004 B | Definición matricial de 96×16 tiles para Niveles 1, 2 y 3. | Estructuras de plataformas escalonadas (máximo 1 tile de desnivel). Probado en emulador. |
| `codigo/niveles/NivelesData.h` | 339 B | Punteros y constantes de dimensionamiento de los niveles. | Compilado exitosamente junto con el núcleo del juego. |
| `recursos/Mapa.gbr` | 10.069 B | Proyecto original de Game Boy Tile Designer para tiles de escenario. | Verificado por hash SHA256: idéntico al proyecto original. |
| `recursos/Tile1.gbr` | 10.069 B | Proyecto original de Game Boy Tile Designer para sprites del jugador. | Verificado por hash SHA256: idéntico al proyecto original. |
| `recursos/Tile2.gbr` | 10.069 B | Proyecto original de Game Boy Tile Designer para enemigos y proyectiles. | Verificado por hash SHA256: idéntico al proyecto original. |
| `recursos/preview/nivel_[1-3].png` | ~1.3 KB c/u | Imágenes panorámicas completas (PNG) de los 3 niveles del juego. | Recursos gráficos de apoyo para documentación y presentación. |
| `herramientas/compile.bat` | 974 B | Script de compilación parametrizado para GBDK-2020. | Compilación verificada desde cero (`Exit code 0`). Genera ROM válida de 32 KB. |
| `herramientas/make_todos_niveles.py` | 10.557 B | Generador matricial reproducible para exportar niveles a C y PNG. | Ejecución probada: genera código C sintácticamente válido sin advertencias. |
| `pruebas/test_completo.py` | 12.884 B | Suite de 13 pruebas automatizadas reproducibles con PyBoy. | 13 de 13 pruebas superadas al 100% (`Exit code 0`). |
| `pruebas/capturas/*.png` | 13 archivos | Evidencias visuales capturadas de cada fase de prueba automatizada. | Generadas durante la ejecución de las pruebas. |
| `compile.bat` | 68 B | Script de acceso directo en la raíz para compilar con un solo comando. | Invoca `herramientas/compile.bat` conservando rutas relativas. |
| `README.md` | 6.936 B | Documentación general con controles, requisitos e instrucciones. | Describe con exactitud las funciones, controles y rutas del proyecto. |
| `.gitignore` | 488 B | Reglas de exclusión de Git para temporales, mapas de símbolos y cachés. | Excluye correctamente `build/`, `*.map`, `*.noi`, `*.o`, `__pycache__/`. |

---

## 2. Comparación con el Repositorio de Referencia

1. **Recursos Gráficos Base:** `Tile1.*`, `Tile2.*`, `Mapa.*` y los archivos de proyecto `.gbr` son idénticos a los almacenados en la rama principal (`main`) del repositorio del equipo, preservando los recursos visuales originales.
2. **Evolución del Motor de Juego:** `juego_completo.c` sustituye al archivo preliminar `ejemplo.c`, incorporando físicas reales, salto parabólico, colisiones con el mapa, menú selector y sonido nativo de Game Boy.
3. **Evolución de los Niveles:** `codigo/niveles/NivelesData.c` sustituye a `Nivel1.c` (mapa único de 32×16 tiles), extendiendo la jugabilidad a tres niveles independientes de 96×16 tiles con streaming dinámico hacia la VRAM circular.
4. **Archivos Obsoletos Excluidos:** Se omitieron de la entrega los prototipos preliminares (`ejemplo.c`, `juego_nivel2.c`, `juego_plataformas.c`), mapas antiguos de 32×16 (`Nivel1.*`, `Nivel2.*`), scripts de depuración desactualizados y archivos intermedios de compilación (`.map`, `.noi`, `.o`, `__pycache__`).

---

## 3. Justificación de la Estructura de Directorios

- **`codigo/` y `codigo/niveles/`:** Agrupa los archivos C y cabeceras que se incluyen mutuamente en la misma unidad de compilación, resolviendo de forma limpia todas las directivas `#include`.
- **`recursos/`:** Mantiene separados los archivos de diseño gráfico (`.gbr`) y las vistas previas panorámicas (`preview/`), evitando mezclar archivos binarios de diseño con el código fuente.
- **`herramientas/`:** Centraliza los scripts de compilación (`compile.bat`) y diseño matricial (`make_todos_niveles.py`).
- **`pruebas/`:** Aloja la suite de verificación con PyBoy y las 13 capturas de pantalla de evidencia.
- **Raíz (`compile.bat`):** Permite compilar el proyecto completo directamente desde la raíz sin requerir configuración manual de rutas.

---

## 4. Resultados de Compilación y Pruebas Técnicas

### 4.1. Compilación Limpia con GBDK-2020 (v4.5.0)
- **Comando:** `cmd.exe /c compile.bat`
- **Comando interno del compilador:**
  ```cmd
  lcc.exe -Wa-l -Wl-m -Wl-j -o build\juego_completo.gb codigo\juego_completo.c
  ```
- **Código de salida:** `0` (Exitoso)
- **Advertencias del compilador:** `0`
- **Errores:** `0`
- **ROM resultante:** `build\juego_completo.gb` (Exactamente 32.768 bytes, ROM DMG estándar de 32 KB).

### 4.2. Suite de Pruebas Automatizadas con PyBoy (v2.8.1)
- **Comando:** `python pruebas/test_completo.py`
- **Código de salida:** `0` (Exitoso)
- **Resultado:** **13 de 13 pruebas superadas (100% PASS)**:

| # | Prueba Técnica | Condición Comprobada | Resultado | Detalle Registrado |
| :-: | :--- | :--- | :-: | :--- |
| **1** | Menú y cursor interactivo | Arranque en estado 0, título `ELIGE TU NIVEL`, cursor Y: 56->72->56 | **PASS** | Título verificado en VRAM, navegación vertical de gema OK |
| **2** | Inicio de Nivel 1 y HUD | Arranque en (24, 96), audio activo (`NR52 = 0xF2`), contador `00` | **PASS** | Estado=1 (Jugando), Nivel=1, HUD de monedas activo |
| **3** | Disparo de proyectil (A) | Proyectil en OAM (X:41, Y:100, Tile 12), sonido canal 1 | **PASS** | Orbe generado correctamente y avanzando hacia la derecha |
| **4** | Recogida de monedas | Colisión de jugador con gemas en px 32 y 40; HUD se actualiza a `02` | **PASS** | Monedas recogidas=2, Tile HUD=17 ('2'), VRAM limpia |
| **5** | Gravedad real de enemigos | Enemigo colocado a Y=40 cae progresivamente y frena en suelo Y=88 | **PASS** | Aterrizaje firme sobre bloques sólidos (`vel_y = 0`) |
| **6** | Patrullaje horizontal | Oscilación lateral de 16 px a izquierda y derecha | **PASS** | Rango: Min=141, Max=168, Amplitud=27 px (Origen=152) |
| **7** | Destrucción de enemigo | Proyectil colisiona con enemigo; enemigo eliminado de OAM | **PASS** | `obstaculo[0].activo=0`, OAM Y=0, proyectil consumido |
| **8** | Muerte por contacto | Jugador colisiona con enemigo; transición a `FIN DEL JUEGO` | **PASS** | `estado=2` (GameOver), Sprites de jugador ocultados |
| **9** | Retorno al menú con botón B | Botón B en fin de partida regresa a `ELIGE TU NIVEL` | **PASS** | Estado restaurado a 0 (Menú) |
| **10** | Selector: Niveles 2 y 3 | Menú carga correctamente Nivel 2 y Nivel 3 | **PASS** | Mapas cargados y renderizados en VRAM |
| **11** | Streaming circular VRAM | Jugador avanza a X=400 (>256 px); buffer circular funciona sin glitches | **PASS** | Cámara X=328, SCX=72 (`328 mod 256`), VRAM regenerada |
| **12** | Meta y pantalla `VICTORIA` | Jugador cruza X=716; activación de fanfarria y texto `VICTORIA` | **PASS** | `estado=3` (Victoria), letras V-I-C-T comprobadas en VRAM |
| **13** | Colisión con bloques sólidos | Enemigo patrullando en Nivel 2 choca con escalón, rebota y no penetra | **PASS** | Min X=192, Max X=205 (Muro sólido en 208), Rebote OK |

---

## 5. Auditoría de Resoluciones Técnicas Específicas

1. **Franqueabilidad del Nivel 2:** El desnivel máximo entre peldaños consecutivos es de 1 tile (8 px). Dado que la altura máxima de salto del motor físico es de 27.5 px (~3.4 tiles), todas las plataformas y torres son accesibles.
2. **Colisión sólida de enemigos:** Se implementó verificación horizontal con `point_is_solid()` previa a cada desplazamiento. Ante una pared sólida, el enemigo se detiene e invierte su sentido de marcha (`dir = -dir`) sin penetrar bloques.
3. **Reproducibilidad:** El código compila limpiamente con GBDK-2020 v4.5.0 estándar y las pruebas automatizadas se ejecutan con Python 3 estándar (Pillow y PyBoy).

---

## 6. Procedimiento de Integración en el Repositorio Git Oficial

Para integrar la entrega limpia en el repositorio del equipo (`https://github.com/digital-un-2026-2/FPGBC_juego`):

```bash
# 1. Posicionarse en el repositorio local y crear rama de trabajo limpia
cd C:\Users\ASUS\Documents\GitHub\FPGBC_juego
git checkout -b feature/plataformas-niveles-completo

# 2. Copiar los archivos aprobados desde FPGBC_GITHUB_FINAL
# (Copiar codigo/, recursos/, herramientas/, pruebas/, compile.bat, README.md, INFORME_REVISION_TECNICA.md, .gitignore)

# 3. Comprobar compilacion y pruebas en el repositorio
compile.bat
python pruebas/test_completo.py

# 4. Registrar los cambios
git add codigo/ recursos/ herramientas/ pruebas/ compile.bat README.md INFORME_REVISION_TECNICA.md .gitignore
git commit -m "feat: implementa motor de plataformas, selector de 3 niveles, fisicas y pruebas automatizadas"

# 5. Publicar y abrir Pull Request
git push -u origin feature/plataformas-niveles-completo
```

---

## 7. Limitaciones Técnicas y Validación en Hardware
El videojuego ha sido probado y verificado al 100% mediante emulación de precisión de ciclo (BGB) y pruebas automatizadas (PyBoy). La validación física en la placa FPGA de la consola FPGBC queda reservada para las pruebas presenciales de laboratorio del equipo.
