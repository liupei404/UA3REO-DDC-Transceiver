module bus_interface(
clk_in,
bus_comm_data_in,
bus_comm_in_ready,
bus_comm_request_new,
bus_comm_active,
adcclk_in,
ADC_OTR,
DAC_OTR,
ADC_IN,
keyb_1,
keyb_2,
keyb_3,
keyb_4,
enc_sw,
enc_a1,
enc_a2,
SPEC_I,
SPEC_Q,
VOICE_I,
VOICE_Q,
bus_stream_data_in,
bus_stream_in_valid,
iq_clock,

bus_comm_data_out,
freq_out,
preamp_enable,
rx,
tx,
audio_clk_en,
TX_I,
TX_Q,
bus_stream_data_out,
bus_stream_enabled
);

input clk_in;
input unsigned [7:0] bus_comm_data_in;
input bus_comm_in_ready;
input bus_comm_request_new;
input bus_comm_active;
input adcclk_in;
input ADC_OTR;
input DAC_OTR;
input signed [11:0] ADC_IN;
input keyb_1;
input keyb_2;
input keyb_3;
input keyb_4;
input enc_sw;
input enc_a1;
input enc_a2;
input signed [15:0] SPEC_I;
input signed [15:0] SPEC_Q;
input signed [15:0] VOICE_I;
input signed [15:0] VOICE_Q;
input unsigned [7:0] bus_stream_data_in;
input bus_stream_in_valid;
input iq_clock;

output reg unsigned [7:0] bus_comm_data_out;
output reg unsigned [21:0] freq_out=620407;
output reg preamp_enable=0;
output reg rx=1;
output reg tx=0;
output reg audio_clk_en=1;
output reg signed [15:0] TX_I=0;
output reg signed [15:0] TX_Q=0;
output reg unsigned [7:0] bus_stream_data_out=0;
output reg bus_stream_enabled=1;

reg [7:0] read_index='d0;
reg [7:0] write_index='d0;
reg [7:0] command='d0;
reg signed [11:0] ADC_MIN;
reg signed [11:0] ADC_MAX;
reg ADC_MIN_RESET;
reg ADC_MAX_RESET;

integer keyb1_unbounce_counter = 0;
integer keyb2_unbounce_counter = 0;
integer keyb3_unbounce_counter = 0;
integer keyb4_unbounce_counter = 0;
integer enc_sw_unbounce_counter = 0;
integer enc_unbounce_counter = 0;
reg keyb1_unbounced_value = 0;
reg keyb2_unbounced_value = 0;
reg keyb3_unbounced_value = 0;
reg keyb4_unbounced_value = 0;
reg enc_sw_unbounced_value = 0;
reg signed [3:0] enc_value = 0; // -7 left 7 right
reg enc_a1_prev = 0;
reg enc_value_reset = 0;

