`timescale 1ns / 1ps
// Banco de pruebas: el "PC" envia comandos por UART al puente y se revisan
// las respuestas contra el modelo del cartucho.
//   iverilog -o tb.vvp tb_cart_bridge.v cart_model.v ../rtl/*.v && vvp tb.vvp

module tb_cart_bridge;
    localparam CLK_HZ = 40_000_000;
    localparam BAUD   = 4_000_000;              // rapido para que la simulacion sea corta
    localparam BIT_NS = 1_000_000_000 / BAUD;

    reg clk = 0, rst_n = 0;
    always #12.5 clk = ~clk;                    // 40 MHz

    reg  pc_tx = 1'b1;
    wire pc_rx;

    wire [15:0] a;
    wire [7:0]  d;
    wire wr_n, rd_n, cs_n, cclk, crst_n, dir, ctrl_oe;

    cart_bridge #(.CLK_HZ(CLK_HZ), .BAUD(BAUD)) dut (
        .clk(clk), .rst_n(rst_n), .uart_rx(pc_tx), .uart_tx(pc_rx),
        .cart_a(a), .cart_d(d), .cart_wr_n(wr_n), .cart_rd_n(rd_n), .cart_cs_n(cs_n),
        .cart_clk(cclk), .cart_rst_n(crst_n), .data_dir(dir), .ctrl_oe(ctrl_oe)
    );

    cart_model cart (.a(a), .d(d), .wr_n(wr_n), .rd_n(rd_n));

    integer errors = 0;

    // Nunca deben estar /RD y /WR activos a la vez
    always @(*) if (!wr_n && !rd_n) begin
        $display("ERROR: /RD y /WR bajos al mismo tiempo en t=%0t", $time);
        errors = errors + 1;
    end

    // ---------- UART del lado del PC ----------
    task send(input [7:0] b);
        integer k;
        begin
            pc_tx = 0; #(BIT_NS);
            for (k = 0; k < 8; k = k + 1) begin pc_tx = b[k]; #(BIT_NS); end
            pc_tx = 1; #(BIT_NS);
        end
    endtask

    task recv(output [7:0] b);
        integer k;
        begin
            @(negedge pc_rx);
            #(BIT_NS + BIT_NS / 2);
            for (k = 0; k < 8; k = k + 1) begin b[k] = pc_rx; #(BIT_NS); end
        end
    endtask

    task expect_byte(input [7:0] want, input [8*24-1:0] what);
        reg [7:0] got;
        begin
            recv(got);
            if (got !== want) begin
                $display("ERROR %0s: esperaba %02h, llego %02h", what, want, got);
                errors = errors + 1;
            end
        end
    endtask

    task cmd_w(input [15:0] addr, input [7:0] data);
        begin send("W"); send(addr[15:8]); send(addr[7:0]); send(data); expect_byte("K", "ack W"); end
    endtask

    task cmd_p(input [15:0] addr, input [7:0] data);
        begin send("P"); send(addr[15:8]); send(addr[7:0]); send(data); expect_byte("K", "ack P"); end
    endtask

    integer i;
    reg [7:0] b;

    initial begin
        $dumpfile("tb_cart_bridge.vcd");
        $dumpvars(0, tb_cart_bridge);

        // Contenido inicial conocido para la prueba de lectura
        cart.rom[16'h0104] = 8'hCE; cart.rom[16'h0105] = 8'hED;
        cart.rom[16'h0106] = 8'h66; cart.rom[16'h0107] = 8'h66;

        #200 rst_n = 1;
        #500;

        // 1. Enlace
        send("V");
        expect_byte("F", "V0"); expect_byte("P", "V1"); expect_byte("G", "V2");
        expect_byte("B", "V3"); expect_byte("C", "V4"); expect_byte("1", "V5");
        $display("OK  enlace 'V'");

        // 2. Lectura de la cabecera (inicio del logo de Nintendo)
        send("R"); send(8'h01); send(8'h04); send(8'h00); send(8'h04);
        expect_byte(8'hCE, "logo0"); expect_byte(8'hED, "logo1");
        expect_byte(8'h66, "logo2"); expect_byte(8'h66, "logo3");
        $display("OK  lectura de cabecera");

        // 3. ID de la flash (autoselect) y salida con F0
        cmd_w(16'h0AAA, 8'hAA); cmd_w(16'h0555, 8'h55); cmd_w(16'h0AAA, 8'h90);
        send("R"); send(8'h00); send(8'h00); send(8'h00); send(8'h03);
        expect_byte(8'h01, "ID fabricante"); expect_byte(8'h00, "ID relleno");
        expect_byte(8'h7E, "ID dispositivo");
        cmd_w(16'h0000, 8'hF0);
        $display("OK  ID de la flash");

        // 4. Borrado de chip
        cmd_w(16'h0AAA, 8'hAA); cmd_w(16'h0555, 8'h55); cmd_w(16'h0AAA, 8'h80);
        cmd_w(16'h0AAA, 8'hAA); cmd_w(16'h0555, 8'h55); cmd_w(16'h0AAA, 8'h10);
        if (cart.rom[16'h0104] !== 8'hFF) begin $display("ERROR borrado"); errors = errors + 1; end
        else $display("OK  borrado de chip");

        // 5. Programar en el banco 2 (direccion fisica 0x8000..) por la ventana 0x4000
        cmd_w(16'h2000, 8'h02); cmd_w(16'h3000, 8'h00);
        for (i = 0; i < 8; i = i + 1) cmd_p(16'h4000 + i, 8'hA0 + i);
        if (cart.rom[18'h08003] !== 8'hA3) begin $display("ERROR programacion"); errors = errors + 1; end

        // 6. Leer de vuelta por la misma ventana
        send("R"); send(8'h40); send(8'h00); send(8'h00); send(8'h08);
        for (i = 0; i < 8; i = i + 1) expect_byte(8'hA0 + i, "readback");
        $display("OK  programacion y verificacion en banco 2");

        // 6b. Un dato igual a F0 (el comando de reset) tambien debe grabarse
        cmd_p(16'h4020, 8'hF0);
        if (cart.rom[18'h08020] !== 8'hF0) begin $display("ERROR dato F0"); errors = errors + 1; end
        else $display("OK  dato 0xF0 programado");

        // 7. Rafaga de comandos P sin esperar respuestas (usa la FIFO)
        fork
            for (i = 0; i < 16; i = i + 1) begin
                send("P"); send(8'h40); send(8'h10 + i); send(8'h30 + i);
            end
            repeat (16) expect_byte("K", "rafaga P");
        join
        if (cart.rom[18'h0801F] !== 8'h3F) begin $display("ERROR rafaga"); errors = errors + 1; end
        else $display("OK  rafaga de 16 comandos P");

        if (errors == 0) $display("RESULTADO: todas las pruebas pasaron");
        else             $display("RESULTADO: %0d errores", errors);
        $finish;
    end

    initial begin
        #50_000_000;
        $display("ERROR: tiempo agotado");
        $finish;
    end
endmodule
