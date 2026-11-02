\# Compatibilidad de juegos con el cartucho FPGBC

\*\*Proyecto:\*\* FPGBC – Game Boy / Game Boy Color

\*\*Hardware:\*\* Cartucho EverSave MBC5

\*\*Controlador del cartucho:\*\* MBC5 implementado en FPGA

\*\*Documento:\*\* Análisis de compatibilidad y selección de juegos de prueba

\*\*Estado:\*\* Análisis técnico preliminar; compatibilidad física pendiente de validación.

\---

\## 1. Introducción

El presente documento establece los criterios técnicos para determinar qué juegos de Game Boy y Game Boy Color pueden almacenarse y ejecutarse en el cartucho FPGBC. El análisis considera la capacidad de sus memorias, la implementación del controlador MBC5 y las funcionalidades disponibles en el sistema de quemado.

El propósito es identificar las condiciones que debe cumplir una ROM para ser candidata a grabación, reconocer las limitaciones de hardware y software existentes y proponer títulos que permitan evaluar progresivamente el funcionamiento del cartucho.

Se deben diferenciar dos aspectos:

- \*\*Compatibilidad de almacenamiento:\*\* posibilidad de escribir íntegramente el archivo del juego en la memoria Flash.
- \*\*Compatibilidad de ejecución:\*\* capacidad del cartucho para proporcionar el controlador, la selección de bancos, la memoria externa y los recursos que el juego espera durante su funcionamiento.

Que un archivo quepa en la memoria Flash no garantiza que pueda ejecutarse correctamente. En particular, el cartucho implementa MBC5; los juegos diseñados para MBC1, MBC3 u otros controladores no son directamente compatibles por el solo hecho de tener un tamaño adecuado.

\## 2. Especificaciones técnicas del cartucho

El cartucho EverSave MBC5 integra una memoria Flash para almacenar la ROM, una memoria FRAM para datos persistentes y una FPGA que implementa la lógica de selección de bancos.

\### 2.1. Memoria ROM

| Característica | Especificación |

\|---|---|

| Tecnología | Flash NOR |

| Referencia | S29GL064N90TFI04 |

| Capacidad | 64 Mbit |

| Capacidad en bytes | 8.388.608 bytes (8 MiB) |

| Organización utilizada | Modo byte (8 bits) |

| Función | Almacenamiento del programa y los recursos del juego |

La capacidad de 8 MiB constituye el límite físico de almacenamiento de la ROM en el cartucho.

\### 2.2. Memoria FRAM

| Característica | Especificación |

\|---|---|

| Tecnología | FRAM |

| Referencia | FM28V100-TG |

| Capacidad | 1 Mbit |

| Capacidad en bytes | 131.072 bytes (128 KiB) |

| Organización documentada | 16 bancos de 8 KiB |

| Función | Almacenamiento persistente de datos del juego |

La disponibilidad de FRAM no significa que cualquier juego pueda utilizarla automáticamente. El juego debe esperar un esquema de memoria y un controlador compatibles con la implementación del cartucho.

\### 2.3. Controlador MBC5

El controlador MBC5 del cartucho está implementado en una FPGA Gowin GW1NZ-LV1, de acuerdo con la documentación del repositorio `FPGBC\_cartucho`.

Sus capacidades documentadas son:

| Recurso | Capacidad |

\|---|---:|

| Bancos ROM | Hasta 512 |

| Tamaño de cada banco ROM | 16 KiB |

| Capacidad máxima ROM | 8 MiB |

| Bancos FRAM | Hasta 16 |

| Tamaño de cada banco FRAM | 8 KiB |

| Capacidad máxima FRAM | 128 KiB |

La selección de bancos ROM utiliza dos registros:

- `0x2000–0x2FFF`: selección de los 8 bits inferiores del banco ROM.
- `0x3000–0x3FFF`: selección del bit 8 del banco ROM.

Para la FRAM, el controlador utiliza el rango `0x4000–0x5FFF` para seleccionar uno de sus 16 bancos.

\### 2.4. Mapa de memoria

La consola Game Boy dispone de un espacio de direcciones de 16 bits. El MBC5 permite acceder a memorias de mayor capacidad mediante conmutación de bancos.

| Rango de direcciones | Tamaño | Función |

\|---|---:|---|

| `0x0000–0x3FFF` | 16 KiB | Banco 0 de ROM |

| `0x4000–0x7FFF` | 16 KiB | Banco ROM conmutable |

| `0xA000–0xBFFF` | 8 KiB | Ventana de acceso a FRAM |

Esta organización permite que el juego acceda a diferentes regiones de memoria sin superar el espacio de direcciones disponible para el cartucho.

\## 3. Criterios de compatibilidad

