`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/20/2024 02:57:24 PM
// Design Name: 
// Module Name: crypto_module_tb
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module crypto_module_tb(

    );
    localparam DATA_WIDTH = 128;
    reg clk = 1;
    reg rst_n = 1;
    // slave in interface
	wire				S_AXIS_TREADY;  // Ready to accept data in
	reg	[DATA_WIDTH - 1: 0]		S_AXIS_TDATA;   // Data in
	reg					S_AXIS_TLAST = 0;   // Optional data in qualifier
	reg					S_AXIS_TVALID = 0;  // Data in is valid
	// master out interface
	wire				M_AXIS_TVALID;  // Data out is valid
	wire [DATA_WIDTH - 1: 0]	M_AXIS_TDATA;   // Data Out
	wire				M_AXIS_TLAST;   // Optional data out qualifier
	reg					M_AXIS_TREADY = 1;  // Connected slave device is ready to accept data out
	
	crypto_module uut(
    // Clock and reset signals
    clk,
    rst_n,
    // AXI Stream Slave Interface
    S_AXIS_TREADY,
    S_AXIS_TDATA,
    S_AXIS_TLAST,
    S_AXIS_TVALID,
    // AXI Stream Master Interface
    M_AXIS_TVALID,
    M_AXIS_TDATA,
    M_AXIS_TLAST,
    M_AXIS_TREADY
    );
    
    always #5 clk = ~clk;
    
  	initial begin
  	#1
  	#40
    rst_n = 0;
    #10
    rst_n = 1;
    
    #40
    // SEND KEY
    S_AXIS_TDATA = 128'hee84e19cda87a76291eaaf2054aef812;
    S_AXIS_TVALID = 1;
    
    #20
    // SEND CRYPTO HEADER
    S_AXIS_TDATA = 128'h13360015f2cb949b8fb0013e_00000001;
    S_AXIS_TVALID = 1;
    
    #10
    // SEND PLAINTEXT
    S_AXIS_TDATA = 128'h4df64bff1fa11895af337eb66b66e129;
    S_AXIS_TVALID = 1;
    
    #390
    S_AXIS_TLAST = 1;
    S_AXIS_TDATA = 128'h1fda3cf888;
    
    #240
    S_AXIS_TLAST = 0;
    S_AXIS_TVALID = 0;
    
    #400
    #100
    // SEND KEY
    S_AXIS_TDATA = 128'ha3557da8c75e9dfde2ff0bd90d0156f8;
    S_AXIS_TVALID = 1;
    
    #20
    // SEND CRYPTO HEADER
    S_AXIS_TDATA = 128'h21640040428fc96f2fd10ba7_00000001;
    S_AXIS_TVALID = 1;
    
    #10
    // SEND PLAINTEXT
    S_AXIS_TDATA = 128'h1c6113d81c8e6421435aa83dd23af5f9;
    S_AXIS_TVALID = 1;
    
    #390
    S_AXIS_TDATA = 128'ha2e75920cf12ae439445e1826ff39469;
    
    #240
    S_AXIS_TDATA = 128'h0f5075af5e3abfa7472f3744176da4bb;
    
    #240
    S_AXIS_TLAST = 1;
    S_AXIS_TDATA = 128'h45bfe21d03ca895b3cfd6d1d51601351;
    
    #240
    S_AXIS_TLAST = 0;
    S_AXIS_TVALID = 0;
    #400
    $finish();
    
    
    
    
  	end
endmodule
