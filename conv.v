
module conv
		#(parameter image = 10, 
					bram0_iaddr = 18'h00008, bram1_iaddr = 18'h10000,
					bram2_iaddr = 18'h20000,
					data_size = 32, d_data_size = 64,
					address_size = 18)
		(
		input wire clk, rst,
		//cnnip_mem_if.master to_input/weight/feature_mem
		input wire [data_size-1:0] xdata,
		input wire [data_size-1:0] wdata,
		output reg ren,	wen,
		output reg [address_size-1:0] xaddr,
		output reg [address_size-1:0] waddr,
		output reg [address_size-1:0] raddr,
		output reg [data_size-1:0] odata,
		output reg ovalid,

		input wire cmd_start,
		input wire [7:0] mode_kernel_size,
		input wire [7:0] mode_kernel_num,	// usless yet
		input wire [1:0] mode_stride,
		input wire mode_padding,			// usless yet

		output wire cmd_done, cmd_done_valid	//declare wire
		);

    reg valid;
	reg d_valid;
	wire [7:0] kernel;
	wire [1:0] stride;

    wire [d_data_size-1:0] multi;
	wire [data_size-1:0] sat_multi;
	wire [data_size-1:0] sat_sum;
	reg [data_size:0] sum;

	reg [4:0] count;
	reg [7: 0] xcount;
    reg [7: 0] ycount;
    reg [3 : 0] jcount;
    reg [3 : 0] icount;

	parameter idle = 2'b00, work = 2'b01, done = 2'b10;
	reg [1:0] cstate;
	reg [1:0] nstate;
	reg wait_done;
	

//--------------------FSM-------------------------------


	always@(*)
	begin
	case(cstate)
	idle:nstate = (cmd_start)?work:idle;
	work:nstate = (wait_done)?done:work;
	done:nstate = idle;
	default nstate = idle;
	endcase
	end

	always@(posedge clk, negedge rst)
	begin
	if(!rst)
		cstate<=idle;
	else
		cstate<=nstate;
	end

	
	always@(posedge clk, negedge rst)
	begin
	if(!rst)
		wait_done<=0;
    else if((ycount==0)&&(xcount==0)&&(icount==0)
			&&(cstate==work)&& (d_valid==1))	// raddr>>2
		wait_done<=1;
	else wait_done<=0;
	end

//--------------------------done state-----------------------------

	assign cmd_done = (cstate==done);
	assign cmd_done_valid = (cstate==done);

//--------------------------Parameter-----------------------------

	assign kernel = mode_kernel_size;
	assign stride = mode_stride;

//--------------------------Address Generater----------------------------

    
    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            ycount <= 0;
		else if((cstate==idle)||(cstate==done))
			ycount <= 0;
        else if((ycount == ((image - kernel) >> (stride - 1)))
				&&(xcount == ((image - kernel) >> (stride - 1))) 
				&& icount == kernel-1 && jcount == kernel-1)
			ycount <= 0;
		else if((xcount == ((image - kernel) >> (stride - 1)))
				&& icount == kernel-1 && jcount == kernel-1)
			ycount <= ycount + 1;
        else ycount <= ycount;
    end
    
    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            xcount <= 0;
		else if((cstate==idle)||(cstate==done))
			xcount <= 0;
		else if((xcount == ((image - kernel) >> (stride - 1)))
				&& icount == kernel-1 && jcount == kernel-1)
			xcount <= 0;
		else if(icount == kernel-1 && jcount == kernel-1)
			xcount <= xcount + 1;
        else xcount <= xcount;
    end
    
    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            icount <= 0;
		else if((cstate==idle)||(cstate==done))
			icount <= 0;
        else if(icount == kernel-1 && jcount == kernel-1) 
			icount <= 0;
		else if(jcount == kernel-1)
			icount <= icount +1;
		else icount <= icount;
    end
    
    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            jcount <= 0;
		else if((cstate==idle)||(cstate==done))
			jcount <= 0;
        else if (jcount == kernel-1)
			jcount <= 0;
		else jcount <= jcount + 1;
    end



//--------------------------Address-------------------------------


    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            xaddr <= 0;
		else if((cstate==idle)||(cstate==done))
			xaddr <= 0;
		else
			xaddr <= ((xcount*stride)+jcount) 
					+ image*((ycount*stride)+icount);
    end

    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            waddr <= 0;
		else if((cstate==idle)||(cstate==done))
			waddr <= 0;
        else
			waddr <= count;
    end

    always@(posedge clk, negedge rst)
    begin
        if(!rst)
            raddr <= 0;
		else if((cstate==idle)||(cstate==done))
			raddr <= 0;
        else if(valid == 1)
			raddr <= raddr +1;
		else
			raddr <= raddr;
    end


//--------------------------Calculation-------------------------------
	

	assign multi = xdata * wdata;
	assign sat_multi = multi[data_size-1:0];

	always@(posedge clk, negedge rst)
	begin
		if(!rst)
			sum <= 0;
		else if((cstate==idle)||(cstate==done))
			sum <= 0;
		else if(d_valid)
			sum<= sat_multi;
		else if(count < kernel * kernel)
			sum <= sat_sum + sat_multi;
		else
			sum <= 0;
	end

	assign sat_sum[data_size-1:0] = sum[data_size-1:0];
   

//--------------------------Ovalid signal-------------------------------


	always@(posedge clk, negedge rst)
    begin
        if(!rst)
            ren = 0;
		else if((cstate==idle)||(cstate==done))
			ren <= 0;
        else ren <= 1;
	end

	always@(posedge clk, negedge rst)
    begin
        if(!rst)
            wen = 0;
		else if(valid||d_valid||ovalid)
			wen <= 1;
        else wen <= 0;
	end
	
	always@(posedge clk, negedge rst)
    begin
        if(!rst)
            count <= 0;
		else if((cstate==idle)||(cstate==done))
			count <= 0;
        else if (count == (kernel*kernel)-1)
			count <= 0;
		else count <= count + 1;
    end
    
	always@(posedge clk, negedge rst)
    begin
        if(!rst)
            valid <= 0;
		else if((cstate==idle)||(cstate==done))
			valid <= 0;
        else if (count == (kernel*kernel)-1)
			valid <= 1;
		else valid <= 0;
    end
    
	always@(posedge clk, negedge rst)
    begin
        if(!rst)
            d_valid <= 0;
		else if((cstate==idle)||(cstate==done))
			d_valid <= 0;
        else
			d_valid <= valid;
	end
	
	always@(posedge clk, negedge rst)
    begin
        if(!rst)
            ovalid <= 0;
		else if((cstate==idle)||(cstate==done))
			ovalid <= 0;
        else
			ovalid <= d_valid;
	end


//--------------------------Odata-------------------------------
	

	always@(posedge clk, negedge rst)
	begin
		if(!rst)
			odata <= 0;
		else if((cstate==idle)||(cstate==done))
			odata <= 0;
		else if(d_valid)
			odata <= sat_sum;
		else
			odata <= 0;
	end



endmodule



