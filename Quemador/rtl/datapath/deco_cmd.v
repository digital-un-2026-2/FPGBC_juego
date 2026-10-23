// Decodificador del byte de comando: dice que comando es y cuantos
// argumentos espera.
module deco_cmd (
    input  wire [7:0] cmd,
    output wire       is_v,
    output wire       is_r,
    output wire       is_w,
    output wire       is_p,
    output wire [2:0] nargs
);
    assign is_v  = (cmd == "V");
    assign is_r  = (cmd == "R");
    assign is_w  = (cmd == "W");
    assign is_p  = (cmd == "P");
    assign nargs = is_r ? 3'd4 : 3'd3;     // R: aH aL nH nL;  W y P: aH aL d
endmodule
