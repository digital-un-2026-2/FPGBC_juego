// Multiplexor 4 a 1 de W bits.
module mux4 #(parameter W = 8) (
    input  wire [1:0]   sel,
    input  wire [W-1:0] in0,
    input  wire [W-1:0] in1,
    input  wire [W-1:0] in2,
    input  wire [W-1:0] in3,
    output reg  [W-1:0] out
);
    always @(*)
        case (sel)
            2'd0: out = in0;
            2'd1: out = in1;
            2'd2: out = in2;
            default: out = in3;
        endcase
endmodule
