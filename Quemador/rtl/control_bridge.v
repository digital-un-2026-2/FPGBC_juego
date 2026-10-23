// Unidad de control del interprete de comandos (maquina de Moore).
// Ver docs/diagramas.md, "Interprete de comandos".
module control_bridge (
    input  wire       clk,
    input  wire       rst,
    // Entradas desde el camino de datos
    input  wire       empty,          // FIFO de recepcion vacia
    input  wire       is_v, is_r, is_w, is_p,
    input  wire       args_done,      // ya se leyeron todos los argumentos
    input  wire       bus_busy,
    input  wire       tx_busy,
    input  wire       rem_z,          // no quedan bytes por leer (comando R)
    input  wire       dq7_ok,         // DQ7 leido = DQ7 del dato: la flash termino
    input  wire       dq5,            // bandera de error de la flash ya vista
    input  wire       poll_max,       // se agotaron los intentos de sondeo
    input  wire       ver_done,       // ya se enviaron los 6 caracteres de 'V'

    // Senales de control hacia el camino de datos
    output reg        pop,
    output reg        ld_cmd,
    output reg        clr_argi,
    output reg        ld_arg,         // guarda el byte de la FIFO en el argumento actual
    output reg        ld_dir,         // direccion de lectura <- argumento
    output reg        inc_dir,
    output reg        ld_rem,         // bytes restantes <- argumento
    output reg        dec_rem,
    output reg        bus_start,
    output reg        bus_write,
    output reg  [1:0] sel_addr,       // 0 = dir, 1 = argumento, 2 = 0x0AAA, 3 = 0x0555
    output reg  [1:0] sel_wdata,      // 0 = argumento, 1 = 0xAA, 2 = 0x55, 3 = 0xA0
    output reg        clr_poll,
    output reg        inc_poll,
    output reg        ld_dq5,
    output reg        clr_ver,
    output reg        inc_ver,
    output reg        tx_start,
    output reg  [1:0] sel_tx          // 0 = dato leido, 1 = version, 2 = 'K', 3 = 'X'
);
    parameter IDLE      = 6'd0;
    parameter POP       = 6'd1;
    parameter LOAD_CMD  = 6'd2;
    parameter DECODE    = 6'd3;
    parameter ARGS      = 6'd4;
    parameter ARG_POP   = 6'd5;
    parameter ARG_LOAD  = 6'd6;
    parameter EXEC      = 6'd7;
    // R: leer n bytes
    parameter R_INIT    = 6'd8;
    parameter R_BUS     = 6'd9;
    parameter R_WAIT    = 6'd10;
    parameter R_TXCHK   = 6'd11;
    parameter R_TX      = 6'd12;
    parameter R_NEXT    = 6'd13;
    // W: una escritura
    parameter W_BUS     = 6'd14;
    parameter W_WAIT    = 6'd15;
    // P: programar un byte de la flash
    parameter P1        = 6'd16;
    parameter P1_WAIT   = 6'd17;
    parameter P2        = 6'd18;
    parameter P2_WAIT   = 6'd19;
    parameter P3        = 6'd20;
    parameter P3_WAIT   = 6'd21;
    parameter P4        = 6'd22;
    parameter P4_WAIT   = 6'd23;
    parameter POLL_INIT = 6'd24;
    parameter POLL      = 6'd25;
    parameter POLL_WAIT = 6'd26;
    parameter POLL_CHK  = 6'd27;
    // Respuestas
    parameter K_CHK     = 6'd28;
    parameter K_TX      = 6'd29;
    parameter X_CHK     = 6'd30;
    parameter X_TX      = 6'd31;
    // V: version
    parameter V_INIT    = 6'd32;
    parameter V_CHK     = 6'd33;
    parameter V_TX      = 6'd34;
    parameter V_NEXT    = 6'd35;

    parameter A_DIR = 2'd0, A_ARG = 2'd1, A_AAA = 2'd2, A_555 = 2'd3;
    parameter D_ARG = 2'd0, D_AA  = 2'd1, D_55  = 2'd2, D_A0  = 2'd3;
    parameter T_RDATA = 2'd0, T_VER = 2'd1, T_K = 2'd2, T_X = 2'd3;

    reg [5:0] state;

    // ------------------------- Siguiente estado -------------------------
    always @(posedge clk) begin
        if (rst)
            state <= IDLE;
        else case (state)
            IDLE:      state <= empty ? IDLE : POP;
            POP:       state <= LOAD_CMD;
            LOAD_CMD:  state <= DECODE;
            DECODE:    if (is_v)                    state <= V_INIT;
                       else if (is_r | is_w | is_p) state <= ARGS;
                       else                         state <= IDLE;   // byte desconocido
            ARGS:      if (args_done)  state <= EXEC;
                       else if (!empty) state <= ARG_POP;
            ARG_POP:   state <= ARG_LOAD;
            ARG_LOAD:  state <= ARGS;
            EXEC:      if (is_r)      state <= R_INIT;
                       else if (is_w) state <= W_BUS;
                       else           state <= P1;

            R_INIT:    state <= R_BUS;
            R_BUS:     state <= R_WAIT;
            R_WAIT:    if (!bus_busy) state <= R_TXCHK;
            R_TXCHK:   if (!tx_busy)  state <= R_TX;
            R_TX:      state <= R_NEXT;
            R_NEXT:    state <= rem_z ? IDLE : R_BUS;

            W_BUS:     state <= W_WAIT;
            W_WAIT:    if (!bus_busy) state <= K_CHK;

            P1:        state <= P1_WAIT;
            P1_WAIT:   if (!bus_busy) state <= P2;
            P2:        state <= P2_WAIT;
            P2_WAIT:   if (!bus_busy) state <= P3;
            P3:        state <= P3_WAIT;
            P3_WAIT:   if (!bus_busy) state <= P4;
            P4:        state <= P4_WAIT;
            P4_WAIT:   if (!bus_busy) state <= POLL_INIT;
            POLL_INIT: state <= POLL;
            POLL:      state <= POLL_WAIT;
            POLL_WAIT: if (!bus_busy) state <= POLL_CHK;
            POLL_CHK:  if (dq7_ok)                state <= K_CHK;
                       else if (dq5 | poll_max)   state <= X_CHK;
                       else                       state <= POLL;

            K_CHK:     if (!tx_busy) state <= K_TX;
            K_TX:      state <= IDLE;
            X_CHK:     if (!tx_busy) state <= X_TX;
            X_TX:      state <= IDLE;

            V_INIT:    state <= V_CHK;
            V_CHK:     if (!tx_busy) state <= V_TX;
            V_TX:      state <= V_NEXT;
            V_NEXT:    state <= ver_done ? IDLE : V_CHK;
            default:   state <= IDLE;
        endcase
    end

    // ----------------------------- Salidas ------------------------------
    always @(*) begin
        pop = 0; ld_cmd = 0; clr_argi = 0; ld_arg = 0; ld_dir = 0; inc_dir = 0;
        ld_rem = 0; dec_rem = 0; bus_start = 0; bus_write = 0;
        sel_addr = A_DIR; sel_wdata = D_ARG; clr_poll = 0; inc_poll = 0; ld_dq5 = 0;
        clr_ver = 0; inc_ver = 0; tx_start = 0; sel_tx = T_RDATA;
        case (state)
            POP:       pop = 1;
            LOAD_CMD:  begin ld_cmd = 1; clr_argi = 1; end
            ARG_POP:   pop = 1;
            ARG_LOAD:  ld_arg = 1;

            R_INIT:    begin ld_dir = 1; ld_rem = 1; end
            R_BUS:     begin bus_start = 1; sel_addr = A_DIR; end
            R_TX:      begin tx_start = 1; sel_tx = T_RDATA; inc_dir = 1; dec_rem = 1; end

            W_BUS:     begin bus_start = 1; bus_write = 1; sel_addr = A_ARG; sel_wdata = D_ARG; end

            P1:        begin bus_start = 1; bus_write = 1; sel_addr = A_AAA; sel_wdata = D_AA; end
            P2:        begin bus_start = 1; bus_write = 1; sel_addr = A_555; sel_wdata = D_55; end
            P3:        begin bus_start = 1; bus_write = 1; sel_addr = A_AAA; sel_wdata = D_A0; end
            P4:        begin bus_start = 1; bus_write = 1; sel_addr = A_ARG; sel_wdata = D_ARG; end
            POLL_INIT: begin clr_poll = 1; end
            POLL:      begin bus_start = 1; sel_addr = A_ARG; inc_poll = 1; end
            POLL_CHK:  ld_dq5 = 1;     // recordar DQ5 para dar una lectura mas antes de fallar

            K_TX:      begin tx_start = 1; sel_tx = T_K; end
            X_TX:      begin tx_start = 1; sel_tx = T_X; end

            V_INIT:    clr_ver = 1;
            V_TX:      begin tx_start = 1; sel_tx = T_VER; inc_ver = 1; end
        endcase
    end

