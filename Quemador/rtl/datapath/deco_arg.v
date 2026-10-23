// Decodificador 2 a 4 con habilitacion: activa la carga del argumento i.
module deco_arg (
    input  wire       en,
    input  wire [1:0] i,
    output wire [3:0] ld
);
    assign ld = en ? (4'b0001 << i) : 4'b0000;
endmodule
