#include "stm32f4xx_hal.h"
#include "main.h"
#include "fpga.h"
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "audio_processor.h"
#include "settings.h"
#include "wm8731.h"
#include "usbd_debug_if.h"

volatile uint32_t FPGA_samples = 0;
volatile bool FPGA_iq_busy = false;
volatile bool FPGA_comm_busy = false;
volatile bool FPGA_NeedSendParams = false;
volatile bool FPGA_NeedGetParams = false;
volatile bool FPGA_Buffer_underrun = false;

static GPIO_InitTypeDef FPGA_GPIO_InitStruct;
static bool FPGA_bus_direction = false; //false - OUT; true - in
uint16_t FPGA_Audio_Buffer_Index = 0;
bool FPGA_Audio_Buffer_State = true; //true - compleate ; false - half
float32_t FPGA_Audio_Buffer_SPEC_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_SPEC_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_VOICE_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_Buffer_VOICE_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_SendBuffer_Q[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
float32_t FPGA_Audio_SendBuffer_I[FPGA_AUDIO_BUFFER_SIZE] = { 0 };
uint8_t SPI_Bus_Buffer_RX[10]={0};
uint8_t SPI_Bus_Buffer_RX_copy[10]={0};
uint8_t SPI_Bus_Buffer_TX[10]={0};

uint8_t FPGA_readPacket(void);
void FPGA_writePacket(uint8_t packet);
void FPGA_clockFall(void);
void FPGA_clockRise(void);
void FPGA_fpgadata_sendiq(void);
void FPGA_fpgadata_getparam(void);
void FPGA_fpgadata_sendparam(void);
void FPGA_setBusInput(void);
void FPGA_setBusOutput(void);
void FPGA_read_flash(void);

void FPGA_Init(void)
{
	FPGA_start_audio_clock();
}

void FPGA_start_audio_clock(void) //запуск PLL для I2S и кодека, при включенном тактовом не программируется i2c
{
	if(FPGA_comm_busy) return;
	FPGA_comm_busy=true;
	i2c_begin(I2C_FPGA);
	i2c_beginTransmission_u8(I2C_FPGA, B8(1110010)); //I2C_ADDRESS
	i2c_write_u8(I2C_FPGA, 33);
	i2c_write_u8(I2C_FPGA, 255);
	uint8_t st = i2c_endTransmission(I2C_FPGA);
	FPGA_comm_busy=false;
}

void FPGA_stop_audio_clock(void) //остановка PLL для I2S и кодека, при включенном тактовом не программируется i2c
{
	if(FPGA_comm_busy) return;
	FPGA_comm_busy=true;
	i2c_begin(I2C_FPGA);
	i2c_beginTransmission_u8(I2C_FPGA, B8(1110010)); //I2C_ADDRESS
	i2c_write_u8(I2C_FPGA, 33);
	i2c_write_u8(I2C_FPGA, 0);
	uint8_t st = i2c_endTransmission(I2C_FPGA);
	FPGA_comm_busy=false;
}

void FPGA_fpgadata_stuffclock(void)
{
	if(FPGA_comm_busy) return;
	FPGA_comm_busy=true;
	if (FPGA_NeedSendParams) { FPGA_fpgadata_sendparam(); FPGA_NeedSendParams = false; }
	else if (FPGA_NeedGetParams) { FPGA_fpgadata_getparam(); FPGA_NeedGetParams = false; }
	FPGA_comm_busy=false;
}

void FPGA_fpgadata_do_iq_transfer(void)
{
	FPGA_iq_busy = true;
	if(HAL_SPI_GetState(&hspi1)==HAL_SPI_STATE_BUSY_TX_RX) HAL_SPI_Abort_IT(&hspi1);
	if(HAL_SPI_GetState(&hspi1)==HAL_SPI_STATE_READY)
		HAL_SPI_TransmitReceive_IT(&hspi1,SPI_Bus_Buffer_TX,SPI_Bus_Buffer_RX,8);
	FPGA_iq_busy = false;
}

void FPGA_fpgadata_parse_iq_answer(void)
{
	FPGA_iq_busy = true;
	for(uint8_t i=0;i<8;i++)
		SPI_Bus_Buffer_RX_copy[i]=SPI_Bus_Buffer_RX[i];
	FPGA_iq_busy = false;
	
	register int16_t FPGA_fpgadata_in_tmp16 = 0;
	FPGA_samples++;

	//in SPEC Q
	FPGA_fpgadata_in_tmp16 = (SPI_Bus_Buffer_RX_copy[6] & 0XFF) << 8;
	FPGA_fpgadata_in_tmp16 |= (SPI_Bus_Buffer_RX_copy[7] & 0XFF);
	if (TRX_IQ_swap)
	{
		if (NeedFFTInputBuffer) FFTInput_I[FFT_buff_index] = FPGA_fpgadata_in_tmp16;
		FPGA_Audio_Buffer_SPEC_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	}
	else
	{
		if (NeedFFTInputBuffer) FFTInput_Q[FFT_buff_index] = FPGA_fpgadata_in_tmp16;
		FPGA_Audio_Buffer_SPEC_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	}
	
	//in SPEC I
	FPGA_fpgadata_in_tmp16 = (SPI_Bus_Buffer_RX_copy[4] & 0XFF) << 8;
	FPGA_fpgadata_in_tmp16 |= (SPI_Bus_Buffer_RX_copy[5] & 0XFF);
	if (TRX_IQ_swap)
	{
		if (NeedFFTInputBuffer) FFTInput_Q[FFT_buff_index] = FPGA_fpgadata_in_tmp16;
		FPGA_Audio_Buffer_SPEC_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	}
	else
	{
		if (NeedFFTInputBuffer) FFTInput_I[FFT_buff_index] = FPGA_fpgadata_in_tmp16;
		FPGA_Audio_Buffer_SPEC_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	}

	//in VOICE Q
	FPGA_fpgadata_in_tmp16 = (SPI_Bus_Buffer_RX_copy[2] & 0XFF) << 8;
	FPGA_fpgadata_in_tmp16 |= (SPI_Bus_Buffer_RX_copy[3] & 0XFF);
	if (TRX_IQ_swap)
		FPGA_Audio_Buffer_VOICE_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	else
		FPGA_Audio_Buffer_VOICE_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;

	//in VOICE I
	FPGA_fpgadata_in_tmp16 = (SPI_Bus_Buffer_RX_copy[0] & 0XFF) << 8;
	FPGA_fpgadata_in_tmp16 |= (SPI_Bus_Buffer_RX_copy[1] & 0XFF);
	if (TRX_IQ_swap)
		FPGA_Audio_Buffer_VOICE_Q[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;
	else
		FPGA_Audio_Buffer_VOICE_I[FPGA_Audio_Buffer_Index] = FPGA_fpgadata_in_tmp16;

	//stuff work RX
	
	FPGA_Audio_Buffer_Index++;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE) FPGA_Audio_Buffer_Index = 0;

	if (NeedFFTInputBuffer)
	{
		FFT_buff_index++;
		if (FFT_buff_index == FFT_SIZE)
		{
			FFT_buff_index = 0;
			NeedFFTInputBuffer = false;
		}
	}
	
	/////// TX
	uint16_t FPGA_fpgadata_out_tmp16 = 0;

	//out Q
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_SendBuffer_Q[FPGA_Audio_Buffer_Index];
	SPI_Bus_Buffer_TX[0]=(FPGA_fpgadata_out_tmp16 >> 8) & 0xFF;
	SPI_Bus_Buffer_TX[1]=FPGA_fpgadata_out_tmp16 & 0xFF;

	//out I
	FPGA_fpgadata_out_tmp16 = (float32_t)FPGA_Audio_SendBuffer_I[FPGA_Audio_Buffer_Index];
	SPI_Bus_Buffer_TX[2]=(FPGA_fpgadata_out_tmp16 >> 8) & 0xFF;
	SPI_Bus_Buffer_TX[3]=FPGA_fpgadata_out_tmp16 & 0xFF;
	
	//stuff work TX
	
	FPGA_Audio_Buffer_Index++;
	if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE)
	{
		if (Processor_NeedTXBuffer)
		{
			FPGA_Buffer_underrun = true;
			FPGA_Audio_Buffer_Index--;
		}
		else
		{
			FPGA_Audio_Buffer_Index = 0;
			FPGA_Audio_Buffer_State = true;
			Processor_NeedTXBuffer = true;
		}
	}
	else if (FPGA_Audio_Buffer_Index == FPGA_AUDIO_BUFFER_SIZE / 2)
	{
		if (Processor_NeedTXBuffer)
		{
			FPGA_Buffer_underrun = true;
			FPGA_Audio_Buffer_Index--;
		}
		else
		{
			FPGA_Audio_Buffer_State = false;
			Processor_NeedTXBuffer = true;
		}
	}
	
}

