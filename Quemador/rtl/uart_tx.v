// Transmisor UART 8N1. Solo interconecta camino de datos y control.
//   Camino de datos: registro del byte, registro de desplazamiento de 10 bits,
//   contador de tiempo de bit y contador de bits.
module uart_tx #(
    parameter CLKS_PER_BIT = 347
) (
    input  wire       clk,
    input  wire       rst,
    input  wire [7:0] data,
    input  wire       start,
    output wire       tx,
    output wire       busy
);
    localparam [15:0] FULL = CLKS_PER_BIT - 3;   // SEND (carga+1) + SHIFT + CHECK

    wire w_ld_dato, w_ld_sr, w_shift, w_ld_baud, w_dec, w_z, w_clr_bits, w_inc_bits, w_bits_done;
    wire [3:0] w_bits;
    wire [7:0] w_dato;

    // El byte se captura en el mismo flanco en que llega start
    registro #(.W(8)) reg_dato (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_dato), .d(data), .q(w_dato));

    rsr_tx sr0 (.clk(clk), .rst(rst), .ld(w_ld_sr), .shift(w_shift), .data(w_dato), .tx(tx));

    contador_dec #(.W(16)) baud0 (.clk(clk), .ld(w_ld_baud), .dec(w_dec), .d(FULL), .q(), .z(w_z));

    contador_inc #(.W(4)) bits0 (.clk(clk), .rst(rst), .clr(w_clr_bits), .ld(1'b0),
                                 .inc(w_inc_bits), .d(4'd0), .q(w_bits));

    comparador #(.W(4)) comp_bits (.a(w_bits), .b(4'd10), .eq(w_bits_done));

    control_uart_tx control0 (.clk(clk), .rst(rst), .start(start), .z_baud(w_z), .bits_done(w_bits_done),
                              .ld_dato(w_ld_dato), .ld_sr(w_ld_sr), .shift(w_shift), .ld_baud(w_ld_baud), .dec_baud(w_dec),
                              .clr_bits(w_clr_bits), .inc_bits(w_inc_bits), .busy(busy));
endmodule
