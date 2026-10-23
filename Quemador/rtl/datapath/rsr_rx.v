// Registro de desplazamiento a la derecha con entrada serial por el MSB.
// La UART recibe primero el bit menos significativo, asi que tras 8
// desplazamientos q contiene el byte completo.
module rsr_rx (
    input  wire       clk,
    input  wire       shift,
    input  wire       in_bit,
    output reg  [7:0] q
);
    always @(posedge clk)
        if (shift) q <= {in_bit, q[7:1]};
endmodule