inline void FPGA_fpgadata_sendparam(void)
{
	uint8_t FPGA_fpgadata_out_tmp8 = 0;
	uint32_t TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq);
	if (!TRX_on_TX())
	{
		switch (TRX_getMode())
		{
		case TRX_MODE_CW_L:
			TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq + TRX.CW_GENERATOR_SHIFT_HZ);
			break;
		case TRX_MODE_CW_U:
			TRX_freq_phrase = getPhraseFromFrequency(CurrentVFO()->Freq - TRX.CW_GENERATOR_SHIFT_HZ);
			break;
		default:
			break;
		}
	}
	
	//out PTT+PREAMP
	bitWrite(FPGA_fpgadata_out_tmp8, 0, (TRX_on_TX() && TRX_getMode() != TRX_MODE_LOOPBACK));
	if (!TRX_on_TX()) bitWrite(FPGA_fpgadata_out_tmp8, 1, TRX.Preamp);
	
	i2c_begin(I2C_FPGA);
	i2c_beginTransmission_u8(I2C_FPGA, B8(1110010)); //I2C_ADDRESS
	i2c_write_u8(I2C_FPGA, 31);
	i2c_write_u8(I2C_FPGA, FPGA_fpgadata_out_tmp8);
	uint8_t st = i2c_endTransmission(I2C_FPGA);
	
	//out FREQ
	i2c_begin(I2C_FPGA);
	i2c_beginTransmission_u8(I2C_FPGA, B8(1110010)); //I2C_ADDRESS
	i2c_write_u8(I2C_FPGA, 32);
	i2c_write_u8(I2C_FPGA, ((TRX_freq_phrase & (0XFF << 16)) >> 16));
	i2c_write_u8(I2C_FPGA, ((TRX_freq_phrase & (0XFF << 8)) >> 8));
	i2c_write_u8(I2C_FPGA, (TRX_freq_phrase & 0XFF));
	st = i2c_endTransmission(I2C_FPGA);
}

