// Contador ascendente con borrado y, opcionalmente, carga paralela.
module contador_inc #(parameter W = 8) (
    input  wire         clk,
    input  wire         rst,
    input  wire         clr,
    input  wire         ld,
    input  wire         inc,
    input  wire [W-1:0] d,
    output reg  [W-1:0] q
);
    always @(posedge clk)
        if (rst || clr) q <= {W{1'b0}};
        else if (ld)    q <= d;
        else if (inc)   q <= q + 1'b1;
endmodule
