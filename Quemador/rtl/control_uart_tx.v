// Unidad de control del transmisor UART (maquina de Moore).
// Ver docs/diagramas.md, "UART TX".
module control_uart_tx (
    input  wire clk,
    input  wire rst,
    input  wire start,       // pedido de envio
    input  wire z_baud,
    input  wire bits_done,   // ya salieron los 10 bits (inicio, 8 datos, parada)

    output reg  ld_dato,     // copiar el byte de entrada mientras espera
    output reg  ld_sr,       // cargar {parada, dato, inicio}
    output reg  shift,
    output reg  ld_baud,
    output reg  dec_baud,
    output reg  clr_bits,
    output reg  inc_bits,
    output reg  busy
);
    parameter IDLE  = 3'd0;
    parameter LOAD  = 3'd1;
    parameter SEND  = 3'd2;
    parameter SHIFT = 3'd3;
    parameter CHECK = 3'd4;

    reg [2:0] state;

    always @(posedge clk) begin
        if (rst)
            state <= IDLE;
        else case (state)
            IDLE:    state <= start ? LOAD : IDLE;
            LOAD:    state <= SEND;
            SEND:    if (z_baud) state <= SHIFT;
            SHIFT:   state <= CHECK;
            CHECK:   state <= bits_done ? IDLE : SEND;
            default: state <= IDLE;
        endcase
    end

    always @(*) begin
        ld_dato = 0; ld_sr = 0; shift = 0; ld_baud = 0; dec_baud = 0;
        clr_bits = 0; inc_bits = 0; busy = 1;
        case (state)
            IDLE:  begin busy = 0; ld_dato = 1; end
            LOAD:  begin ld_sr = 1; ld_baud = 1; clr_bits = 1; end
            SEND:  dec_baud = 1;
            SHIFT: begin shift = 1; inc_bits = 1; ld_baud = 1; end
            CHECK: ;
        endcase
    end

`ifdef BENCH
    reg [8*8:1] state_name;
    always @(*) case (state)
        IDLE: state_name = "IDLE"; LOAD: state_name = "LOAD"; SEND: state_name = "SEND";
        SHIFT: state_name = "SHIFT"; CHECK: state_name = "CHECK";
    endcase
`endif
endmodule