inline void FPGA_fpgadata_getparam(void)
{
	/*
	uint8_t FPGA_fpgadata_in_tmp8 = 0;
	int16_t adc_min = 0;
	int16_t adc_max = 0;
	
	//STAGE 2
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	TRX_ADC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 0);
	TRX_DAC_OTR = bitRead(FPGA_fpgadata_in_tmp8, 1);
	TRX_FrontPanel.key_4 = bitRead(FPGA_fpgadata_in_tmp8, 2);
	TRX_FrontPanel.key_3 = bitRead(FPGA_fpgadata_in_tmp8, 3);
	TRX_FrontPanel.key_2 = bitRead(FPGA_fpgadata_in_tmp8, 4);
	TRX_FrontPanel.key_1 = bitRead(FPGA_fpgadata_in_tmp8, 5);
	//clock
	FPGA_clockFall();

	//STAGE 3
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	adc_min |= (FPGA_fpgadata_in_tmp8 & 0XF0) << 4;
	adc_max |= (FPGA_fpgadata_in_tmp8 & 0X0F) << 8;
	//clock
	FPGA_clockFall();

	//STAGE 4
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	adc_min |= (FPGA_fpgadata_in_tmp8 & 0XFF);
	TRX_ADC_MINAMPLITUDE = (adc_min << 4);
	TRX_ADC_MINAMPLITUDE = TRX_ADC_MINAMPLITUDE / 16;
	//clock
	FPGA_clockFall();

	//STAGE 5
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	adc_max |= (FPGA_fpgadata_in_tmp8 & 0XFF);
	TRX_ADC_MAXAMPLITUDE = adc_max;
	//clock
	FPGA_clockFall();
	
	//STAGE 6
	//clock
	FPGA_clockRise();
	//in
	FPGA_fpgadata_in_tmp8 = FPGA_readPacket();
	int8_t enc_val = (FPGA_fpgadata_in_tmp8 & 0XF);
	if(enc_val>7) enc_val |= 0xF0;
	TRX_FrontPanel.sec_encoder = enc_val;
	TRX_FrontPanel.key_enc = bitRead(FPGA_fpgadata_in_tmp8, 4);
	//clock
	FPGA_clockFall();
	*/
}

