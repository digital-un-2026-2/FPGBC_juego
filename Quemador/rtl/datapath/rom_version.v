// ROM con la respuesta al comando 'V': "FPGBC1".
module rom_version (
    input  wire [2:0] i,
    output reg  [7:0] c
);
    always @(*)
        case (i)
            3'd0: c = "F";
            3'd1: c = "P";
            3'd2: c = "G";
            3'd3: c = "B";
            3'd4: c = "C";
            default: c = "1";
        endcase
endmodule
