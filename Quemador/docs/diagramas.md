# Diagramas del quemador

Metodología: para cada bloque hay (1) un **diagrama de caja negra** con sus entradas y salidas, (2) un **diagrama de bloques** que separa el camino de datos de la unidad de control y (3) el **diagrama de flujo (ASM)** de la unidad de control. En los diagramas ASM, cada rectángulo es un estado y lista las salidas que valen 1 en ese estado (máquina de Moore); los rombos son las decisiones.

Los diagramas están en Mermaid; GitHub los dibuja solos al abrir este archivo.

Jerarquía:

```
cart_bridge (top)
├── uart_rx    = sincronizador + contador_dec + contador_inc + comparador + rsr_rx + control_uart_rx
├── fifo_rx
├── uart_tx    = registro + rsr_tx + contador_dec + contador_inc + comparador + control_uart_tx
├── cart_bus   = registro x4 + mux4 + contador_dec + control_cart_bus
├── registro, contador_inc, contador_dec, comparador, mux4, deco_cmd, deco_arg, rom_version
└── control_bridge
```

---

## 1. Sistema completo: `cart_bridge`

### Caja negra

```mermaid
flowchart LR
    clk[clk] --> B
    rst[rst_n] --> B
    rx[uart_rx] --> B
    B["cart_bridge<br/>(puente PC - cartucho)"]
    B --> tx[uart_tx]
    B --> a["cart_a[15:0]"]
    B <--> d["cart_d[7:0]"]
    B --> wr[cart_wr_n]
    B --> rd[cart_rd_n]
    B --> cs[cart_cs_n]
    B --> dir[data_dir]
    B --> oe[ctrl_oe]
    B --> rs[cart_rst_n]
    B --> ck[cart_clk]
```

| Señal | Dir. | Ancho | Función |
| --- | --- | --- | --- |
| `clk` | E | 1 | Reloj de 40 MHz (pin 11) |
| `rst_n` | E | 1 | Reset activo en bajo (pin 120) |
| `uart_rx` / `uart_tx` | E / S | 1 | Serial con el PC por el puerto link (pines 27 / 28) |
| `cart_a` | S | 16 | Dirección A0–A15 del conector |
| `cart_d` | E/S | 8 | Datos D0–D7 del conector |
| `cart_wr_n`, `cart_rd_n`, `cart_cs_n` | S | 1 c/u | Control del bus (activos en bajo) |
| `data_dir` | S | 1 | Dirección del level shifter de datos |
| `ctrl_oe`, `cart_rst_n`, `cart_clk` | S | 1 c/u | Constantes: 1, 1 y 0 |

### Diagrama de bloques

```mermaid
flowchart LR
    RX[uart_rx] -->|byte, valid| FIFO[fifo_rx]
    FIFO -->|fifo_out| CMD[reg_cmd + deco_cmd]
    FIFO -->|fifo_out| ARGS["reg_arg0..3<br/>cont_argi, deco_arg"]
    ARGS --> DIR[cont_dir]
    ARGS --> REM[cont_rem]
    DIR --> MA[mux_addr]
    ARGS --> MA
    ARGS --> MW[mux_wdata]
    MA --> BUS[cart_bus]
    MW --> BUS
    BUS -->|rdata| MT[mux_tx]
    BUS -->|rdata| POLL["comp_dq7, reg_dq5<br/>cont_poll"]
    VER["cont_ver + rom_version"] --> MT
    MT --> TX[uart_tx]
    CTRL{{control_bridge}} -. señales de control .-> FIFO & ARGS & DIR & REM & MA & MW & BUS & POLL & VER & MT & TX
    CMD -. is_v/is_r/is_w/is_p .-> CTRL
    POLL -. dq7_ok, dq5, poll_max .-> CTRL
    REM -. rem_z .-> CTRL
```

### ASM de `control_bridge`

