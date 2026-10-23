# Quemador del cartucho FPGBC

Diseño para leer y quemar juegos en el cartucho **EverSave MBC5** usando la FPGA **GW2AR-18** de la consola FPGBC.

La FPGA carga un puente (`cart_bridge`) que recibe comandos del PC por UART y genera ciclos de bus de Game Boy en el conector del cartucho. El algoritmo de la flash S29GL064N (desbloqueo, borrado, programación y verificación) lo dirige el script de Python. La única excepción es la programación de cada byte, que el puente hace en hardware para ir más rápido.


```
PC (fpgbc_cart.py) --UART--> FPGA consola (cart_bridge) --conector 32 pines--> MBC5 + flash S29GL064N
```

Guía de diseño completa: <https://claude.ai/artifact/5pCVntXqpNy74bZwQhL8Vv>

## Contenido

El diseño sigue la metodología del curso: **caja negra → diagrama de flujo (ASM) → código por módulos**, separando camino de datos y unidad de control (como `mult_32` / `control_mult`). Los diagramas están en [`docs/diagramas.md`](docs/diagramas.md) (Mermaid, GitHub los dibuja) y exportados como imágenes en `docs/img/`.

| Ruta | Qué es |
| --- | --- |
| `rtl/cart_bridge.v` | Top: solo interconecta UART, FIFO, registros, contadores, multiplexores, `cart_bus` y `control_bridge` |
| `rtl/control_bridge.v` | Control del intérprete de comandos `V`, `R`, `W`, `P` (Moore, 36 estados) |
| `rtl/cart_bus.v` + `rtl/control_cart_bus.v` | Ciclo de lectura/escritura en el conector: camino de datos + control |
| `rtl/uart_rx.v` + `rtl/control_uart_rx.v` | Receptor UART 8N1: camino de datos + control |
| `rtl/uart_tx.v` + `rtl/control_uart_tx.v` | Transmisor UART 8N1: camino de datos + control |
| `rtl/datapath/` | Módulos del camino de datos: `registro`, `contador_dec`, `contador_inc`, `comparador`, `mux4`, `rsr_rx`, `rsr_tx`, `sincronizador`, `fifo_rx`, `deco_cmd`, `deco_arg`, `rom_version` |
| `constraints/fpgbc_cart.cst` | Pines de la GW2AR-LV18EQ144 (algunos son supuestos, ver abajo) |
| `sim/cart_model.v` | Modelo de MBC5 + flash para simulación |
| `sim/tb_cart_bridge.v` | Banco de pruebas (enlace, lectura, ID, borrado, programación, dato 0xF0, ráfaga) |
| `pc/fpgbc_cart.py` | Herramienta del PC: `info`, `header`, `dump`, `erase`, `burn`, `verify` |
| `pc/prueba_sin_hardware.py` | Ejecuta `fpgbc_cart.py` contra un puente falso, sin consola |
| `docs/diagramas.md`, `docs/img/` | Cajas negras, diagramas de bloques y ASM de cada unidad de control |

Al simular con `make sim` se define `BENCH`, así que en gtkwave cada control muestra la señal `state_name` con el nombre del estado.

## Pines supuestos (CONFIRMAR antes de cargar)

| Señal | Pin FPGA | Estado |
| --- | --- | --- |
| `data_dir`: DIR del SN74LVC8T245 (chip LJ24SA, pin 2) | 46 (IOB14A) | **Supuesto**: 46, 47 y 49 son los pines libres junto al bus de datos |
| `cart_rst_n`: /RESET, pin 30 del conector | 49 (IOB17B) | **Supuesto**: el EverSave no lo usa (NC), así que no es crítico |
| `DIR_WRITE`: nivel de DIR para escribir hacia el cartucho | `1` | **Supuesto**: correcto si el lado A del 8T245 es la FPGA |
| A14 / A15 | 83 (IOR42B) / 84 (IOR42A) | El CSV del repositorio repite IOR42A; la base de datos de pines de la herramienta (apicula) da IOR42B para el 83 |

Cómo confirmarlos, con la consola **apagada** y el multímetro en continuidad:

1. Pin 2 (DIR) del chip LJ24SA → pines 46, 47 y 49 de la FPGA.
2. Pin 22 (/OE) del mismo chip → GND o algún pin de la FPGA. Si va a un pin, hay que agregarlo al diseño y ponerlo en 0.
3. Pines 3–10 (lado A) del chip: ¿van a la FPGA o al conector? Si van al conector, cambien `DIR_WRITE` a `1'b0` en `rtl/cart_bridge.v`.
4. Pin 30 del conector → 49 (o el que sea).

Si DIR resulta ser otro pin, solo cambien la línea `IO_LOC "data_dir" 46;` del `.cst`.

## Instalación del entorno