void FPGA_start_command(uint8_t command) //выполнение команды к SPI flash
{
	/*
	FPGA_busy = true;
	
	//STAGE 1
	FPGA_writePacket(7); //FPGA FLASH READ command
	GPIOC->BSRR = FPGA_SYNC_Pin;
	FPGA_clockRise();
	GPIOC->BSRR = ((uint32_t)FPGA_CLK_Pin << 16U) | ((uint32_t)FPGA_SYNC_Pin << 16U);
	HAL_Delay(1);
	
	//STAGE 2
	FPGA_writePacket(command); //SPI FLASH READ STATUS COMMAND
	FPGA_clockRise();
	FPGA_clockFall();
	HAL_Delay(1);
	*/
}

uint8_t FPGA_continue_command(uint8_t writedata) //продолжение чтения и записи к SPI flash
{
	/*
	//STAGE 3 WRITE
	FPGA_writePacket(writedata); 
	FPGA_clockRise();
	FPGA_clockFall();
	HAL_Delay(1);
	//STAGE 4 READ
	FPGA_clockRise();
	FPGA_clockFall();
	uint8_t data = FPGA_readPacket();
	HAL_Delay(1);
	
	return data;
	*/
	return 0;
}

/*
Micron M25P80 Serial Flash COMMANDS:
06h - WRITE ENABLE
04h - WRITE DISABLE
9Fh - READ IDENTIFICATION 
9Eh - READ IDENTIFICATION 
05h - READ STATUS REGISTER 
01h - WRITE STATUS REGISTER 
03h - READ DATA BYTES
0Bh - READ DATA BYTES at HIGHER SPEED
02h - PAGE PROGRAM 
D8h - SECTOR ERASE 
C7h - BULK ERASE 
B9h - DEEP POWER-DOWN 
ABh - RELEASE from DEEP POWER-DOWN 
*/
void FPGA_read_flash(void) //чтение flash памяти
{
	/*
	FPGA_busy = true;
	//FPGA_start_command(0xB9);
	FPGA_start_command(0xAB);
	//FPGA_start_command(0x04);
	FPGA_start_command(0x06);
	FPGA_start_command(0x05);
	//FPGA_start_command(0x03); // READ DATA BYTES
	//FPGA_continue_command(0x00); //addr 1
	//FPGA_continue_command(0x00); //addr 2
	//FPGA_continue_command(0x00); //addr 3
	
	for(uint16_t i=1 ; i<=512 ; i++)
	{
		uint8_t data = FPGA_continue_command(0x05);
		sendToDebug_hex(data,true);
		sendToDebug_str(" ");
		if(i % 16 == 0)
		{
			sendToDebug_str("\r\n");
			HAL_IWDG_Refresh(&hiwdg);
			DEBUG_Transmit_FIFO_Events();
		}
		//HAL_IWDG_Refresh(&hiwdg);
		//DEBUG_Transmit_FIFO_Events();
	}
	sendToDebug_newline();
	
	FPGA_busy = false;
	*/
}