```mermaid
flowchart TD
    IDLE[IDLE] --> e1{empty?}
    e1 -- sí --> IDLE
    e1 -- no --> POP["POP<br/>pop"]
    POP --> LOAD_CMD["LOAD_CMD<br/>ld_cmd, clr_argi"]
    LOAD_CMD --> DECODE[DECODE]
    DECODE --> c1{is_v?}
    c1 -- sí --> V_INIT
    c1 -- no --> c2{"is_r o is_w o is_p?"}
    c2 -- no --> IDLE
    c2 -- sí --> ARGS[ARGS]
    ARGS --> a1{args_done?}
    a1 -- sí --> EXEC[EXEC]
    a1 -- no --> a2{empty?}
    a2 -- sí --> ARGS
    a2 -- no --> ARG_POP["ARG_POP<br/>pop"]
    ARG_POP --> ARG_LOAD["ARG_LOAD<br/>ld_arg (e inc argi)"]
    ARG_LOAD --> ARGS
    EXEC --> x1{is_r?}
    x1 -- sí --> R_INIT
    x1 -- no --> x2{is_w?}
    x2 -- sí --> W_BUS
    x2 -- no --> P1

    subgraph R ["Comando R: leer n bytes"]
        R_INIT["R_INIT<br/>ld_dir, ld_rem"] --> R_BUS["R_BUS<br/>bus_start, sel_addr=DIR"]
        R_BUS --> R_WAIT[R_WAIT] --> r1{bus_busy?}
        r1 -- sí --> R_WAIT
        r1 -- no --> R_TXCHK[R_TXCHK] --> r2{tx_busy?}
        r2 -- sí --> R_TXCHK
        r2 -- no --> R_TX["R_TX<br/>tx_start, sel_tx=RDATA<br/>inc_dir, dec_rem"]
        R_TX --> R_NEXT[R_NEXT] --> r3{rem_z?}
        r3 -- no --> R_BUS
    end
    r3 -- sí --> IDLE

    subgraph W ["Comando W: escribir"]
        W_BUS["W_BUS<br/>bus_start, bus_write<br/>sel_addr=ARG, sel_wdata=ARG"] --> W_WAIT[W_WAIT] --> w1{bus_busy?}
        w1 -- sí --> W_WAIT
    end
    w1 -- no --> K_CHK

    subgraph P ["Comando P: programar un byte"]
        P1["P1<br/>bus_start, bus_write<br/>AAA ← AA"] --> P1W{"P1_WAIT<br/>bus_busy?"}
        P1W -- no --> P2["P2<br/>555 ← 55"] --> P2W{"P2_WAIT<br/>bus_busy?"}
        P2W -- no --> P3["P3<br/>AAA ← A0"] --> P3W{"P3_WAIT<br/>bus_busy?"}
        P3W -- no --> P4["P4<br/>dir ← dato"] --> P4W{"P4_WAIT<br/>bus_busy?"}
        P4W -- no --> POLL_INIT["POLL_INIT<br/>clr_poll (borra contador y dq5)"]
        POLL_INIT --> POLLS["POLL<br/>bus_start (lectura), inc_poll"]
        POLLS --> PW{"POLL_WAIT<br/>bus_busy?"}
        PW -- no --> PC["POLL_CHK<br/>ld_dq5"]
        PC --> p1{dq7_ok?}
        p1 -- no --> p2{"dq5 o poll_max?"}
        p2 -- no --> POLLS
    end
    p1 -- sí --> K_CHK
    p2 -- sí --> X_CHK

    K_CHK[K_CHK] --> k1{tx_busy?}
    k1 -- sí --> K_CHK
    k1 -- no --> K_TX["K_TX<br/>tx_start, sel_tx='K'"] --> IDLE
    X_CHK[X_CHK] --> k2{tx_busy?}
    k2 -- sí --> X_CHK
    k2 -- no --> X_TX["X_TX<br/>tx_start, sel_tx='X'"] --> IDLE

    subgraph V ["Comando V: versión"]
        V_INIT["V_INIT<br/>clr_ver"] --> V_CHK[V_CHK] --> v1{tx_busy?}
        v1 -- sí --> V_CHK
        v1 -- no --> V_TX["V_TX<br/>tx_start, sel_tx=VER, inc_ver"]
        V_TX --> V_NEXT[V_NEXT] --> v2{ver_done?}
        v2 -- no --> V_CHK
    end
    v2 -- sí --> IDLE
```

(Los estados `Pn_WAIT` y `POLL_WAIT` vuelven a sí mismos mientras `bus_busy = 1`; en el diagrama se dibujaron como un solo rombo para no recargarlo.)

---

## 2. Ciclo de bus: `cart_bus`

