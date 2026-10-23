// Dos flip-flops para llevar una entrada asincrona al dominio del reloj.
module sincronizador (
    input  wire clk,
    input  wire in,
    output reg  out
);
    reg s1;
    initial begin s1 = 1'b1; out = 1'b1; end
    always @(posedge clk) begin
        s1  <= in;
        out <= s1;
    end
endmodule
