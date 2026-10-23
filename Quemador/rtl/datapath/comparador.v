// Comparador de igualdad.
module comparador #(parameter W = 8) (
    input  wire [W-1:0] a,
    input  wire [W-1:0] b,
    output wire         eq
);
    assign eq = (a == b);
endmodule
