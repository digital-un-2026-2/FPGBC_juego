// Puente PC <-> cartucho por UART para la FPGA de la consola FPGBC (top).
// Solo interconecta los bloques; la logica esta en los modulos.
//
// Comandos (numeros en big-endian):
//   'V'                 -> "FPGBC1"                         (prueba de enlace)
//   'R' aH aL nH nL     -> n bytes leidos desde a (n = 0 significa 65536)
//   'W' aH aL d         -> un ciclo de escritura en a; responde 'K'
//   'P' aH aL d         -> programa un byte de la flash: AAA<-AA, 555<-55,
//                          AAA<-A0, a<-d, y sondea DQ7. Responde 'K' o 'X'.
// El banco del MBC5 lo elige el PC con comandos 'W' (0x2000 y 0x3000).
module cart_bridge #(
    parameter CLK_HZ    = 40_000_000,
    parameter BAUD      = 115_200,
    parameter DIR_WRITE = 1'b1
) (
    input  wire        clk,
    input  wire        rst_n,

    input  wire        uart_rx,
    output wire        uart_tx,

    output wire [15:0] cart_a,
    inout  wire [7:0]  cart_d,
    output wire        cart_wr_n,
    output wire        cart_rd_n,
    output wire        cart_cs_n,
    output wire        cart_clk,
    output wire        cart_rst_n,  // /RESET del cartucho (el EverSave no lo usa)
    output wire        data_dir,    // DIR del level shifter de datos (SN74LVC8T245)
    output wire        ctrl_oe      // OE del level shifter de control (TXB0104, activo en alto)
);
    localparam CLKS_PER_BIT = CLK_HZ / BAUD;
    localparam [23:0] POLL_LIMIT = 24'd200_000;   // lecturas de sondeo antes de rendirse

    wire rst = ~rst_n;

    assign cart_clk   = 1'b0;
    assign cart_rst_n = 1'b1;
    assign ctrl_oe    = 1'b1;

    // ---------------------------- Cables ----------------------------
    wire [7:0]  w_rx_byte, w_fifo_out, w_cmd, w_arg0, w_arg1, w_arg2, w_arg3;
    wire [7:0]  w_rdata, w_wdata, w_tx_byte, w_ver_char;
    wire [15:0] w_dir, w_addr;
    wire [16:0] w_rem_value;
    wire [23:0] w_polls;
    wire [2:0]  w_argi, w_nargs, w_ver;
    wire [3:0]  w_ld_args;
    wire [1:0]  w_sel_addr, w_sel_wdata, w_sel_tx;
    wire w_rx_valid, w_empty, w_is_v, w_is_r, w_is_w, w_is_p, w_args_done;
    wire w_bus_busy, w_tx_busy, w_rem_z, w_dq7_ok, w_dq5, w_poll_max, w_ver_done;
    wire w_pop, w_ld_cmd, w_clr_argi, w_ld_arg, w_ld_dir, w_inc_dir, w_ld_rem, w_dec_rem;
    wire w_bus_start, w_bus_write, w_clr_poll, w_inc_poll, w_ld_dq5, w_clr_ver, w_inc_ver, w_tx_start;

    wire [15:0] w_a_arg = {w_arg0, w_arg1};

    // -------------------------- UART + FIFO --------------------------
    uart_rx #(.CLKS_PER_BIT(CLKS_PER_BIT)) rx0 (.clk(clk), .rst(rst), .rx(uart_rx),
                                                .data(w_rx_byte), .valid(w_rx_valid));

    fifo_rx fifo0 (.clk(clk), .rst(rst), .push(w_rx_valid), .din(w_rx_byte),
                   .pop(w_pop), .dout(w_fifo_out), .empty(w_empty));

    uart_tx #(.CLKS_PER_BIT(CLKS_PER_BIT)) tx0 (.clk(clk), .rst(rst), .data(w_tx_byte),
                                                .start(w_tx_start), .tx(uart_tx), .busy(w_tx_busy));

    // ------------------------ Comando y argumentos ------------------------
    registro #(.W(8)) reg_cmd (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_cmd), .d(w_fifo_out), .q(w_cmd));
    deco_cmd deco0 (.cmd(w_cmd), .is_v(w_is_v), .is_r(w_is_r), .is_w(w_is_w), .is_p(w_is_p), .nargs(w_nargs));

    contador_inc #(.W(3)) cont_argi (.clk(clk), .rst(rst), .clr(w_clr_argi), .ld(1'b0),
                                     .inc(w_ld_arg), .d(3'd0), .q(w_argi));
    comparador #(.W(3)) comp_args (.a(w_argi), .b(w_nargs), .eq(w_args_done));

    deco_arg deco_a (.en(w_ld_arg), .i(w_argi[1:0]), .ld(w_ld_args));
    registro #(.W(8)) reg_arg0 (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_args[0]), .d(w_fifo_out), .q(w_arg0));
    registro #(.W(8)) reg_arg1 (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_args[1]), .d(w_fifo_out), .q(w_arg1));
    registro #(.W(8)) reg_arg2 (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_args[2]), .d(w_fifo_out), .q(w_arg2));
    registro #(.W(8)) reg_arg3 (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_args[3]), .d(w_fifo_out), .q(w_arg3));

    // ------------------------ Comando R: lectura ------------------------
    contador_inc #(.W(16)) cont_dir (.clk(clk), .rst(rst), .clr(1'b0), .ld(w_ld_dir),
                                     .inc(w_inc_dir), .d(w_a_arg), .q(w_dir));

    assign w_rem_value = ({w_arg2, w_arg3} == 16'd0) ? 17'h10000 : {1'b0, w_arg2, w_arg3};
    contador_dec #(.W(17)) cont_rem (.clk(clk), .ld(w_ld_rem), .dec(w_dec_rem),
                                     .d(w_rem_value), .q(), .z(w_rem_z));

    // ------------------- Multiplexores hacia el bus -------------------
    mux4 #(.W(16)) mux_addr (.sel(w_sel_addr), .in0(w_dir), .in1(w_a_arg),
                             .in2(16'h0AAA), .in3(16'h0555), .out(w_addr));
    mux4 #(.W(8))  mux_wdata (.sel(w_sel_wdata), .in0(w_arg2), .in1(8'hAA),
                              .in2(8'h55), .in3(8'hA0), .out(w_wdata));

    cart_bus #(.DIR_WRITE(DIR_WRITE)) bus0 (
        .clk(clk), .rst(rst), .start(w_bus_start), .is_write(w_bus_write), .addr(w_addr),
        .wdata(w_wdata), .rdata(w_rdata), .busy(w_bus_busy),
        .cart_a(cart_a), .cart_d(cart_d), .cart_wr_n(cart_wr_n), .cart_rd_n(cart_rd_n),
        .cart_cs_n(cart_cs_n), .data_dir(data_dir));

    // --------------------- Comando P: sondeo de la flash ---------------------
    comparador #(.W(1)) comp_dq7 (.a(w_rdata[7]), .b(w_arg2[7]), .eq(w_dq7_ok));
    registro #(.W(1)) reg_dq5 (.clk(clk), .rst(rst), .clr(w_clr_poll), .ld(w_ld_dq5),
                               .d(w_rdata[5]), .q(w_dq5));
    contador_inc #(.W(24)) cont_poll (.clk(clk), .rst(rst), .clr(w_clr_poll), .ld(1'b0),
                                      .inc(w_inc_poll), .d(24'd0), .q(w_polls));
    comparador #(.W(24)) comp_poll (.a(w_polls), .b(POLL_LIMIT), .eq(w_poll_max));

    // ---------------------- Comando V y respuestas ----------------------
    contador_inc #(.W(3)) cont_ver (.clk(clk), .rst(rst), .clr(w_clr_ver), .ld(1'b0),
                                    .inc(w_inc_ver), .d(3'd0), .q(w_ver));
    comparador #(.W(3)) comp_ver (.a(w_ver), .b(3'd6), .eq(w_ver_done));
    rom_version rom0 (.i(w_ver), .c(w_ver_char));

    mux4 #(.W(8)) mux_tx (.sel(w_sel_tx), .in0(w_rdata), .in1(w_ver_char),
                          .in2("K"), .in3("X"), .out(w_tx_byte));

    // ------------------------------ Control ------------------------------
    control_bridge control0 (
        .clk(clk), .rst(rst),
        .empty(w_empty), .is_v(w_is_v), .is_r(w_is_r), .is_w(w_is_w), .is_p(w_is_p),
        .args_done(w_args_done), .bus_busy(w_bus_busy), .tx_busy(w_tx_busy), .rem_z(w_rem_z),
        .dq7_ok(w_dq7_ok), .dq5(w_dq5), .poll_max(w_poll_max), .ver_done(w_ver_done),
        .pop(w_pop), .ld_cmd(w_ld_cmd), .clr_argi(w_clr_argi), .ld_arg(w_ld_arg),
        .ld_dir(w_ld_dir), .inc_dir(w_inc_dir), .ld_rem(w_ld_rem), .dec_rem(w_dec_rem),
        .bus_start(w_bus_start), .bus_write(w_bus_write), .sel_addr(w_sel_addr),
        .sel_wdata(w_sel_wdata), .clr_poll(w_clr_poll), .inc_poll(w_inc_poll), .ld_dq5(w_ld_dq5),
        .clr_ver(w_clr_ver), .inc_ver(w_inc_ver), .tx_start(w_tx_start), .sel_tx(w_sel_tx));
endmodule