### Caja negra

```mermaid
flowchart LR
    clk --> B
    rst --> B
    start --> B
    is_write --> B
    addr["addr[15:0]"] --> B
    wdata["wdata[7:0]"] --> B
    B["cart_bus"]
    B --> rdata["rdata[7:0]"]
    B --> busy
    B --> ca["cart_a[15:0]"]
    B <--> cd["cart_d[7:0]"]
    B --> wr[cart_wr_n]
    B --> rd[cart_rd_n]
    B --> cs[cart_cs_n]
    B --> dir[data_dir]
```

### Diagrama de bloques

```mermaid
flowchart LR
    addr --> RA[reg_addr] --> cart_a
    wdata --> RW[reg_wdata] --> TRI["buffer tri-estado"] <--> cart_d
    cart_d --> RR[reg_rdata] --> rdata
    MT["mux_t<br/>SETUP / PULSE / HOLD"] --> CT[cont_t]
    CT -. z_t .-> CTRL{{control_cart_bus}}
    CTRL -. ld_in .-> RA & RW
    CTRL -. ld_rdata .-> RR
    CTRL -. "ld_t, sel_t, dec_t" .-> MT & CT
    CTRL --> RS["reg_salida<br/>wr_n, rd_n, drive_d, dir"]
    RS --> cart_wr_n & cart_rd_n & data_dir
    RS -. drive_d .-> TRI
```

### ASM de `control_cart_bus`

```mermaid
flowchart TD
    IDLE["IDLE<br/>busy=0, ld_in, ld_t (SETUP)"] --> s{start?}
    s -- no --> IDLE
    s -- sí --> w{is_write?}

    w -- sí --> WS["W_SETUP<br/>dir=W, drive_d, dec_t"]
    WS --> z1{z_t?}
    z1 -- no --> WS
    z1 -- sí --> WL["W_LDP<br/>dir=W, drive_d, wr_n=0, ld_t (PULSE)"]
    WL --> WP["W_PULSE<br/>dir=W, drive_d, wr_n=0, dec_t"]
    WP --> z2{z_t?}
    z2 -- no --> WP
    z2 -- sí --> WH1["W_LDH<br/>dir=W, drive_d, ld_t (HOLD)"]
    WH1 --> WH["W_HOLD<br/>dir=W, drive_d, dec_t"]
    WH --> z3{z_t?}
    z3 -- no --> WH
    z3 -- sí --> EL

    w -- no --> RSU["R_SETUP<br/>dec_t"]
    RSU --> z4{z_t?}
    z4 -- no --> RSU
    z4 -- sí --> RL["R_LDP<br/>rd_n=0, ld_t (PULSE)"]
    RL --> RP["R_PULSE<br/>rd_n=0, dec_t"]
    RP --> z5{z_t?}
    z5 -- no --> RP
    z5 -- sí --> RS["R_SAMP<br/>rd_n=0, ld_rdata"]
    RS --> EL

    EL["END_LD<br/>ld_t (HOLD)"] --> E["END<br/>dec_t"]
    E --> z6{z_t?}
    z6 -- no --> E
    z6 -- sí --> IDLE
```

En todos los estados que no lo dicen: `wr_n = rd_n = 1`, `drive_d = 0`, `dir` apunta del cartucho a la FPGA y `busy = 1`. Las salidas pasan por `reg_salida` antes de llegar a los pines, así que llegan un ciclo después y sin glitches.

---

## 3. Receptor serial: `uart_rx`

### Caja negra

```mermaid
flowchart LR
    clk --> B
    rst --> B
    rx --> B
    B["uart_rx<br/>8N1, CLKS_PER_BIT"]
    B --> data["data[7:0]"]
    B --> valid
```

### Diagrama de bloques

```mermaid
flowchart LR
    rx --> S[sincronizador] --> SR[rsr_rx] --> data
    CB["cont_baud<br/>(HALF o FULL)"] -. z_baud .-> CTRL{{control_uart_rx}}
    CN[cont_bits] --> CMP["comparador = 8"] -. bits_done .-> CTRL
    S -. rx .-> CTRL
    CTRL -. "ld_half, ld_full, dec_baud" .-> CB
    CTRL -. "clr_bits, inc_bits" .-> CN
    CTRL -. shift .-> SR
    CTRL --> valid
```

