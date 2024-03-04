`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/02/2024 01:14:37 PM
// Design Name: 
// Module Name: crypto_module
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


module crypto_module(
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
    
    localparam DATA_WIDTH = 128;
    input clk;
    input rst_n;
    // slave in interface
	output	reg				S_AXIS_TREADY;  // Ready to accept data in
	input	[DATA_WIDTH - 1: 0]		S_AXIS_TDATA;   // Data in
	input					S_AXIS_TLAST;   // Optional data in qualifier
	input					S_AXIS_TVALID;  // Data in is valid
	// master out interface
	output	reg				M_AXIS_TVALID;  // Data out is valid
	output	reg [DATA_WIDTH - 1: 0]	M_AXIS_TDATA;   // Data Out
	output	reg				M_AXIS_TLAST;   // Optional data out qualifier
	input					M_AXIS_TREADY;  // Connected slave device is ready to accept data out
    
    reg rst;
    reg [DATA_WIDTH-1:0] dii_data = 0;
    reg [3:0]       dii_data_size = 0;
    reg             dii_data_vld = 0;
    reg             dii_data_type = 0;
    reg             dii_last_word = 0;
    
    reg [DATA_WIDTH-1:0] cii_K = 0;
    reg             cii_ctl_vld = 0;
    reg             cii_IV_vld = 0;
    
    
    // Outputs
    wire                  dii_data_not_ready;
    wire [DATA_WIDTH-1:0]      Out_data;
    wire                  Out_vld;
    wire                  Tag_vld;
    wire [3:0]            Out_data_size;
    wire                  Out_last_word;
    
    gcm_aes_v0 yey (
        .clk(clk), 
        .rst(rst), 
        .dii_data(dii_data), 
        .dii_data_size(dii_data_size),
        .dii_data_vld(dii_data_vld), 
        .dii_data_type(dii_data_type), 
        .dii_data_not_ready(dii_data_not_ready), 
        .dii_last_word(dii_last_word),
        .cii_K(cii_K),
        .cii_ctl_vld(cii_ctl_vld), 
        .cii_IV_vld(cii_IV_vld), 
        .Out_data(Out_data), 
        .Out_vld(Out_vld), 
        .Tag_vld(Tag_vld),
        .Out_data_size(Out_data_size),
        .Out_last_word(Out_last_word)
    );
    
    // Define the states of state machine (one hot encoding)
	localparam IDLE  = 4'd0;
	localparam READ_KEY = 4'd1;
	localparam READ_CRYPTO_HEADER = 4'd2;
	localparam WRITE_AAD = 4'd3;
	localparam READ_PLAINTEXT = 4'd4;
	localparam WRITE_CIPHERTEXT  = 4'd5;
	localparam WRITE_TAG = 4'd6;
	localparam COMPUTE = 4'd7;

	reg [3:0] state;
    reg [3:0] next_state;
    reg data_vld_reseted = 0;
    reg last_block = 0;
    reg [3:0] last_block_size = 0;

	
	reg [DATA_WIDTH-1:0] aad = 0;
	reg [DATA_WIDTH-1:0] key = 0;
	/*
	always @(posedge clk) 
	begin
	   if (!rst_n)
	   begin
	   rst  <= 1;
	   state <= IDLE;
	   end
	end
	
	always @(posedge clk) 
	begin
        if (state == COMPUTE)    
        begin
            dii_data_vld <= 1'b0;
            S_AXIS_TREADY <= 0;
            M_AXIS_TVALID <= 0;
            
            if(data_vld_reseted)
            begin
                case(next_state)
                
                WRITE_AAD:
                begin
                    if (!dii_data_not_ready)
                    begin
                        state <= WRITE_AAD;
                    end
                end
                
                READ_PLAINTEXT:
                begin
                    if (!dii_data_not_ready)
                    begin
                        state <= READ_PLAINTEXT;
                        S_AXIS_TREADY 	<= 1; 
                    end
                end
                
                WRITE_CIPHERTEXT:
                begin
                    if (Out_vld)
                    begin
                        state <= WRITE_CIPHERTEXT;
                    end
                end
    
                WRITE_TAG:
                begin
                    if (Tag_vld)
                    begin
                        state <= WRITE_TAG;
                    end
                end
                endcase
            end
            else
            begin
                data_vld_reseted <= 1;
            end
        end
    end
    */
	
    always @(posedge clk) 
    begin 
    if (!rst_n)
    begin
        rst  <= 1;
        state <= IDLE;
    end
    else
    begin
        case (state)
    
        IDLE:
        begin
            S_AXIS_TREADY 	<= 0;
            M_AXIS_TVALID 	<= 0;
            M_AXIS_TLAST  	<= 0;
            rst = 0;
            data_vld_reseted = 0;
            last_block = 0;
            dii_last_word = 0;
            if (S_AXIS_TVALID == 1)
            begin
                state           <= READ_KEY;
                S_AXIS_TREADY 	<= 1; 
            end
        end
        
        READ_KEY:
        begin
            S_AXIS_TREADY 	<= 1;
            if (S_AXIS_TVALID == 1 && S_AXIS_TREADY == 1)
            begin
                key <= S_AXIS_TDATA;
                state <= READ_CRYPTO_HEADER;
            end
        end
        
        READ_CRYPTO_HEADER:
        begin
            if (S_AXIS_TVALID == 1 && S_AXIS_TREADY == 1)
            begin
                // Send Key into GCM Encrypt module
                cii_K <= key;
                cii_ctl_vld <= 1'b1;
                // Send IV into GCM Encrypt module
                dii_data <= S_AXIS_TDATA;
                cii_IV_vld <= 1'b1;
                S_AXIS_TREADY <= 0;
                aad[95:0] <= S_AXIS_TDATA[127:32];
                last_block_size <= S_AXIS_TDATA[111:96];
                state <= COMPUTE;
                next_state <= WRITE_AAD;
            end
        end

        WRITE_AAD:
        begin
            cii_IV_vld = 1'b0;
            dii_data_vld = 1'b1;
            dii_data_type = 1'b1;
            dii_data_size = 4'd15;
            dii_data <= aad;
            data_vld_reseted <= 0;
            state <= COMPUTE;
            next_state <= READ_PLAINTEXT;
        end
        
        READ_PLAINTEXT:
        begin
            if (S_AXIS_TVALID == 1 && S_AXIS_TREADY == 1)
            begin
                dii_data <= S_AXIS_TDATA;
                dii_data_vld = 1'b1;
                dii_data_type = 1'b0;
                if (S_AXIS_TLAST)
                begin
                    dii_data_size = last_block_size - 1;
                    last_block = 1;
                    dii_last_word = 1;
                end
                else
                begin
                    dii_data_size = 4'd15;
                end
                
                S_AXIS_TREADY <= 0;
                data_vld_reseted <= 0;
                state <= COMPUTE;
                next_state <= WRITE_CIPHERTEXT;
            end
        end
        
        WRITE_CIPHERTEXT:
        begin
            M_AXIS_TDATA <= Out_data;
            M_AXIS_TVALID <= 1;
            if (M_AXIS_TREADY == 1 && M_AXIS_TVALID == 1)
            begin 
                M_AXIS_TVALID <= 0;
                data_vld_reseted <= 0;
                state <= COMPUTE;
                if (last_block)
                begin
                    next_state <= WRITE_TAG;
                end
                else
                begin
                    next_state <= READ_PLAINTEXT;
                end
            end
        end
        
        WRITE_TAG:
        begin
            M_AXIS_TDATA <= Out_data;
            M_AXIS_TVALID <= 1;
            M_AXIS_TLAST <= 1;
            rst = 1;
            if (M_AXIS_TREADY == 1 && M_AXIS_TVALID == 1)
            begin 
                M_AXIS_TVALID <= 0;
                M_AXIS_TLAST <= 0;
               state <= IDLE;
            end
        end
        
        COMPUTE:
        begin
            if(data_vld_reseted)
            begin
                case(next_state)
                
                WRITE_AAD:
                begin
                    if (!dii_data_not_ready)
                    begin
                        state <= WRITE_AAD;
                    end
                end
                
                READ_PLAINTEXT:
                begin
                    if (!dii_data_not_ready)
                    begin
                        state <= READ_PLAINTEXT;
                        S_AXIS_TREADY 	<= 1; 
                    end
                end
                
                WRITE_CIPHERTEXT:
                begin
                    if (Out_vld)
                    begin
                        state <= WRITE_CIPHERTEXT;
                    end
                end
    
                WRITE_TAG:
                begin
                    if (Tag_vld)
                    begin
                        state <= WRITE_TAG;
                    end
                end
                endcase
            end
            else
            begin
                cii_ctl_vld <= 1'b0;
                dii_data_vld <= 1'b0;
                S_AXIS_TREADY <= 0;
                M_AXIS_TVALID <= 0;
                data_vld_reseted <= 1;
            end
        end
    endcase
    end
    end
    
endmodule