always @ (posedge bus_comm_in_ready or negedge bus_comm_active)
begin
	if(!bus_comm_active)
		read_index=0;
	else
	if(bus_comm_in_ready==1)
	begin
		read_index=read_index+1'd1;
		
		if(read_index==1)
		begin
			command[7:0] = bus_comm_data_in[7:0];
		end
		else
		begin
			if(command=='d31) //set Preamp & TX
			begin
				preamp_enable=bus_comm_data_in[1:1];
				if(bus_comm_data_in[0:0]==1)
				begin
					tx=1;
					rx=0;
				end
				else
				begin
					tx=0;
					rx=1;
				end
			end
			else if(command=='d32) //set Freq
			begin
				if(read_index==2)
					freq_out[21:16]=bus_comm_data_in[5:0];
				if(read_index==3)
					freq_out[15:8]=bus_comm_data_in[7:0];
				if(read_index==4)
					freq_out[7:0]=bus_comm_data_in[7:0];
			end
			else if(command=='d33) //set AUDIO PLL enabled
			begin
				if(bus_comm_data_in[0:0]==1)
					audio_clk_en=1'd1;
				else
					audio_clk_en=1'd0;
			end
		end
	end
end

//передача 
always @ (posedge bus_comm_request_new or negedge bus_comm_active)
begin
	if(!bus_comm_active)
	begin
		write_index = 0;
		ADC_MIN_RESET = 0;
		ADC_MAX_RESET = 0;
		enc_value_reset = 0;
	end
	else
		begin
		write_index=write_index+1'd1;
		
		if(command==1) //get OTR
		begin
			bus_comm_data_out[0:0]=ADC_OTR;
			bus_comm_data_out[1:1]=DAC_OTR;
		end
		else if(command==2) //get KEYS
		begin
			bus_comm_data_out[0:0]=keyb1_unbounced_value;
			bus_comm_data_out[1:1]=keyb2_unbounced_value;
			bus_comm_data_out[2:2]=keyb3_unbounced_value;
			bus_comm_data_out[3:3]=keyb4_unbounced_value;
		end
		else if(command==3) //get ADC MIN
		begin
			if(write_index==1)
				bus_comm_data_out[7:0]=ADC_MIN[7:0];
			else if(write_index==2)
				bus_comm_data_out[3:0]=ADC_MIN[11:8];
			ADC_MIN_RESET=1;
		end
		else if(command==4) //get ADC MAX
		begin
			if(write_index==1)
				bus_comm_data_out[7:0]=ADC_MAX[7:0];
			if(write_index==2)
				bus_comm_data_out[3:0]=ADC_MAX[11:8];
			ADC_MAX_RESET=1;
		end
		else if(command==5) //get ENC
		begin
			bus_comm_data_out[3:0]=enc_value[3:0];
			bus_comm_data_out[4:4]=enc_sw_unbounced_value;
			enc_value_reset = 1;
		end
		else
			bus_comm_data_out[7:0]=bus_comm_data_in[7:0];
	end
end

always @ (posedge adcclk_in)
begin
	//KEYBOARD
	if(keyb_1==keyb1_unbounced_value)
	begin
		if (keyb1_unbounce_counter<'d10000)
			keyb1_unbounce_counter = keyb1_unbounce_counter + 'd1;
		else
			keyb1_unbounced_value = !keyb1_unbounced_value;
	end
	else
		keyb1_unbounce_counter = 'd0;
	
	if(keyb_2==keyb2_unbounced_value)
	begin
		if (keyb2_unbounce_counter<'d10000)
			keyb2_unbounce_counter = keyb2_unbounce_counter + 'd1;
		else
			keyb2_unbounced_value = !keyb2_unbounced_value;
	end
	else
		keyb2_unbounce_counter = 'd0;
		
	if(keyb_3==keyb3_unbounced_value)
	begin
		if (keyb3_unbounce_counter<'d10000)
			keyb3_unbounce_counter = keyb3_unbounce_counter + 'd1;
		else
			keyb3_unbounced_value = !keyb3_unbounced_value;
	end
	else
		keyb3_unbounce_counter = 'd0;
		
	if(keyb_4==keyb4_unbounced_value)
	begin
		if (keyb4_unbounce_counter<'d10000)
			keyb4_unbounce_counter = keyb4_unbounce_counter + 'd1;
		else
			keyb4_unbounced_value = !keyb4_unbounced_value;
	end
	else
		keyb4_unbounce_counter = 'd0;
	
	//ENCODER
	if(enc_sw==enc_sw_unbounced_value)
	begin
		if (enc_sw_unbounce_counter<'d10000)
			enc_sw_unbounce_counter = enc_sw_unbounce_counter + 'd1;
		else
			enc_sw_unbounced_value = !enc_sw_unbounced_value;
	end
	else
		enc_sw_unbounce_counter = 'd0;
	
	if(enc_value_reset)
	begin
		enc_value = 'd0;
	end
	
	if (enc_unbounce_counter<'d10000)
			enc_unbounce_counter = enc_unbounce_counter + 'd1;
	else
	begin
		if(enc_a1!=enc_a1_prev)
		begin
			enc_a1_prev=enc_a1;
			if(enc_a1 && !enc_value_reset)
			begin
				if(enc_a2)
					enc_value = enc_value - 1'd1;
				else
					enc_value = enc_value + 1'd1;
			end
		end
		enc_unbounce_counter = 'd0;
	end
	
	//ADC MIN-MAX
	if(ADC_MIN_RESET==1)
	begin
		ADC_MIN=12'd2000;
	end
	if(ADC_MAX_RESET==1)
	begin
		ADC_MAX=-12'd2000;
	end
	if(ADC_MAX<ADC_IN)
	begin
		ADC_MAX=ADC_IN;
	end
	if(ADC_MIN>ADC_IN)
	begin
		ADC_MIN=ADC_IN;
	end
end

reg [7:0] iq_index='d0;
reg iq_clock_prev='d0;
reg stream_reseted='d0;

always @ (posedge clk_in)
begin
	if(!iq_clock_prev && iq_clock && !stream_reseted)
	begin
		stream_reseted=1;
		iq_clock_prev=iq_clock;
	end
	else
	begin
	 stream_reseted=0;
	 iq_clock_prev=iq_clock;
	end
end

always @ (posedge bus_stream_in_valid or posedge stream_reseted)
begin
	if(stream_reseted)
	begin
		iq_index=0;
		//bus_stream_data_out[7:0]='d255;
		bus_stream_data_out[7:0]=VOICE_I[15:8];
	end
	else
	if(bus_stream_in_valid)
	begin
		if(iq_index < 7)
		begin
			iq_index=iq_index+1'd1;
		
			if(iq_index==1)
			begin
				TX_Q[15:8]=bus_stream_data_in[7:0];
				bus_stream_data_out[7:0]=VOICE_I[7:0];
				//bus_stream_data_out[7:0]='d20;
			end
			else if(iq_index==2)
			begin
				TX_Q[7:0]=bus_stream_data_in[7:0];
				bus_stream_data_out[7:0]=VOICE_Q[15:8];
				//bus_stream_data_out[7:0]='d30;
			end
			
			else if(iq_index==3)
			begin
				TX_I[15:8]=bus_stream_data_in[7:0];
				bus_stream_data_out[7:0]=VOICE_Q[7:0];
				//bus_stream_data_out[7:0]='d40;
			end
			
			else if(iq_index==4)
			begin
				TX_I[7:0]=bus_stream_data_in[7:0];
				bus_stream_data_out[7:0]=SPEC_I[15:8];
				//bus_stream_data_out[7:0]='d50;
			end
			
			else if(iq_index==5)
			begin
				bus_stream_data_out[7:0]=SPEC_I[7:0];
				//bus_stream_data_out[7:0]='d60;
			end
			
			else if(iq_index==6)
			begin
				bus_stream_data_out[7:0]=SPEC_Q[15:8];
				//bus_stream_data_out[7:0]='d70;
			end
			
			else if(iq_index==7)
			begin
				bus_stream_data_out[7:0]=SPEC_Q[7:0];
				//bus_stream_data_out[7:0]='d80;
			end
		end

	end
end

endmodule
