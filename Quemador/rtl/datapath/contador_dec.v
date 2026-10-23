// Contador descendente con carga. z = 1 cuando la cuenta vale 0.
module contador_dec #(parameter W = 16) (
    input  wire         clk,
    input  wire         ld,
    input  wire         dec,
    input  wire [W-1:0] d,
    output reg  [W-1:0] q,
    output wire         z
);
    assign z = (q == {W{1'b0}});
    always @(posedge clk)
        if (ld)       q <= d;
        else if (dec) q <= q - 1'b1;
endmodule
