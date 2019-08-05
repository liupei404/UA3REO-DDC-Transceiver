module spi_master(
clk_in,
enabled,
data_in,
continue_rw,
MISO,

data_out,
MOSI,
SCK,
CS,
busy
);

input clk_in;
input enabled;
input unsigned [7:0] data_in;
input MISO;
input continue_rw;

output reg unsigned [7:0] data_out=1;
reg unsigned [7:0] data_out_tmp=1;
output reg MOSI=1'd1;
output reg SCK=1'd1;
output reg CS=1'd1;
output reg busy=1'd0;

reg unsigned [7:0] spi_stage=0;
reg unsigned [7:0] spi_bit_position=7;

always @ (posedge clk_in)
begin
	if(enabled==1)
	begin	
		if(continue_rw==1 && busy==0)
		begin
			spi_stage=1;
			spi_bit_position=7;
			busy=1;
		end
		else 
		begin
			if(spi_stage==0) //начинаем передачу
			begin
				busy=1;
				CS=0;
				SCK=0;
				spi_bit_position=7;
				spi_stage=1;
			end
			else if(spi_stage==1) //отсылаем 1 бит
			begin
				busy=1;
				SCK=0;
				MOSI=data_in[spi_bit_position];
				spi_stage=2;
			end
			else if(spi_stage==2) //завершена отсылка 1го бита, принимаем ответ
			begin
				busy=1;
				SCK=1;
				data_out_tmp[spi_bit_position]=MISO;
				if(spi_bit_position==0)
				begin
					spi_stage=99;
					data_out[7:0]=data_out_tmp[7:0];
					busy=0;
				end
				else
				begin
					spi_bit_position=spi_bit_position-1'd1;
					spi_stage=1;
				end
			end
		end
	end
	else
	begin	
		SCK=1;
		CS=1;
		MOSI=0;
		spi_bit_position=7;
		spi_stage=0;
		busy=0;
	end
end

endmodule
