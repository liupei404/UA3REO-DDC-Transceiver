#include "functions.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "fpga.h"

void readHalfFromCircleBuffer32(float32_t *source, float32_t *dest, uint16_t index, uint16_t length)
{
	uint16_t halflen = length / 2;
	if (index >= halflen)
	{
		memcpy(&dest[0], &source[index - halflen], halflen * 4);
	}
	else
	{
		uint16_t prev_part = halflen - index;
		memcpy(&dest[0], &source[length - prev_part], prev_part * 4);
		memcpy(&dest[prev_part], &source[0], (halflen - prev_part) * 4);
	}
}

void readHalfFromCircleBufferU16(uint16_t *source, uint16_t *dest, uint16_t index, uint16_t length)
{
	uint16_t halflen = length / 2;
	if (index >= halflen)
	{
		memcpy((uint16_t *)&dest[0], (uint16_t *)&source[index - halflen], halflen * 2);
	}
	else
	{
		uint16_t prev_part = halflen - index;
		memcpy((uint16_t *)&dest[0], (uint16_t *)&source[length - prev_part], prev_part * 2);
		memcpy((uint16_t *)&dest[prev_part], (uint16_t *)&source[0], (halflen - prev_part) * 2);
	}
}

void readHalfFromCircleBufferU32(uint32_t *source, uint32_t *dest, uint16_t index, uint16_t length)
{
	uint32_t halflen = length / 2;
	if (index >= halflen)
	{
		memcpy((uint32_t *)&dest[0], (uint32_t *)&source[index - halflen], halflen * 2);
	}
	else
	{
		uint32_t prev_part = halflen - index;
		memcpy((uint32_t *)&dest[0], (uint32_t *)&source[length - prev_part], prev_part * 2);
		memcpy((uint32_t *)&dest[prev_part], (uint32_t *)&source[0], (halflen - prev_part) * 2);
	}
}

void logToUART1_str(char* data)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)data, strlen(data), HAL_MAX_DELAY);
}

void logToUART1_num(uint8_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%d\r\n", data);
	logToUART1_str(tmp);
}

void logToUART1_numinline(uint8_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%d", data);
	logToUART1_str(tmp);
}

void logToUART1_num16(uint16_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%d\r\n", data);
	logToUART1_str(tmp);
}
void logToUART1_int16(int16_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%d\r\n", data);
	logToUART1_str(tmp);
}
void logToUART1_int32(int32_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%d\r\n", data);
	logToUART1_str(tmp);
}
void logToUART1_num32(uint32_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%d\r\n", data);
	logToUART1_str(tmp);
}

void logToUART1_float32(float32_t data)
{
	char tmp[50] = "";
	sprintf(tmp, "%f\r\n", data);
	logToUART1_str(tmp);
}

void delay_us(uint32_t us)
{
	int32_t us_count_tick = us * (SystemCoreClock / 1000000);
	//разрешаем использовать счётчик
	SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	//обнуляем значение счётного регистра
	DWT_CYCCNT = 0;
	//запускаем счётчик
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;
	while (DWT_CYCCNT < us_count_tick);
	//останавливаем счётчик
	DWT_CONTROL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t getFrequencyFromPhrase(uint32_t phrase) //высчитываем фазу частоты для FPGA
{
	uint32_t res = 0;
	res = ceil(((double)phrase / 4194304) * ADCDAC_CLOCK / 100) * 100; //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
	return res;
}

uint32_t getPhraseFromFrequency(uint32_t freq) //высчитываем частоту из фразы ля FPGA
{
	uint32_t res = 0;
	res = round(((double)freq / ADCDAC_CLOCK) * 4194304); //freq in hz/oscil in hz*2^bits = (freq/48000000)*4194304;
	return res;
}

uint32_t hexStringToInt(char* in) //преобразование строки шестнадцатеричного числа в число
{
	char* res;
	strcpy(res, "0");
	if (strlen(in) % 2)
	{
		strcat(res, in);
		in = res;
	}
	return strtol(in, NULL, 16);
}

void addSymbols(char* dest, char* str, uint8_t length, char* symbol, bool toEnd) //добавляем нули
{
	char res[50] = "";
	strcpy(res, str);
	while (strlen(res) < length)
	{
		if (toEnd)
			strcat(res, symbol);
		else
		{
			char tmp[50] = "";
			strcat(tmp, symbol);
			strcat(tmp, res);
			strcpy(res, tmp);
		}
	}
	strcpy(dest, res);
}

float log10f_fast(float X) {
	float Y, F;
	int E;
	F = frexpf(fabsf(X), &E);
	Y = 1.23149591368684f;
	Y *= F;
	Y += -4.11852516267426f;
	Y *= F;
	Y += 6.02197014179219f;
	Y *= F;
	Y += -3.13396450166353f;
	Y += E;
	return(Y * 0.3010299956639812f);
}