Para considerar un juego como candidato se deben evaluar su tamaño, el controlador declarado en la cabecera y los recursos que necesita durante la ejecución.

\### 3.1. Capacidad de almacenamiento

La ROM debe cumplir la siguiente condición:

\[

S\_{\mathrm{ROM}} \leq 8.388.608\ \text{bytes}

\]

El tamaño debe corresponder a una imagen completa del cartucho, no a un archivo comprimido ni a un conjunto de archivos de distribución.

El límite de 8 MiB corresponde a la capacidad física de la Flash y coincide con el direccionamiento máximo documentado para el MBC5 del cartucho.

\### 3.2. Tipo de controlador

El byte `0x0147` de la cabecera de una ROM identifica el tipo de cartucho y el controlador que espera utilizar el juego.

El programa `pc/fpgbc\_cart.py` incluye explícitamente los siguientes códigos en su tabla `CART\_TYPES`:

| Código de cabecera | Tipo declarado | Evaluación para este cartucho |

\|---|---|---|

| `0x19` | MBC5 | Candidato prioritario |

| `0x1A` | MBC5 + RAM | Candidato, sujeto a verificar acceso a FRAM |

| `0x1B` | MBC5 + RAM + batería | Candidato, sujeto a verificar acceso y persistencia de FRAM |

Estos son los tipos MBC5 que reconoce actualmente el script. El reconocimiento por parte del script no constituye por sí mismo una prueba de ejecución correcta.

\*\*Los juegos que utilizan MBC1 no son directamente compatibles con la implementación MBC5 documentada.\*\* Por ejemplo, Pokémon Red y Pokémon Blue utilizan MBC1 en sus versiones originales. Aunque sus archivos puedan caber en la Flash, el controlador MBC5 no reproduce automáticamente los registros y el comportamiento que esos juegos esperan de MBC1. Para ejecutarlos sería necesario implementar una adaptación o emulación de MBC1, que no está documentada como parte del diseño actual.

La misma precaución aplica a juegos que utilizan MBC3 u otros controladores: no deben clasificarse como compatibles sin implementar y verificar el comportamiento correspondiente.

Los tipos MBC5 con funciones adicionales, como las variantes que declaran vibración, tampoco deben considerarse plenamente compatibles si el cartucho no incorpora el periférico requerido.

\### 3.3. Requisitos de memoria para partidas

El byte `0x0149` de la cabecera identifica el tamaño de RAM declarado por el juego. La capacidad máxima documentada para la FRAM del cartucho es de 128 KiB, distribuida en 16 bancos de 8 KiB.

La compatibilidad depende de que el juego utilice un esquema de memoria que corresponda con la implementación MBC5 y con el acceso a FRAM del cartucho.

Se deben distinguir:

- Juegos que no necesitan memoria externa para guardar partidas.
- Juegos que utilizan RAM externa y requieren acceso compatible con la FRAM.
- Juegos que dependen de funciones adicionales de almacenamiento o periféricos no documentados en el cartucho.

La presencia de una batería en el tipo de cartucho original no debe interpretarse como prueba de que el cartucho FPGBC requiera una batería: la FRAM es memoria no volátil. Sin embargo, debe verificarse que el controlador habilite y mapee correctamente esa memoria para el juego.

\### 3.4. Cabecera y estructura de la ROM

La cabecera de Game Boy contiene información necesaria para identificar y analizar cada juego.

El programa `fpgbc\_cart.py` consulta, entre otros, los siguientes campos:

| Dirección | Campo | Utilidad |

\|---|---|---|

| `0x0104–0x0133` | Logo de Nintendo | Comprobación del contenido esperado |

| `0x0134–0x0143` | Título | Identificación del juego |

| `0x0147` | Tipo de cartucho | Identificación del controlador |

| `0x0148` | Tamaño de ROM | Capacidad declarada |

| `0x0149` | Tamaño de RAM | Memoria externa declarada |

| `0x014D` | Checksum de cabecera | Comprobación de integridad de la cabecera |

El quemador muestra información de estos campos, pero actualmente no utiliza todos sus resultados como condiciones obligatorias para impedir una grabación. Por tanto, el análisis de la cabecera debe complementarse con una validación previa del archivo.

\## 4. Limitaciones identificadas

Las limitaciones se dividen entre las relacionadas con el hardware del cartucho y las correspondientes al programa de quemado.

\### 4.1. Limitaciones de hardware

| Elemento | Limitación o condición |

\|---|---|

| Memoria ROM | Capacidad física máxima de 8 MiB |

| Controlador | Implementación MBC5; no equivale a implementar todos los controladores Game Boy |

| Memoria FRAM | Capacidad máxima documentada de 128 KiB |

| Bancos ROM | Hasta 512 bancos de 16 KiB |

| Bancos FRAM | Hasta 16 bancos de 8 KiB |

