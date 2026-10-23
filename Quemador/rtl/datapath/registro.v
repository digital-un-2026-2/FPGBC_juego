// Registro de W bits con borrado y carga sincronos.
module registro #(parameter W = 8, parameter [W-1:0] RST_VAL = {W{1'b0}}) (
    input  wire         clk,
    input  wire         rst,
    input  wire         clr,
    input  wire         ld,
    input  wire [W-1:0] d,
    output reg  [W-1:0] q
);
    always @(posedge clk)
        if (rst)        q <= RST_VAL;
        else if (clr)   q <= {W{1'b0}};
        else if (ld)    q <= d;
endmodule
