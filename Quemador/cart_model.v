// Modelo de comportamiento del cartucho EverSave para simulacion:
// MBC5 (registro de banco) + flash S29GL064N en modo byte (comandos basicos).
// No modela tiempos reales; solo la logica que el puente debe respetar.

module cart_model #(
    parameter ROM_BYTES = 262144,       // 256 KB (16 bancos) bastan para probar
    parameter PROG_DELAY = 40           // lecturas "ocupada" tras programar un byte
) (
    input  wire [15:0] a,
    inout  wire [7:0]  d,
    input  wire        wr_n,
    input  wire        rd_n
);
    reg [7:0] rom [0:ROM_BYTES-1];
    reg [8:0] rom_bank;

    // Direccion fisica en la flash, como la arma el MBC5
    wire [22:0] phys = a[14] ? {rom_bank, a[13:0]} : {9'd0, a[13:0]};

    // Estado de comandos de la flash
    localparam F_READ = 0, F_C1 = 1, F_C2 = 2, F_PROG = 3, F_E3 = 4, F_E4 = 5, F_E5 = 6,
               F_ID = 7;
    integer fstate;
    integer busy_left;
    reg [7:0] last_prog_data;
    reg [22:0] last_prog_addr;

    integer i;
    initial begin
        for (i = 0; i < ROM_BYTES; i = i + 1) rom[i] = 8'hFF;
        rom_bank  = 9'd1;
        fstate    = F_READ;
        busy_left = 0;
    end

    // Lectura: la ROM responde cuando A15=0 (ROM_CE_N = A15) y /RD=0
    // Sensibilidad explicita: con @(*) iverilog vigila los 256K bytes y tarda mucho
    reg [7:0] dout;
    always @(a or rd_n or rom_bank or fstate or busy_left) begin
        if (busy_left > 0 && phys == last_prog_addr)
            dout = {~last_prog_data[7], 7'h00};          // DQ7 invertido mientras programa
        else if (fstate == F_ID)
            dout = (a[7:0] == 8'h00) ? 8'h01 : (a[7:0] == 8'h02) ? 8'h7E : 8'h00;
        else
            dout = rom[phys % ROM_BYTES];
    end
    assign d = (!rd_n && !a[15]) ? dout : 8'bz;

    // Cada lectura durante la programacion consume tiempo
    always @(negedge rd_n)
        if (busy_left > 0) busy_left = busy_left - 1;

    // Escrituras: registros del MBC5 y ciclos de comando de la flash
    always @(posedge wr_n) begin
        if (!a[15]) begin
            // MBC5
            if (a[15:12] == 4'h2) rom_bank[7:0] = d;
            if (a[15:12] == 4'h3) rom_bank[8]   = d[0];

            // Flash (las direcciones de comando son las de 12 bits bajos)
            if (d == 8'hF0 && fstate != F_PROG) fstate = F_READ;   // en F_PROG, F0 es un dato
            else case (fstate)
                F_READ, F_ID: fstate = (a[11:0] == 12'hAAA && d == 8'hAA) ? F_C1 : fstate;
                F_C1:  fstate = (a[11:0] == 12'h555 && d == 8'h55) ? F_C2 : F_READ;
                F_C2:  if (a[11:0] == 12'hAAA && d == 8'hA0) fstate = F_PROG;
                       else if (a[11:0] == 12'hAAA && d == 8'h90) fstate = F_ID;
                       else if (a[11:0] == 12'hAAA && d == 8'h80) fstate = F_E3;
                       else fstate = F_READ;
                F_PROG: begin
                    rom[phys % ROM_BYTES] = rom[phys % ROM_BYTES] & d;  // la flash solo baja bits
                    last_prog_addr = phys;
                    last_prog_data = d;
                    busy_left = PROG_DELAY;
                    fstate = F_READ;
                end
                F_E3:  fstate = (a[11:0] == 12'hAAA && d == 8'hAA) ? F_E4 : F_READ;
                F_E4:  fstate = (a[11:0] == 12'h555 && d == 8'h55) ? F_E5 : F_READ;
                F_E5: begin
                    if (a[11:0] == 12'hAAA && d == 8'h10)
                        for (i = 0; i < ROM_BYTES; i = i + 1) rom[i] = 8'hFF;
                    fstate = F_READ;
                end
                default: fstate = F_READ;
            endcase
        end
    end
endmodule
