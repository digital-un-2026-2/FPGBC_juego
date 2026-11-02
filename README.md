# FPGBC_juego

autores: David Rico, Daniel Suarez Roa, Sebastian Quejada

### Desarrollo de videojuegos para Game Boy / Game Boy Color mediante FPGA

Repositorio del proyecto académico **FPGBC**, orientado al desarrollo de videojuegos y su integración con una plataforma basada en FPGA, tomando como referencia la arquitectura de las consolas Game Boy y Game Boy Color.

El proyecto reúne el desarrollo del videojuego, sus recursos gráficos, herramientas de compilación, pruebas y documentación técnica, además del sistema de lectura y programación del cartucho EverSave MBC5.

---

## Descripción del proyecto

El objetivo es desarrollar un videojuego compatible con la arquitectura Game Boy, utilizando lenguaje C y herramientas de desarrollo como GBDK-2020.

El trabajo contempla tanto el desarrollo del software del juego como su integración con el hardware del proyecto FPGBC. Para ello, se trabaja con herramientas de compilación, emulación, diseño de recursos gráficos y programación de memorias externas.

Adicionalmente, se incorpora un sistema de comunicación entre el computador y la FPGA de la consola, destinado a leer, programar y verificar juegos en el cartucho EverSave MBC5.

## Componentes del proyecto

| Componente | Descripción |
|---|---|
| Desarrollo del videojuego | Programación de niveles, mecánicas, interacciones y lógica del juego. |
| Recursos gráficos | Diseño y administración de sprites, mapas, tiles y elementos visuales. |
| Compilación | Generación de archivos ROM mediante GBDK-2020. |
| Pruebas | Verificación del comportamiento del videojuego mediante herramientas de desarrollo y emulación. |
| Quemador del cartucho | Sistema de comunicación PC–FPGA para lectura, programación y verificación de la memoria Flash. |
| Documentación técnica | Registro de la arquitectura, decisiones de diseño, pruebas y características del hardware. |

## Arquitectura general

El flujo de desarrollo e integración contempla las siguientes etapas:

```text
       DESARROLLO DEL VIDEOJUEGO
                  |
                  v
        Código fuente en C
                  |
                  v
           GBDK-2020
                  |
                  v
             Archivo ROM
                (.gb)
                  |
          +-------+-------+
          |               |
          v               v
     EMULACIÓN       SISTEMA DE
      Y PRUEBAS       QUEMADO
                          |
                          v
                     Comunicación
                         UART
                          |
                          v
                    FPGA FPGBC
                          |
                          v
                   Cartucho MBC5
                          |
                          v
                   Memoria Flash
```

El diagrama representa el flujo previsto de desarrollo e integración. La grabación y ejecución en el hardware físico están sujetas a las pruebas de validación correspondientes.

## Estructura del repositorio

El repositorio está organizado en directorios que separan el desarrollo del juego, las herramientas y la documentación.

| Directorio / archivo | Contenido |
|---|---|
| `Quemador/` | Diseño Verilog y herramientas Python para la lectura y programación del cartucho. |
| `codigo/` | Código fuente y componentes del desarrollo del videojuego. |
| `herramientas/` | Herramientas y recursos de apoyo al desarrollo. |
| `pruebas/` | Archivos y material destinado a pruebas del proyecto. |
| `recursos/` | Recursos complementarios del videojuego. |
| `INFORME_REVISION_TECNICA.md` | Documentación de revisión técnica. |
| `INVESTIGACION_Y_ESTADO_DEL_JUEGO` | Investigación y seguimiento del desarrollo. |
| `Mapa.c`, `Mapa.gbr`, `Mapa.h` | Archivos relacionados con los mapas del videojuego. |
| `Nivel1.c`, `Nivel1.gbm`, `Nivel1.h` | Archivos relacionados con el primer nivel. |
| `Nivel2.c`, `Nivel2.gbm`, `Nivel2.h` | Archivos relacionados con el segundo nivel. |
| `compile.bat` | Script de compilación para el entorno configurado. |
| `ejemplo.c`, `ejemplo.gbm`, `ejemplo.map`, `ejemplo.rom` | Archivos de ejemplo y recursos asociados. |

*La estructura puede evolucionar conforme se incorporen nuevos niveles, funcionalidades y herramientas.*

## Tecnologías y herramientas

| Tecnología | Aplicación |
|---|---|
| C | Desarrollo de la lógica del videojuego. |
| GBDK-2020 | Compilación de programas para Game Boy. |
| Verilog | Diseño digital del puente de comunicación con el cartucho. |
| Python | Comunicación serial y gestión de la programación de la memoria. |
| FPGA Gowin | Implementación de la lógica digital del sistema. |
| Icarus Verilog | Simulación de módulos digitales. |
| GTKWave | Visualización y análisis de señales de simulación. |
| Git y GitHub | Control de versiones y colaboración. |

## Hardware de referencia

El sistema de cartucho utilizado como referencia corresponde al EverSave MBC5, cuyas características principales son:

| Componente | Referencia | Capacidad |
|---|---|---:|
| Controlador de bancos | MBC5 en FPGA | Hasta 512 bancos ROM |
| Memoria Flash | S29GL064N | 8 MiB |
| Memoria FRAM | FM28V100 | 128 KiB |
| FPGA de la consola | Gowin GW2AR-18 | Según configuración de la plataforma |

La compatibilidad de cada videojuego depende de su tipo de controlador, tamaño de ROM, requisitos de memoria y funcionalidades utilizadas.

## Documentación

La documentación técnica se encuentra distribuida en los archivos del repositorio.

- **Quemador:** descripción de la arquitectura, instalación, comunicación y procedimiento de programación del cartucho.
- **Compatibilidad de juegos:** análisis de las capacidades de memoria, requisitos de ROM, limitaciones y selección de juegos candidatos para pruebas.
- **Revisión técnica:** documentación de los aspectos de diseño y evaluación del proyecto.
- **Investigación y estado del juego:** seguimiento del desarrollo y de las características implementadas.

## Estado del proyecto

El desarrollo comprende diferentes etapas de trabajo:

- [x] Organización del código y recursos del videojuego.
- [x] Desarrollo de herramientas y documentación del proyecto.
- [x] Diseño inicial del sistema de quemado mediante FPGA y Python.
- [x] Documentación preliminar de compatibilidad del cartucho.
- [ ] Validación física de las conexiones de la FPGA.
- [ ] Pruebas de simulación y compilación del quemador.
- [ ] Verificación de comunicación entre PC y FPGA.
- [ ] Lectura y respaldo de la memoria del cartucho.
- [ ] Grabación y verificación de una ROM de prueba.
- [ ] Validación de ejecución del videojuego en la consola.

*Las casillas representan el estado documental y de planificación; las pruebas físicas solo se marcarán como completadas cuando se hayan ejecutado y registrado sus resultados.*

## Objetivo final

Consolidar un videojuego funcional y un procedimiento reproducible para su compilación, transferencia y ejecución en la plataforma FPGBC, integrando los componentes de software, hardware y documentación desarrollados durante el proyecto.

---

**Proyecto académico – Universidad Nacional de Colombia**  
**Repositorio:** `digital-un-2026-2/FPGBC_juego`