Las herramientas son las mismas de [FPGBC_digital_UN](https://github.com/digital-un-2026-2/FPGBC_digital_UN) (Yosys, nextpnr-gowin, apicula, openFPGALoader, iverilog), instaladas con **pixi**. El `pixi.toml` de ese repositorio solo soporta **Linux x86-64**.

### Si usan Windows: WSL2

1. En PowerShell como administrador: `wsl --install -d Ubuntu`, y reiniciar.
2. Abrir "Ubuntu" y seguir los pasos de Linux de abajo.
3. Para que WSL vea el cable USB de la FPGA: instalar [usbipd-win](https://github.com/dorssel/usbipd-win). Luego, en PowerShell como administrador: `usbipd list`, `usbipd bind --busid <id>` y `usbipd attach --wsl --busid <id>`.
4. El script de Python (`pc/fpgbc_cart.py`) también funciona directamente en Windows, con el puerto `COM3` (o el que aparezca en el Administrador de dispositivos).

### Linux (o dentro de WSL)

```bash
# 1. Instalar pixi
curl -fsSL https://pixi.sh/install.sh | sh
exec $SHELL            # recargar el PATH

# 2. Clonar los dos repositorios, uno al lado del otro
git clone https://github.com/digital-un-2026-2/FPGBC_digital_UN.git
git clone https://github.com/digital-un-2026-2/FPGBC_juego.git

# 3. Instalar el entorno (tarda varios minutos la primera vez)
cd FPGBC_digital_UN
pixi install
pixi run setup-env     # descarga el toolchain RISC-V y pide sudo para las reglas USB del FT232
pixi run make -C test/env/min_soc detect   # opcional: ver si se detecta la FPGA

# 4. Herramientas extra para este diseño
pixi shell             # entra al entorno; de aquí en adelante todo se ejecuta dentro
pip install pyserial
```

Comprobar que todo quedó instalado:

```bash
cd ../FPGBC_juego/quemador
make sim               # debe terminar con "RESULTADO: todas las pruebas pasaron"
make build             # genera build/cart_bridge.fs
python pc/prueba_sin_hardware.py ../ejemplo.gb   # prueba el script sin consola
```

## Uso con la consola

Conexiones:

- **JTAG** (para cargar el bitstream): el FT232RL del curso, como en `min_soc` (`--pins TXD:CTS:DTR:RXD`).
- **UART** (para los comandos): adaptador USB-serial de **3.3 V** al puerto link. El TX del adaptador va al pin 27 de la FPGA (Serial In), el RX del adaptador al pin 28 (Serial Out), y GND común. 115200 baudios.

```bash
make load                                          # carga el puente en la SRAM de la FPGA (se pierde al apagar)
python pc/fpgbc_cart.py --port /dev/ttyUSB1 info   # debe decir "Puente: FPGBC1"
python pc/fpgbc_cart.py --port /dev/ttyUSB1 header # PRIMER HITO: leer la cabecera del juego que tenga el cartucho
```

Orden de pruebas recomendado, de menor a mayor riesgo:

1. `header`: solo lee. Si el logo sale "correcto", el bus de lectura funciona.
2. `dump copia.gb --size 32768`: guarda una copia de lo que tenga el cartucho **antes** de borrar nada.
3. `info`: entra al modo ID de la flash. Debe responder fabricante `0x01` y dispositivo `0x7e`. Si en vez de eso se ve contenido normal de la ROM, las escrituras no están llegando: revisen DIR y `DIR_WRITE`.
4. `burn ../build/juego_completo.gb`: borra los sectores necesarios, programa y verifica.
5. Volver a cargar el núcleo de juego de la consola y probar el cartucho.

## Protocolo serial

| Comando | Bytes que envía el PC | Respuesta |
| --- | --- | --- |
| `V` | `V` | `FPGBC1` |
| `R` | `R` aH aL nH nL | n bytes leídos desde la dirección a (n = 0 → 65536) |
| `W` | `W` aH aL d | `K` |
| `P` | `P` aH aL d | `K` si se programó, `X` si falló (el PC envía luego `F0` para resetear la flash) |

El banco del MBC5 lo elige el PC con `W 0x2000` (bits 0–7) y `W 0x3000` (bit 8). Todo se programa por la ventana `0x4000–0x7FFF`, así que la dirección física es `banco * 0x4000 + (a & 0x3FFF)`.

## Limitaciones conocidas

- La frecuencia del reloj del pin 11 se supuso de 40 MHz, como en `min_soc`. Si la UART recibe basura, cambien `CLK_HZ` en `cart_bridge.v` por la frecuencia real.
- El modelo de simulación no reproduce los tiempos reales de la flash ni de los level shifters. Los tiempos del bus (200 ns de preparación y 500 ns de pulso) son conservadores.
- Si el MBC5 de fábrica del cartucho no habilita la flash o el transceptor durante las escrituras, el paso `info` lo mostrará. En ese caso habría que programar un MBC5 propio en la GW1NZ del cartucho (coordinar con el grupo del cartucho).