`ifdef BENCH
    reg [8*10:1] state_name;
    always @(*) case (state)
        IDLE: state_name = "IDLE";         POP: state_name = "POP";           LOAD_CMD: state_name = "LOAD_CMD";
        DECODE: state_name = "DECODE";     ARGS: state_name = "ARGS";         ARG_POP: state_name = "ARG_POP";
        ARG_LOAD: state_name = "ARG_LOAD"; EXEC: state_name = "EXEC";         R_INIT: state_name = "R_INIT";
        R_BUS: state_name = "R_BUS";       R_WAIT: state_name = "R_WAIT";     R_TXCHK: state_name = "R_TXCHK";
        R_TX: state_name = "R_TX";         R_NEXT: state_name = "R_NEXT";     W_BUS: state_name = "W_BUS";
        W_WAIT: state_name = "W_WAIT";     P1: state_name = "P1";             P1_WAIT: state_name = "P1_WAIT";
        P2: state_name = "P2";             P2_WAIT: state_name = "P2_WAIT";   P3: state_name = "P3";
        P3_WAIT: state_name = "P3_WAIT";   P4: state_name = "P4";             P4_WAIT: state_name = "P4_WAIT";
        POLL_INIT: state_name = "POLL_INIT"; POLL: state_name = "POLL";       POLL_WAIT: state_name = "POLL_WAIT";
        POLL_CHK: state_name = "POLL_CHK"; K_CHK: state_name = "K_CHK";       K_TX: state_name = "K_TX";
        X_CHK: state_name = "X_CHK";       X_TX: state_name = "X_TX";         V_INIT: state_name = "V_INIT";
        V_CHK: state_name = "V_CHK";       V_TX: state_name = "V_TX";         V_NEXT: state_name = "V_NEXT";
    endcase
`endif
endmodule
