// Unidad de control del receptor UART (maquina de Moore).
// Ver docs/diagramas.md, "UART RX".
module control_uart_rx (
    input  wire clk,
    input  wire rst,
    input  wire rx,          // linea serial ya sincronizada
    input  wire z_baud,      // el contador de tiempo de bit llego a 0
    input  wire bits_done,   // ya se recibieron 8 bits

    output reg  ld_half,     // cargar medio bit en el contador de tiempo
    output reg  ld_full,     // cargar un bit completo en el contador de tiempo
    output reg  dec_baud,
    output reg  clr_bits,
    output reg  inc_bits,
    output reg  shift,       // guardar el bit actual en el registro de desplazamiento
    output reg  valid        // byte completo disponible (un ciclo)
);
    parameter IDLE     = 3'd0;
    parameter START    = 3'd1;
    parameter START_OK = 3'd2;
    parameter BITS     = 3'd3;
    parameter SAMPLE   = 3'd4;
    parameter CHECK    = 3'd5;
    parameter STOP     = 3'd6;
    parameter DONE     = 3'd7;

    reg [2:0] state;

    // Siguiente estado
    always @(posedge clk) begin
        if (rst)
            state <= IDLE;
        else case (state)
            IDLE:     state <= rx ? IDLE : START;
            START:    if (z_baud) state <= rx ? IDLE : START_OK;  // ruido: vuelve a esperar
            START_OK: state <= BITS;
            BITS:     if (z_baud) state <= SAMPLE;
            SAMPLE:   state <= CHECK;
            CHECK:    state <= bits_done ? STOP : BITS;
            STOP:     if (z_baud) state <= rx ? DONE : IDLE;      // sin bit de parada: se descarta
            DONE:     state <= IDLE;
            default:  state <= IDLE;
        endcase
    end

    // Salidas
    always @(*) begin
        ld_half = 0; ld_full = 0; dec_baud = 0;
        clr_bits = 0; inc_bits = 0; shift = 0; valid = 0;
        case (state)
            IDLE:     ld_half  = 1;
            START:    dec_baud = 1;
            START_OK: begin ld_full = 1; clr_bits = 1; end
            BITS:     dec_baud = 1;
            SAMPLE:   begin shift = 1; inc_bits = 1; ld_full = 1; end
            CHECK:    ;
            STOP:     dec_baud = 1;
            DONE:     valid = 1;
        endcase
    end

`ifdef BENCH
    reg [8*10:1] state_name;
    always @(*) case (state)
        IDLE: state_name = "IDLE";   START: state_name = "START";  START_OK: state_name = "START_OK";
        BITS: state_name = "BITS";   SAMPLE: state_name = "SAMPLE"; CHECK: state_name = "CHECK";
        STOP: state_name = "STOP";   DONE: state_name = "DONE";
    endcase
`endif
endmodule
