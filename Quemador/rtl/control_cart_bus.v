// Unidad de control del ciclo de bus del cartucho (maquina de Moore).
// Ver docs/diagramas.md, "Bus del cartucho".
module control_cart_bus #(
    parameter DIR_WRITE = 1'b1        // nivel de DIR que lleva el dato FPGA -> cartucho
) (
    input  wire       clk,
    input  wire       rst,
    input  wire       start,          // pedido de un ciclo
    input  wire       is_write,       // 1 = escritura, 0 = lectura
    input  wire       z_t,            // el contador de tiempo llego a 0

    output reg        ld_in,          // capturar direccion, dato y tipo de ciclo
    output reg        ld_t,           // cargar el contador de tiempo
    output reg  [1:0] sel_t,          // 0 = preparacion, 1 = pulso, 2 = sostenimiento
    output reg        dec_t,
    output reg        ld_rdata,       // capturar el dato leido
    output reg        wr_n,
    output reg        rd_n,
    output reg        drive_d,        // la FPGA maneja D0-D7
    output reg        data_dir,
    output reg        busy
);
    parameter IDLE    = 4'd0;
    parameter W_SETUP = 4'd1;
    parameter W_LDP   = 4'd2;
    parameter W_PULSE = 4'd3;
    parameter W_LDH   = 4'd4;
    parameter W_HOLD  = 4'd5;
    parameter R_SETUP = 4'd6;
    parameter R_LDP   = 4'd7;
    parameter R_PULSE = 4'd8;
    parameter R_SAMP  = 4'd9;
    parameter END_LD  = 4'd10;
    parameter END     = 4'd11;

    parameter T_SETUP = 2'd0, T_PULSE = 2'd1, T_HOLD = 2'd2;

    reg [3:0] state;

    always @(posedge clk) begin
        if (rst)
            state <= IDLE;
        else case (state)
            IDLE:    if (start) state <= is_write ? W_SETUP : R_SETUP;
            W_SETUP: if (z_t) state <= W_LDP;
            W_LDP:   state <= W_PULSE;
            W_PULSE: if (z_t) state <= W_LDH;
            W_LDH:   state <= W_HOLD;
            W_HOLD:  if (z_t) state <= END_LD;
            R_SETUP: if (z_t) state <= R_LDP;
            R_LDP:   state <= R_PULSE;
            R_PULSE: if (z_t) state <= R_SAMP;
            R_SAMP:  state <= END_LD;
            END_LD:  state <= END;
            END:     if (z_t) state <= IDLE;
            default: state <= IDLE;
        endcase
    end

    always @(*) begin
        ld_in = 0; ld_t = 0; sel_t = T_SETUP; dec_t = 0; ld_rdata = 0;
        wr_n = 1; rd_n = 1; drive_d = 0; data_dir = ~DIR_WRITE; busy = 1;
        case (state)
            IDLE:    begin busy = 0; ld_in = 1; ld_t = 1; sel_t = T_SETUP; end
            W_SETUP: begin data_dir = DIR_WRITE; drive_d = 1; dec_t = 1; end
            W_LDP:   begin data_dir = DIR_WRITE; drive_d = 1; wr_n = 0; ld_t = 1; sel_t = T_PULSE; end
            W_PULSE: begin data_dir = DIR_WRITE; drive_d = 1; wr_n = 0; dec_t = 1; end
            W_LDH:   begin data_dir = DIR_WRITE; drive_d = 1; ld_t = 1; sel_t = T_HOLD; end
            W_HOLD:  begin data_dir = DIR_WRITE; drive_d = 1; dec_t = 1; end
            R_SETUP: dec_t = 1;
            R_LDP:   begin rd_n = 0; ld_t = 1; sel_t = T_PULSE; end
            R_PULSE: begin rd_n = 0; dec_t = 1; end
            R_SAMP:  begin rd_n = 0; ld_rdata = 1; end
            END_LD:  begin ld_t = 1; sel_t = T_HOLD; end
            END:     dec_t = 1;
        endcase
    end

`ifdef BENCH
    reg [8*8:1] state_name;
    always @(*) case (state)
        IDLE: state_name = "IDLE";       W_SETUP: state_name = "W_SETUP"; W_LDP: state_name = "W_LDP";
        W_PULSE: state_name = "W_PULSE"; W_LDH: state_name = "W_LDH";     W_HOLD: state_name = "W_HOLD";
        R_SETUP: state_name = "R_SETUP"; R_LDP: state_name = "R_LDP";     R_PULSE: state_name = "R_PULSE";
        R_SAMP: state_name = "R_SAMP";   END_LD: state_name = "END_LD";   END: state_name = "END";
    endcase
`endif
endmodule
