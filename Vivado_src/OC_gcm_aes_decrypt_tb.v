/////////////////////////////////////////////////////////////////////
////                                                             ////
////  				Test Bench for GCM-AES                       ////
////                                                             ////
////                                                             ////
////  Author: Tariq Bashir Ahmad and Guy Hutchison               ////
////          tariq.bashir@gmail.com                             ////
////          ghutchis@gmail.com                                 ////
////                                                             ////
////  Downloaded from: http://www.opencores.org/				  ////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2010 	 Tariq Bashir Ahmad and 			 ////	
////                         Guy Hutchison						 ////
////                         http://www.ecs.umass.edu/~tbashir   ////
////                                                			 ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////



`timescale 1ns / 1ps
`define SIZE 128

module test_bench;

  // Inputs
    reg clk = 0;
    reg rst = 0;
    reg [`SIZE-1:0] dii_data = 0;
    reg [3:0]       dii_data_size = 0;
    reg             dii_data_vld = 0;
    reg             dii_data_type = 0;
    reg             dii_last_word = 0;
    
    reg [`SIZE-1:0] cii_K = 0;
    reg             cii_ctl_vld = 0;
    reg             cii_IV_vld = 0;
    
    
    // Outputs
    wire                  dii_data_not_ready;
    wire [`SIZE-1:0]      Out_data;
    wire                  Out_vld;
    wire                  Tag_vld;
    wire [3:0]            Out_data_size;
    wire                  Out_last_word;
  
    gcm_aes_v0_decrypt uut (
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
    
    always #5 clk = ~clk;
    
  	initial begin
  	    // FIRST FRAME
        #50
        rst = 1;
	    #10
	    rst = 0;
	    
	    #40
	    cii_ctl_vld = 1'b1;
        // Put 128 bit Key onto cii_K bus
        cii_K       = 128'hfeffe9928665731c6d6a8f9467308308;
        
        #10
        cii_ctl_vld = 1'b0;
        
        cii_IV_vld = 1'b1;
        // Put IV onto dii_data bus
        dii_data   = 128'hcafebabefacedbaddecaf888_00000001;
        
       
        #250
        cii_IV_vld = 1'b0;

        // Put AAD onto dii_data bus
        dii_data_vld = 1'b1;
        dii_data_type = 1'b1;
        dii_data_size = 4'd15;
        dii_data      = 128'hfeedfacedeadbeeffeedfacedeadbeef;
        
        #10
        dii_data_vld = 1'b0;
        
        #80
        dii_data_vld = 1'b1;
        dii_data_size = 4'd3;
        dii_data      = 32'habaddad2;//{`SIZE{1'b1}};
        
        #10
        dii_data_vld = 1'b0;
        
        #80
        //Put Ciphertext onto dii_data bus
        dii_data_vld  = 1'b1;
        dii_data_type = 1'b0; 
        dii_data_size = 4'd15; 
        dii_data      = 128'h42831ec2217774244b7221b784d0d49c;
        
        #10
        dii_data_vld  = 1'b0;
        
        #210
        dii_data_vld  = 1'b1;
        dii_data_type = 1'b0; 
        dii_data_size = 4'd15;
        dii_data      = 128'he3aa212f2c02a4e035c17e2329aca12e;

        #10
        dii_data_vld  = 1'b0;
        
        #210
        dii_data_vld  = 1'b1;
        dii_data_type = 1'b0; 
        dii_data_size = 4'd15;
        dii_data      = 128'h21d514b25466931c7d8f6a5aac84aa05;   
        
        #10
        dii_data_vld  = 1'b0;
        
        #210
        dii_last_word = 1'b1;
        dii_data_vld  = 1'b1;
        dii_data_type = 1'b0; 
        dii_data_size = 4'd11;
        dii_data      = 128'h1ba30b396a0aac973d58e091;   
        #10
        dii_data_vld  = 1'b0;
        
        #310
        dii_last_word = 0;
        dii_data_size = 0;
        cii_K = 0;
        
        #100
        rst = 1;
	    #10
	    rst = 0;
	    #10
        $finish();

	  end

   
endmodule


/*
// Put 128 bit Key onto cii_K bus
cii_K       = 128'hfeffe9928665731c6d6a8f9467308308;

// Put IV onto dii_data bus
dii_data   = 128'hcafebabefacedbaddecaf888_00000001;

// Put AAD onto dii_data bus
dii_data      = 128'hfeedfacedeadbeeffeedfacedeadbeef; // {`SIZE{1'b1}};
dii_data      = 32'habaddad2;//{`SIZE{1'b1}};

//Put Plaintext onto dii_data bus
dii_data      = 128'hd9313225f88406e5a55909c5aff5269a;//{`SIZE{1'b1}};
dii_data      = 128'h86a7a9531534f7da2e4c303d8a318a72;//{`SIZE{1'b1}};
dii_data      = 128'h1c3c0c95956809532fcf0e2449a6b525;//{`SIZE{1'b1}};
dii_data      = 128'hb16aedf5aa0de657ba637b39;//{`SIZE{1'b1}};
*/