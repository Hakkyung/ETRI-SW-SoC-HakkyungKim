/*
 *  cnnip_ctrlr.sv -- CNN IP controller
 *  ETRI <SW-SoC AI Deep Learning HW Accelerator RTL Design> course material
 *
 *  first draft by Junyoung Park
 */

module cnnip_ctrlr #
(
  parameter DATA_WIDTH = 32,
  parameter ADDR_WIDTH = 12,
  parameter latency = 1
)
(
  // clock and reset signals from domain a
  input wire clk_a,
  input wire arstz_aq,

  // internal memories
  cnnip_mem_if.master to_input_mem,     //master
  cnnip_mem_if.master to_weight_mem,    //master
  cnnip_mem_if.master to_feature_mem,   //master

  // configuration registers
  input wire         CMD_START,
  input wire   [7:0] MODE_KERNEL_SIZE,
  input wire   [7:0] MODE_KERNEL_NUMS,
  input wire   [1:0] MODE_STRIDE,
  input wire         MODE_PADDING,

  output wire        CMD_DONE,
  output wire        CMD_DONE_VALID

);


 
//--------------------------------------------------------------------------------

    localparam image = 5;
    localparam bram0_iaddr = 12'h100;
    localparam bram1_iaddr = 12'h200;
    localparam bram2_iaddr = 12'h300; 
    localparam D_DATA_WIDTH = DATA_WIDTH*DATA_WIDTH-1;
	localparam IDLE = 2'b00, WAIT = 2'b01, DONE = 2'b10;
		//cnnip_mem_if.master to_input/weight/feature_mem
		 reg [DATA_WIDTH-1:0] xdata;
		 reg [DATA_WIDTH-1:0] wdata;
		 reg ren,	wen;
		 reg [ADDR_WIDTH-1:0] xaddr;
		 reg [ADDR_WIDTH-1:0] waddr;
		 reg [ADDR_WIDTH-1:0] raddr;
		 reg [DATA_WIDTH-1:0] odata;
		 

    reg valid;
	reg d1_valid;
	reg d2_valid;
    reg d3_valid;
    reg ovalid;
	
    wire [D_DATA_WIDTH-1:0] multi;
	wire [DATA_WIDTH-1:0] sat_multi;
	wire [DATA_WIDTH-1:0] sat_sum;
	reg [DATA_WIDTH:0] sum;
    
    reg lcount;
	reg [4 : 0] count;
	reg [7 : 0] xcount;
    reg [7 : 0] ycount;
    reg [3 : 0] jcount;
    reg [3 : 0] icount;
    
	reg [1:0] cstate;
	reg [1:0] nstate;
	reg wait_done;
	

//--------------------FSM-------------------------------


	always@(*)
	begin
	case(cstate)
	IDLE : nstate = (CMD_START==1) ? WAIT:IDLE;
	WAIT : nstate = (wait_done==1) ? DONE:WAIT;
	DONE : nstate = IDLE;
	default nstate = IDLE;
	endcase
	end

	always@(posedge clk_a, negedge arstz_aq)
	begin
	if(!arstz_aq)
		cstate<=IDLE;
	else
		cstate<=nstate;
	end

	
	always@(posedge clk_a, negedge arstz_aq)
	begin
	if(!arstz_aq)
		wait_done<=0;
    else if((ycount==0)&&(xcount==0)&&(icount==0)
			&&(cstate==WAIT)&& (d3_valid==1))	// raddr>>2
		wait_done<=1;
	else wait_done<=0;
	end

//--------------------------DONE state-----------------------------

	assign CMD_DONE = (cstate==DONE);
	assign CMD_DONE_VALID = (cstate==DONE);

//--------------------------Address Generater----------------------------

    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            lcount <= 0;
        else if((cstate==IDLE)||(cstate==DONE))
			lcount <= 0;
		else if(lcount == latency)
		    lcount <= 0;
		else lcount <= lcount+1;
    end

    
    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            ycount <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			ycount <= 0;
        else if((ycount == ((image - MODE_KERNEL_SIZE) >> (MODE_STRIDE - 1)))
				&&(xcount == ((image - MODE_KERNEL_SIZE) >> (MODE_STRIDE - 1))) 
				&& icount == MODE_KERNEL_SIZE-1 && jcount == MODE_KERNEL_SIZE-1 && (lcount==latency))
			ycount <= 0;
		else if((xcount == ((image - MODE_KERNEL_SIZE) >> (MODE_STRIDE - 1)))
				&& icount == MODE_KERNEL_SIZE-1 && jcount == MODE_KERNEL_SIZE-1 && (lcount==latency))
			ycount <= ycount + 1;
        else ycount <= ycount;
    end
    
    
    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            xcount <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			xcount <= 0;
		else if((xcount == ((image - MODE_KERNEL_SIZE) >> (MODE_STRIDE - 1)))
				&& icount == MODE_KERNEL_SIZE-1 && jcount == MODE_KERNEL_SIZE-1 && (lcount==latency))
			xcount <= 0;
		else if(icount == MODE_KERNEL_SIZE-1 && jcount == MODE_KERNEL_SIZE-1 && (lcount==latency))
			xcount <= xcount + 1;
        else xcount <= xcount;
    end
    
    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            icount <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			icount <= 0;
        else if((icount == MODE_KERNEL_SIZE-1 && jcount == MODE_KERNEL_SIZE-1) &&(lcount==latency))
			icount <= 0;
		else if((jcount == MODE_KERNEL_SIZE-1)&&(lcount==latency))
			icount <= icount +1;
		else icount <= icount;
    end
    
    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            jcount <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			jcount <= 0;
        else if ((jcount == MODE_KERNEL_SIZE-1)&&(lcount==latency))
			jcount <= 0;
		else if (lcount==latency) jcount <= jcount + 1;
    end



