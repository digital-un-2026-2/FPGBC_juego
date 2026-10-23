// Ciclo de bus de Game Boy en el conector del cartucho.
// Solo interconecta camino de datos y control.
//   Camino de datos: registros de direccion, dato a escribir y dato leido;
//   registro de salida para /WR, /RD, DIR y el manejo de D (evita glitches
//   de la logica combinacional del control en los pines); contador de tiempos con su multiplexor de carga.
// Tiempos en ciclos de reloj (a 40 MHz, 1 ciclo = 25 ns).
module cart_bus #(
    parameter DIR_WRITE = 1'b1,
    parameter T_SETUP   = 8,      // 200 ns: direccion/dato estables antes del pulso
    parameter T_PULSE   = 20,     // 500 ns: ancho de /WR o /RD
    parameter T_HOLD    = 8       // 200 ns: dato sostenido despues de /WR
) (
    input  wire        clk,
    input  wire        rst,

    // Interfaz interna
    input  wire        start,
    input  wire        is_write,
    input  wire [15:0] addr,
    input  wire [7:0]  wdata,
    output wire [7:0]  rdata,
    output wire        busy,

    // Pines hacia el conector
    output wire [15:0] cart_a,
    inout  wire [7:0]  cart_d,
    output wire        cart_wr_n,
    output wire        cart_rd_n,
    output wire        cart_cs_n,
    output wire        data_dir
);
    wire       w_ld_in, w_ld_t, w_dec_t, w_z_t, w_ld_rdata, w_drive;
    wire       c_wr_n, c_rd_n, c_drive, c_dir;   // salidas del control antes del registro
    wire [1:0] w_sel_t;
    wire [7:0] w_t_value, w_wdata;

    assign cart_cs_n = 1'b1;                          // /CS solo selecciona la RAM del cartucho
    assign cart_d    = w_drive ? w_wdata : 8'bz;

    registro #(.W(16)) reg_addr  (.clk(clk), .rst(1'b0), .clr(1'b0), .ld(w_ld_in), .d(addr),     .q(cart_a));
    registro #(.W(8))  reg_wdata (.clk(clk), .rst(1'b0), .clr(1'b0), .ld(w_ld_in), .d(wdata),    .q(w_wdata));
    registro #(.W(8))  reg_rdata (.clk(clk), .rst(rst),  .clr(1'b0), .ld(w_ld_rdata), .d(cart_d), .q(rdata));

    mux4 #(.W(8)) mux_t (.sel(w_sel_t), .in0(T_SETUP[7:0]), .in1(T_PULSE[7:0]), .in2(T_HOLD[7:0]),
                         .in3(8'd0), .out(w_t_value));
    contador_dec #(.W(8)) cont_t (.clk(clk), .ld(w_ld_t), .dec(w_dec_t), .d(w_t_value), .q(), .z(w_z_t));

    control_cart_bus #(.DIR_WRITE(DIR_WRITE)) control0 (
        .clk(clk), .rst(rst), .start(start), .is_write(is_write), .z_t(w_z_t),
        .ld_in(w_ld_in), .ld_t(w_ld_t), .sel_t(w_sel_t), .dec_t(w_dec_t), .ld_rdata(w_ld_rdata),
        .wr_n(c_wr_n), .rd_n(c_rd_n), .drive_d(c_drive), .data_dir(c_dir), .busy(busy));

    registro #(.W(4), .RST_VAL({1'b1, 1'b1, 1'b0, ~DIR_WRITE})) reg_salida (
        .clk(clk), .rst(rst), .clr(1'b0), .ld(1'b1),
        .d({c_wr_n, c_rd_n, c_drive, c_dir}), .q({cart_wr_n, cart_rd_n, w_drive, data_dir}));
endmodule
