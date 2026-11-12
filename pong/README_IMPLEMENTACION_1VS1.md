# PONG – Implementación del modo multijugador local 1 vs. 1

## 1. Descripción del proyecto

El objetivo de esta modificación es ampliar el videojuego Pong existente para incorporar un modo multijugador local, en el que dos personas puedan competir utilizando una sola consola Game Boy (FPGBC).

La implementación se realizará sobre el código fuente existente, conservando las mecánicas, los gráficos y las funcionalidades actuales del juego.

Se busca disponer de dos modalidades:

- **1 jugador:** el usuario compite contra la CPU.
- **2 jugadores:** dos usuarios controlan simultáneamente las raquetas desde una misma consola.

Inicialmente, los cambios se desarrollarán y probarán en el simulador de PC. Una vez validado el funcionamiento, se realizarán los ajustes necesarios para llevar el modo multijugador a la FPGBC.

## 2. Funcionamiento propuesto

### 2.1. Menú principal

Se incorporará una pantalla inicial que permita seleccionar la modalidad de juego:

- 1 JUGADOR
- 2 JUGADORES

El usuario podrá desplazarse entre las opciones y confirmar la selección.

### 2.2. Modo de un jugador

Se conservará el funcionamiento original del videojuego:

- El jugador controla la raqueta izquierda.
- La raqueta derecha es controlada por la CPU.
- Se mantienen los niveles de dificultad existentes.
- Se conserva el sistema de puntuación.
- Se mantienen las mecánicas de saque, colisiones, pausa y finalización de partida.

La incorporación del nuevo modo no debe alterar el comportamiento de esta modalidad.

### 2.3. Modo de dos jugadores

En esta modalidad, ambos participantes jugarán en la misma pantalla:

- Jugador 1: controla la raqueta izquierda.
- Jugador 2: controla la raqueta derecha.
- La CPU queda desactivada.
- Ambos jugadores pueden mover sus raquetas durante el mismo ciclo de ejecución.
- Se conserva la física de la pelota y el sistema de colisiones.
- El primer jugador que alcance 7 puntos será declarado ganador.

No se requiere comunicación entre dos consolas ni utilización de cable Link.

## 3. Distribución de controles

### 3.1. Simulador de PC

Para facilitar las pruebas iniciales se propone la siguiente distribución:

| Acción | Jugador 1 | Jugador 2 |
|---|---|---|
| Subir | W | Flecha arriba |
| Bajar | S | Flecha abajo |

Controles generales:

- ENTER: iniciar el saque.
- ESC: pausar o regresar al menú, según el estado del juego.

### 3.2. Consola FPGBC

Para la implementación en hardware se propone:

| Acción | Jugador 1 | Jugador 2 |
|---|---|---|
| Subir | Cruceta arriba | Botón A |
| Bajar | Cruceta abajo | Botón B |

Controles generales:

- START: iniciar el saque.
- START: controlar la pausa.

Esta distribución deberá validarse teniendo en cuenta la lectura simultánea de entradas y la asignación física de los botones.

## 4. Archivos que se deben revisar y modificar

La implementación debe aprovechar la arquitectura actual del proyecto.

| Archivo | Modificación prevista |
|---|---|
| `main.c` | Revisar la inicialización y el ciclo principal del juego. |
| `game.c` | Incorporar el menú, la selección de modalidad y la lógica de dos jugadores. |
| `game.h` | Añadir las declaraciones, variables y estructuras necesarias para las nuevas funcionalidades. |
| `hal.h` | Revisar la interfaz de lectura de controles. |
| `hal_pc.c` | Implementar la lectura de controles independientes para ambos jugadores en PC. |
| `hal_fpgbc.c` | Adaptar la lectura de botones físicos para la modalidad de dos jugadores. |
| `tiles.c` | Incorporar o modificar los gráficos necesarios para el menú. |
| `tiles.h` | Actualizar las declaraciones de los recursos gráficos, si es necesario. |
| `Makefile` | Verificar la compilación del simulador en PC. |
| `Makefile.fpgbc` | Verificar la compilación para la plataforma FPGBC. |

