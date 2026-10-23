// Registro de desplazamiento de 10 bits para transmitir: carga
// {parada, dato, inicio} y desplaza a la derecha rellenando con 1.
// La linea serial es siempre q[0]; en reposo vale 1.
module rsr_tx (
    input  wire       clk,
    input  wire       rst,
    input  wire       ld,
    input  wire       shift,
    input  wire [7:0] data,
    output wire       tx
);
    reg [9:0] q;
    assign tx = q[0];
    always @(posedge clk)
        if (rst)        q <= 10'h3FF;
        else if (ld)    q <= {1'b1, data, 1'b0};
        else if (shift) q <= {1'b1, q[9:1]};
endmodule
