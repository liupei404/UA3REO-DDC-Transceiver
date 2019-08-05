module i2c_Slave(CLCK, SCL, SDA, dataout, datain, datain_ready, master_request_new_byte, active);

//I2C
input CLCK;
input SCL;
inout SDA;
input [7:0] dataout;
output reg[7:0] datain = 8'b00000000;
output reg datain_ready = 1'b0;
output reg master_request_new_byte = 1'b0;
output reg active = 1'b0;

parameter slaveaddress = 7'b1110010;

//Synch SCL edge to the CPLD clock
reg [2:0] SCLSynch = 3'b000;  
always @(posedge CLCK) 
	SCLSynch <= {SCLSynch[1:0], SCL};
	
wire SCL_posedge = (SCLSynch[2:1] == 2'b01);  
wire SCL_negedge = (SCLSynch[2:1] == 2'b10);  

//Synch SDA to the CPLD clock
reg [2:0] SDASynch = 3'b000;
always @(posedge CLCK) 
	SDASynch <= {SDASynch[1:0], SDA};
	
wire SDA_synched = SDASynch[0] & SDASynch[1] & SDASynch[2];

//Detect start and stop
reg start = 1'b0;
reg stop = 1'b0;
always @(negedge SDA)
	start = SCL;
always @(posedge SDA)
	stop = SCL;

//Set cycle state 
always @(*)
	if (start)
	begin
		active = 1'b1;
	end
	else if (stop)
	begin
		active = 1'b0;	
	end
	
//Address and incomming data handling
reg [7:0] bitcount = 0;
reg [6:0] address = 7'b0000000;
reg [2:0] currvalue = 	0;
reg write_op = 1'b0;
reg addressmatch = 1'b0;
reg sdadata = 1'bZ; 
reg[7:0] datain_tmp = 8'b00000000;

always @(posedge SCL_posedge or negedge active)
	if (~active)	
	begin
		//Reset the bit counter at the end of a sequence
		bitcount = 0;
	end
	else
   begin
		bitcount = bitcount + 1'd1;
		
	   //Get the address
		if (bitcount < 8)
		begin
			address[7 - bitcount] = SDA_synched;
		end
		
		if (bitcount == 8)
		begin
			write_op = SDA_synched;
			addressmatch = (slaveaddress == address) ? 1'b1 : 1'b0;
		end
			
		if ((bitcount > 8) &&  (~write_op) && (addressmatch))
		begin
			//Receive data
			if (((bitcount - 9) - (currvalue * 9)) == 0)
			begin
				datain_tmp=8'b00000000;
			end
			else
			begin
				datain_tmp[8 - ((bitcount - 9) - (currvalue * 9))] = SDA_synched;
				//byte ready
				if(((bitcount - 9) - (currvalue * 9))==8)
					datain=datain_tmp;
			end
		end
		
		if ((bitcount > 8) && (((bitcount - 9) - (currvalue * 9)) == 0) &&  (write_op)) //master request new byte to read
			master_request_new_byte = 1;
		else
			master_request_new_byte = 0;
	end

//ACK's and out going data
always @(posedge SCL_negedge) 
begin
	datain_ready = 0;
	//ACK address received
	if ((bitcount == 8) & (addressmatch))
	begin
		sdadata = 1'b0;
		currvalue = 0;
	end
	//ACK's on byte received
	else if ((bitcount > 9) && (~write_op) && (((bitcount - 9) - (currvalue * 9)) == 8) & (addressmatch))
	begin
		sdadata = 1'b0;
		currvalue = currvalue + 1'd1;
		datain_ready = 1;
	end
	//Data
	else if ((bitcount > 8) & (write_op) & (addressmatch))
	begin
		//Send Data  
		if (((bitcount - 9) - (currvalue * 9)) == 8)
		begin
			//Release SDA so master can ACK/NAK
			sdadata = 1'bZ;
			currvalue = currvalue + 1'd1;
		end
		else 
			sdadata = dataout[7 - ((bitcount - 9) - (currvalue * 9))];
	end
	//Nothing, reading
	else sdadata = 1'bZ;
end

assign SDA = sdadata;

endmodule