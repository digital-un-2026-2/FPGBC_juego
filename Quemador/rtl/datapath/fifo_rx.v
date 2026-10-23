// FIFO de 512 bytes para los bytes recibidos por la UART.
// dout muestra (con un ciclo de retardo) el byte mas antiguo; pop lo descarta.
module fifo_rx (
    input  wire       clk,
    input  wire       rst,
    input  wire       push,
    input  wire [7:0] din,
    input  wire       pop,
    output reg  [7:0] dout,
    output wire       empty
);
    reg [7:0] mem [0:511];
    reg [8:0] wr_ptr, rd_ptr;

    assign empty = (wr_ptr == rd_ptr);

    always @(posedge clk) begin
        if (push) mem[wr_ptr] <= din;
        dout <= mem[rd_ptr];
    end

    always @(posedge clk)
        if (rst) begin
            wr_ptr <= 9'd0;
            rd_ptr <= 9'd0;
        end else begin
            if (push) wr_ptr <= wr_ptr + 1'b1;
            if (pop)  rd_ptr <= rd_ptr + 1'b1;
        end
endmodule