//--------------------------Address-------------------------------


    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            xaddr <= bram0_iaddr;
		else if((cstate==IDLE)||(cstate==DONE))
			xaddr <= bram0_iaddr;
		else
			xaddr <= bram0_iaddr + 4*(((xcount*MODE_STRIDE)+jcount) 
					+ image*((ycount*MODE_STRIDE)+icount));
    end

    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            waddr <= bram1_iaddr;
		else if((cstate==IDLE)||(cstate==DONE))
			waddr <= bram1_iaddr;
        else if(lcount==latency)
			waddr <= bram1_iaddr + count*4;
    end

    always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            raddr <= bram2_iaddr;
		else if((cstate==IDLE)||(cstate==DONE))
			raddr <= bram2_iaddr;
        else if(valid == 1)
			raddr <= raddr + 4;
		else
			raddr <= raddr;
    end


//--------------------------Calculation-------------------------------


	assign multi = xdata * wdata;
	assign sat_multi = multi[DATA_WIDTH-1:0];

	always@(posedge clk_a, negedge arstz_aq)
	begin
		if(!arstz_aq)
			sum <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			sum <= 0;
		else if(d3_valid)
			sum<= sat_multi;
		else if(count < MODE_KERNEL_SIZE * MODE_KERNEL_SIZE)
			sum <= sat_sum + sat_multi;
		else
			sum <= 0;
	end

	assign sat_sum[DATA_WIDTH-1:0] = sum[DATA_WIDTH-1:0];
   

//--------------------------Ovalid signal-------------------------------

	always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            ren = 0;
		else if((cstate==IDLE)||(cstate==DONE))
			ren <= 0;
        else ren <= 1;
	end

	always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            wen = 0;
		else if(ovalid||d3_valid)
			wen <= 1;
        else wen <= 0;
	end
	
	always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            count <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			count <= 0;
        else if ((count == (MODE_KERNEL_SIZE*MODE_KERNEL_SIZE)-1)&& lcount==1)
			count <= 0;
		else if(lcount==latency)
		    count <= count + 1;
    end
    
	always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            valid <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			valid <= 0;
        else if ((count == (MODE_KERNEL_SIZE*MODE_KERNEL_SIZE)-1)&& lcount==1)
			valid <= 1;
		else valid <= 0;
    end
    
	always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            d1_valid <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			d1_valid <= 0;
        else
			d1_valid <= valid;
	end
	
	always@(posedge clk_a, negedge arstz_aq)
    begin
        if(!arstz_aq)
            d2_valid <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			d2_valid <= 0;
        else
			d2_valid <= d1_valid;
	end
	
    always@(posedge clk_a, negedge arstz_aq)
	begin
		if(!arstz_aq)
			d3_valid <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			d3_valid <= 0;
		else d3_valid <= d2_valid;
	end
	
	always@(posedge clk_a, negedge arstz_aq)
	begin
		if(!arstz_aq)
			ovalid <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			ovalid <= 0;
		else ovalid <= d3_valid;
	end

//--------------------------Odata-------------------------------
	
	always@(posedge clk_a, negedge arstz_aq)
	begin
		if(!arstz_aq)
			odata <= 0;
		else if((cstate==IDLE)||(cstate==DONE))
			odata <= 0;
		else if(d3_valid)
			odata <= sat_sum;
		else if(ovalid)
		    odata <= odata;
		else
			odata <= 0;
	end


//--------------------------read BRAM data-------------------------------

  always @(posedge clk_a, negedge arstz_aq) begin
    if (arstz_aq == 1'b0) xdata <= 'b0;
    else if(to_input_mem.valid) xdata <= to_input_mem.dout;
    else xdata <= 'b0;
  end
  
  always @(posedge clk_a, negedge arstz_aq) begin
    if (arstz_aq == 1'b0) wdata <= 'b0;
    else if(to_weight_mem.valid) wdata <= to_weight_mem.dout;
    else wdata <= 'b0;
  end
  
//--------------------------connection-------------------------------	
    always @(posedge clk_a, negedge arstz_aq) begin
    if (arstz_aq == 1'b0) to_input_mem.en <= 0;
    else if(cstate == WAIT) to_input_mem.en <= 1;
    else to_input_mem.en <= 0;
  end
  always @(posedge clk_a, negedge arstz_aq) begin
    if (arstz_aq == 1'b0) to_weight_mem.en <= 0;
    else if(cstate == WAIT) to_weight_mem.en <= 1;
    else to_weight_mem.en <= 0;
  end


  //assign to_input_mem.en   = (cstate == WAIT)? 1: 0;
  assign to_input_mem.we   = (cstate == WAIT)? 'b0: 'b1;
  assign to_input_mem.addr = xaddr;
  
  //assign to_weight_mem.en   = (cstate == WAIT)? 1: 0;
  assign to_weight_mem.we   = (cstate == WAIT)? 'b0: 'b1;
  assign to_weight_mem.addr = waddr;
  
  assign to_feature_mem.en   = (cstate == WAIT)? 'b1: 'b0;
  assign to_feature_mem.we   = (wen) ? 'b1: 'b0;
  assign to_feature_mem.addr = raddr;
  assign to_feature_mem.din  = odata;


endmodule// cnnip_ctrlr