| Funciones especiales | No se documenta la implementación de periféricos como vibración ni de otros controladores como MBC1 o MBC3 |

\### 4.2. Limitaciones del programa de quemado

El análisis de `pc/fpgbc\_cart.py` permite identificar las siguientes condiciones:

1. \*\*Reconocimiento limitado de tipos de cartucho:\*\* el diccionario `CART\_TYPES` reconoce `0x00` (ROM Only), `0x01` (MBC1), `0x13` (MBC3 + RAM + batería) y los tipos MBC5 `0x19`, `0x1A` y `0x1B`. Esta tabla es informativa; no implementa los controladores que aparecen en ella.
1. \*\*Ausencia de un bloqueo por capacidad:\*\* el comando `burn` lee el archivo y procede con el borrado y la programación sin comprobar previamente que su tamaño sea menor o igual a la capacidad de la Flash.
1. \*\*Validación informativa de cabecera:\*\* se muestran el logo, el tipo de cartucho, el tamaño declarado y el checksum, pero estos resultados no constituyen una política completa de aceptación o rechazo.
1. \*\*Ausencia de validación de requisitos especiales:\*\* el programa no determina si un juego depende de periféricos adicionales o de funciones que no estén implementadas.
1. \*\*Gestión de partidas no incluida en el flujo del quemador:\*\* las operaciones implementadas se concentran en la ROM. La lectura, escritura y validación de las partidas almacenadas en FRAM no forman parte del flujo documentado de esta herramienta.

Estas limitaciones no demuestran que el hardware sea incapaz de ejecutar cualquier juego no validado por el script. Indican que todavía no existe una comprobación automática suficiente para garantizar su compatibilidad.

\## 5. Selección de juegos candidatos para pruebas

Los títulos de esta sección se presentan como referencias para estudiar y planificar pruebas. \*\*No se afirma que hayan sido grabados ni ejecutados en el cartucho FPGBC.\*\* Antes de seleccionar una ROM concreta se debe revisar su cabecera y confirmar el código de controlador, el tamaño declarado y los requisitos de memoria.

\### 5.1. Tetris (Game Boy): prueba básica condicionada

\*\*Justificación:\*\*

- Es un juego de Game Boy de estructura relativamente sencilla.
- La versión original utiliza un cartucho ROM Only, sin MBC5.
- Puede servir como referencia para estudiar el arranque de una ROM sencilla, siempre que el comportamiento del cartucho permita acceder correctamente a una ROM sin controlador MBC5.

\*\*Objetivo de la prueba:\*\* evaluar inicialmente el acceso a la ROM y el arranque del juego.

\*\*Condición de compatibilidad:\*\* no es una prueba directa de la selección de bancos MBC5. Su uso queda condicionado a que el hardware admita correctamente el tipo ROM Only. No debe presentarse como compatible de manera garantizada.

\### 5.2. Pokémon Red / Blue (Game Boy): ejemplo no compatible directamente

\*\*Justificación para incluirlo como contraste:\*\*

- Son títulos conocidos que requieren almacenamiento persistente de progreso.
- Sus versiones originales utilizan MBC1, no MBC5.
- Permiten ilustrar por qué la capacidad de la Flash y la presencia de FRAM no bastan para asegurar compatibilidad.

\*\*Objetivo del análisis:\*\* mostrar una limitación de controlador y diferenciar la compatibilidad de almacenamiento de la compatibilidad de ejecución.

\*\*Condición de compatibilidad:\*\* no son candidatos para grabación y ejecución directa con la implementación MBC5 documentada. Para ejecutarlos sería necesario implementar una adaptación o emulación de MBC1, que no forma parte del alcance documentado actualmente.

\### 5.3. Juegos con MBC5: candidatos prioritarios

La primera prueba orientada a validar el controlador debe utilizar una ROM cuya cabecera declare uno de los tipos MBC5 reconocidos por el script:

- `0x19`: MBC5.
- `0x1A`: MBC5 + RAM.
- `0x1B`: MBC5 + RAM + batería.

La selección del título concreto debe realizarse después de comprobar su cabecera, tamaño real, tamaño de RAM y requisitos adicionales. Se recomienda comenzar con un juego MBC5 de tamaño reducido y sin periféricos especiales; posteriormente se puede probar uno que utilice varios bancos ROM y, finalmente, uno que requiera FRAM.

\*\*No se asigna todavía un título comercial específico a esta categoría\*\*, porque los archivos y datos de cabecera de los candidatos no han sido examinados en este análisis. Es preferible dejar esta selección pendiente antes que afirmar un controlador o tamaño sin verificarlo.

\## 6. Matriz preliminar de compatibilidad

| Criterio | Condición | Evaluación |

\|---|---|---|