No es obligatorio modificar todos estos archivos. Los cambios deberán limitarse a los componentes que realmente lo requieran.

## 5. Estrategia de implementación

El desarrollo se organizará en las siguientes etapas:

### Etapa 1. Análisis del código existente

- Revisar la arquitectura del proyecto.
- Identificar cómo se controlan actualmente las raquetas.
- Identificar cómo se detectan las entradas del teclado y los botones.
- Comprender la máquina de estados del juego.
- Determinar cómo se implementa el control de la CPU.

### Etapa 2. Incorporación del menú

- Crear la pantalla de selección de modalidad.
- Incorporar las opciones de uno y dos jugadores.
- Implementar la navegación y confirmación.
- Conservar las pantallas y transiciones existentes.

### Etapa 3. Implementación del modo 1 vs. 1

- Incorporar el control independiente de la segunda raqueta.
- Desactivar el movimiento automático de la CPU.
- Permitir que ambos jugadores muevan sus raquetas simultáneamente.
- Conservar las reglas, colisiones y puntuaciones originales.

### Etapa 4. Adaptación de controles

- Implementar los controles propuestos para PC.
- Adaptar la lectura de botones en la FPGBC.
- Comprobar que las entradas de ambos jugadores no interfieran entre sí.
- Revisar las funciones de saque y pausa.

### Etapa 5. Pruebas en PC

Utilizar el simulador existente para comprobar:

1. Navegación del menú principal.
2. Selección del modo de un jugador.
3. Funcionamiento de la CPU.
4. Selección del modo de dos jugadores.
5. Movimiento independiente de ambas raquetas.
6. Detección de colisiones.
7. Actualización correcta de los marcadores.
8. Determinación del ganador.
9. Pausa y reinicio de partida.
10. Conservación del funcionamiento original.

### Etapa 6. Adaptación y validación en FPGBC

Una vez superadas las pruebas en PC:

- Adaptar los cambios a la interfaz de hardware.
- Revisar la disponibilidad de los botones físicos.
- Compilar el proyecto para la plataforma FPGBC.
- Generar la ROM correspondiente.
- Grabar el juego en el cartucho.
- Validar el funcionamiento directamente en la consola.

## 6. Consideraciones técnicas

- Se debe conservar una copia de seguridad del proyecto original.
- No se debe reescribir innecesariamente la lógica existente.
- La lógica del juego debe mantenerse independiente de la plataforma.
- La lectura de controles debe gestionarse desde las interfaces HAL correspondientes.
- No se deben eliminar las dificultades del modo individual.
- La física de la pelota debe permanecer sin cambios, salvo que las pruebas identifiquen algún problema.
- La versión de PC no debe considerarse una validación definitiva del hardware.
- No se debe afirmar que el modo multijugador está terminado en FPGBC hasta realizar pruebas en la consola.

## 7. Estado inicial del desarrollo

| Componente | Estado |
|---|---|
| Juego Pong original | Disponible |
| Modo de un jugador | Implementado |
| Control de CPU | Implementado |
| Menú de selección de jugadores | Pendiente |
| Modo local 1 vs. 1 | Pendiente |
| Controles independientes en PC | Pendiente |
| Adaptación de controles FPGBC | Pendiente |
| Compilación de la nueva versión | Pendiente |
| Pruebas en PC | Pendiente |
| Pruebas en hardware | Pendiente |

## 8. Resultados esperados

Al finalizar esta modificación se espera disponer de una versión de Pong que permita seleccionar entre uno y dos jugadores desde un menú principal.

En el modo multijugador local, ambos participantes podrán competir utilizando una sola consola, con controles independientes, manteniendo las mecánicas originales del videojuego.

Este documento corresponde a la planificación inicial. Se actualizará posteriormente con los cambios realmente realizados, los archivos modificados, las decisiones de implementación, los resultados de compilación y las pruebas efectuadas.