### ASM de `control_uart_rx`

```mermaid
flowchart TD
    IDLE["IDLE<br/>ld_half"] --> r0{rx = 0?}
    r0 -- no --> IDLE
    r0 -- sí --> START["START<br/>dec_baud"]
    START --> z0{z_baud?}
    z0 -- no --> START
    z0 -- sí --> r1{rx = 0?}
    r1 -- "no (ruido)" --> IDLE
    r1 -- sí --> SOK["START_OK<br/>ld_full, clr_bits"]
    SOK --> BITS["BITS<br/>dec_baud"]
    BITS --> z1{z_baud?}
    z1 -- no --> BITS
    z1 -- sí --> SAMPLE["SAMPLE<br/>shift, inc_bits, ld_full"]
    SAMPLE --> CHECK[CHECK] --> b{bits_done?}
    b -- no --> BITS
    b -- sí --> STOP["STOP<br/>dec_baud"]
    STOP --> z2{z_baud?}
    z2 -- no --> STOP
    z2 -- sí --> r2{rx = 1?}
    r2 -- no --> IDLE
    r2 -- sí --> DONE["DONE<br/>valid"] --> IDLE
```

---

## 4. Transmisor serial: `uart_tx`

### Caja negra

```mermaid
flowchart LR
    clk --> B
    rst --> B
    data["data[7:0]"] --> B
    start --> B
    B["uart_tx<br/>8N1, CLKS_PER_BIT"]
    B --> tx
    B --> busy
```

### Diagrama de bloques

```mermaid
flowchart LR
    data --> RD[reg_dato] --> SR["rsr_tx (10 bits)"] --> tx
    CB[cont_baud] -. z_baud .-> CTRL{{control_uart_tx}}
    CN[cont_bits] --> CMP["comparador = 10"] -. bits_done .-> CTRL
    start --> CTRL
    CTRL -. ld_dato .-> RD
    CTRL -. "ld_sr, shift" .-> SR
    CTRL -. "ld_baud, dec_baud" .-> CB
    CTRL -. "clr_bits, inc_bits" .-> CN
    CTRL --> busy
```

### ASM de `control_uart_tx`

```mermaid
flowchart TD
    IDLE["IDLE<br/>busy=0, ld_dato"] --> s{start?}
    s -- no --> IDLE
    s -- sí --> LOAD["LOAD<br/>ld_sr, ld_baud, clr_bits"]
    LOAD --> SEND["SEND<br/>dec_baud"]
    SEND --> z{z_baud?}
    z -- no --> SEND
    z -- sí --> SHIFT["SHIFT<br/>shift, inc_bits, ld_baud"]
    SHIFT --> CHECK[CHECK] --> b{bits_done?}
    b -- no --> SEND
    b -- sí --> IDLE
```

---

## Módulos del camino de datos (`rtl/datapath/`)

| Módulo | Qué hace | Dónde se usa |
| --- | --- | --- |
| `registro` | Registro con carga, borrado y valor de reset | Comando, argumentos, dirección, datos, `reg_salida`, `reg_dq5`, `reg_dato` |
| `contador_dec` | Contador descendente con carga y bandera `z` | Tiempos de bit (UART), tiempos de bus, bytes restantes |
| `contador_inc` | Contador ascendente con borrado y carga | Bits UART, índice de argumento, dirección de lectura, sondeos, versión |
| `comparador` | Igualdad | Fin de bits, fin de argumentos, DQ7, límite de sondeo, fin de versión |
| `mux4` | Multiplexor 4 a 1 | Dirección y dato del bus, byte a transmitir, carga del contador de tiempos |
| `rsr_rx` | Desplazamiento a la derecha con entrada serial | `uart_rx` |
| `rsr_tx` | Desplazamiento de 10 bits con salida serial | `uart_tx` |
| `sincronizador` | Dos flip-flops para la entrada asíncrona | `uart_rx` |
| `fifo_rx` | Memoria de 512 bytes con punteros | Bytes recibidos |
| `deco_cmd` | Decodifica el comando y cuántos argumentos espera | `cart_bridge` |
| `deco_arg` | Decodificador 2 a 4: qué argumento se carga | `cart_bridge` |
| `rom_version` | Caracteres de "FPGBC1" | `cart_bridge` |
