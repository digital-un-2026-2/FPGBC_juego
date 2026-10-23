// Receptor UART 8N1. Solo interconecta camino de datos y control.
//   Camino de datos: sincronizador, contador de tiempo de bit, contador de
//   bits y registro de desplazamiento.
module uart_rx #(
    parameter CLKS_PER_BIT = 347          // 40 MHz / 115200
) (
    input  wire       clk,
    input  wire       rst,
    input  wire       rx,
    output wire [7:0] data,
    output wire       valid
);
    // Cada bit dura: (carga + 1) ciclos contando + SAMPLE + CHECK = CLKS_PER_BIT
    localparam [15:0] FULL = CLKS_PER_BIT - 3;
    localparam [15:0] HALF = CLKS_PER_BIT / 2;

    wire w_rx, w_ld_half, w_ld_full, w_dec, w_z, w_clr_bits, w_inc_bits, w_shift, w_bits_done;
    wire [3:0]  w_bits;

    sincronizador sync0 (.clk(clk), .in(rx), .out(w_rx));

    contador_dec #(.W(16)) baud0 (.clk(clk), .ld(w_ld_half | w_ld_full), .dec(w_dec),
                                  .d(w_ld_half ? HALF : FULL), .q(), .z(w_z));

    contador_inc #(.W(4)) bits0 (.clk(clk), .rst(rst), .clr(w_clr_bits), .ld(1'b0),
                                 .inc(w_inc_bits), .d(4'd0), .q(w_bits));

    comparador #(.W(4)) comp_bits (.a(w_bits), .b(4'd8), .eq(w_bits_done));

    rsr_rx sr0 (.clk(clk), .shift(w_shift), .in_bit(w_rx), .q(data));

    control_uart_rx control0 (.clk(clk), .rst(rst), .rx(w_rx), .z_baud(w_z), .bits_done(w_bits_done),
                              .ld_half(w_ld_half), .ld_full(w_ld_full), .dec_baud(w_dec),
                              .clr_bits(w_clr_bits), .inc_bits(w_inc_bits), .shift(w_shift),
                              .valid(valid));
endmodule