| Tamaño ROM | Menor o igual a 8 MiB | Requisito físico |

| Controlador | MBC5 | Recomendado para compatibilidad directa |

| Tipo MBC5 | `0x19`, `0x1A` o `0x1B` | Reconocido por el script |

| Bancos ROM | Hasta 512 | Capacidad documentada |

| RAM externa | Hasta 128 KiB | Capacidad documentada; acceso por verificar |

| Funciones especiales | No depender de hardware no implementado | Requisito |

| Cabecera | Estructura válida | El script informa algunos campos; validación completa pendiente |

| Grabación | Coincidencia byte a byte después de programar | Verificación implementada en el script |

| Ejecución | Arranque y funcionamiento en consola | Pendiente de pruebas físicas |

\### 6.1. Clasificación propuesta

- \*\*Candidato de compatibilidad directa:\*\* ROM de tamaño admisible, tipo MBC5 contemplado y requisitos de memoria y periféricos cubiertos por el hardware.
- \*\*Candidato condicionado:\*\* cumple el límite de almacenamiento, pero requiere comprobar aspectos de controlador, RAM o funciones adicionales. Tetris (ROM Only) pertenece a esta categoría.
- \*\*No compatible directamente:\*\* utiliza un controlador distinto al MBC5 implementado, sin que exista una capa de adaptación. Pokémon Red/Blue (MBC1) pertenecen a esta categoría.

Esta clasificación es preliminar y no sustituye la comprobación experimental.

\## 7. Procedimiento propuesto para evaluar un juego

Para reducir el riesgo de errores durante la selección y grabación de ROM, se propone seguir este orden:

1. Obtener una imagen ROM completa y de procedencia legítima.
1. Consultar su tamaño real en bytes.
1. Leer y analizar la cabecera, especialmente los campos de tipo de cartucho, tamaño de ROM y tamaño de RAM.
1. Confirmar que el controlador declarado corresponda al MBC5 implementado, o clasificar expresamente el juego como condicionado/no compatible directamente.
1. Revisar si requiere periféricos o funciones adicionales.
1. Confirmar que el tamaño no exceda los 8 MiB de Flash.
1. Realizar una copia de seguridad del contenido original del cartucho.
1. Ejecutar la grabación mediante `fpgbc\_cart.py`, una vez validadas las conexiones y el funcionamiento del puente.
1. Comparar byte a byte la ROM grabada con el archivo original.
1. Probar el juego en la consola y registrar su comportamiento.

La verificación de escritura permite comprobar la coincidencia entre los datos programados y el archivo de origen. Sin embargo, no garantiza por sí sola que el juego sea funcional durante la ejecución.

\## 8. Conclusiones

El cartucho EverSave MBC5 dispone de una capacidad de almacenamiento de 8 MiB en memoria Flash y 128 KiB de memoria FRAM, con una arquitectura de conmutación de bancos que permite direccionar hasta 512 bancos ROM y 16 bancos de RAM.

Estas capacidades proporcionan una base para probar juegos de Game Boy y Game Boy Color, pero la compatibilidad directa depende también del controlador que espera utilizar cada título. La implementación documentada corresponde a MBC5; por tanto, juegos MBC1, MBC3 u otros controladores no deben considerarse compatibles directamente sin una adaptación que reproduzca su comportamiento.

El análisis de `fpgbc\_cart.py` evidencia que existen mecanismos para leer, borrar, programar y verificar la memoria Flash. No obstante, la herramienta todavía no incorpora una validación integral de compatibilidad antes de iniciar el borrado.

Se recomienda implementar posteriormente un módulo de validación de ROM que compruebe el tamaño real del archivo, el tipo de cartucho, el tamaño declarado, los bancos y los requisitos de memoria antes de permitir el borrado de la Flash.

Como siguiente paso experimental, se recomienda seleccionar una ROM MBC5 de tamaño reducido, verificar sus datos de cabecera y realizar primero las pruebas de lectura y respaldo. La compatibilidad definitiva solo podrá establecerse después de completar la grabación, verificar los datos y comprobar la ejecución en la consola FPGBC.

\## 9. Referencias del proyecto

1. `FPGBC\_cartucho/README.md`. Documentación del cartucho Game Boy / Game Boy Color con controlador MBC5. Universidad Nacional de Colombia, Electrónica Digital, 2026-2.
1. `FPGBC\_juego/Quemador/README.md`. Documentación del sistema de lectura y grabación del cartucho FPGBC.
1. `FPGBC\_juego/Quemador/pc/fpgbc\_cart.py`. Implementación de la herramienta de comunicación serial, lectura, programación y verificación de ROM.
1. Datasheet de la memoria Flash S29GL064N.
1. Datasheet de la memoria FRAM FM28V100-TG.
