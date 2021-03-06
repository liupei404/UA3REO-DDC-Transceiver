module spi_interface(
clk_in,
enabled,
data_in,
continue_read,
MISO_DQ1,

data_out,
MOSI_DQ0,
SCK_C,
CS_S,
busy
);

input clk_in;
input enabled;
input unsigned [7:0] data_in;
input MISO_DQ1;
input continue_read;

output reg unsigned [7:0] data_out=1;
output reg MOSI_DQ0=1;
output reg SCK_C=1;
output reg CS_S=1;
output reg busy=0;

reg unsigned [7:0] spi_stage=0;
reg unsigned [7:0] spi_bit_position=7;

always @ (posedge clk_in)
begin
	if(enabled==1)
	begin	
		if(continue_read==1 && busy==0)
		begin
			spi_stage=1;
			spi_bit_position=7;
			busy=0;
		end
		else
		begin
			if(spi_stage==0) //начинаем передачу
			begin
				busy=1;
				CS_S=0;
				SCK_C=0;
				spi_bit_position=7;
				spi_stage=1;
			end
			else if(spi_stage==1) //отсылаем 1 бит
			begin
				busy=1;
				SCK_C=0;
				MOSI_DQ0=data_in[spi_bit_position];
				spi_stage=2;
			end
			else if(spi_stage==2) //завершена отсылка 1го бита, принимаем ответ
			begin
				busy=1;
				SCK_C=1;
				data_out[spi_bit_position]=MISO_DQ1;
				if(spi_bit_position==0)
				begin
					spi_stage=99;
					busy=0;
				end
				else
				begin
					spi_bit_position=spi_bit_position-8'd1;
					spi_stage=1;
				end
			end
		end
	end
	else
	begin	
		SCK_C=1;
		CS_S=1;
		MOSI_DQ0=0;
		spi_bit_position=7;
		spi_stage=0;
		busy=0;
	end
end

endmodule
