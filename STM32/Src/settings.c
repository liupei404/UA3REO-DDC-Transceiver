#include "settings.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include "functions.h"
#include "trx_manager.h"
#include "lcd.h"
#include "fpga.h"
#include "main.h"
#include "bands.h"
#include "wire.h"

//I2C Settings
const I2C_DEVICE I2C_WM8731 = { WM8731_SCK_GPIO_Port, WM8731_SCK_Pin, WM8731_SDA_GPIO_Port, WM8731_SDA_Pin };
const I2C_DEVICE I2C_FPGA = { BUS_COMM_SCK_GPIO_Port, BUS_COMM_SCK_Pin, BUS_COMM_SDA_GPIO_Port, BUS_COMM_SDA_Pin };

//W25Q16
static uint8_t Write_Enable = W25Q16_COMMAND_Write_Enable;
//static uint8_t Erase_Chip = W25Q16_COMMAND_Erase_Chip;
static uint8_t Sector_Erase = W25Q16_COMMAND_Sector_Erase;
static uint8_t Page_Program = W25Q16_COMMAND_Page_Program;
static uint8_t Read_Data = W25Q16_COMMAND_Read_Data;
static uint8_t Address[3] = { 0x00 };
struct TRX_SETTINGS TRX = { 0 };

volatile bool NeedSaveSettings = false;

static void Flash_Sector_Erase(void);
static void Flash_Write_Data(void);
static void Flash_Read_Data(void);

void LoadSettings(void)
{
	Flash_Read_Data();
	
	if (TRX.clean_flash != 180) //code to trace new clean flash
	{
		TRX.clean_flash = 180; //ID прошивки в eeprom, если не совпадает - используем дефолтные
		TRX.VFO_A.Freq = 7100000; //сохранённая частота VFO-A
		TRX.VFO_A.Mode = TRX_MODE_LSB; //сохранённая мода VFO-A
		TRX.VFO_A.Filter_Width = 2700; //сохранённая ширина полосы VFO-A
		TRX.VFO_B.Freq = 14150000; //сохранённая частота VFO-B
		TRX.VFO_B.Mode = TRX_MODE_USB; //сохранённая мода VFO-B
		TRX.VFO_B.Filter_Width = 2700; //сохранённая ширина полосы VFO-B
		TRX.current_vfo = false; // текущая VFO (false - A)
		TRX.Preamp = false; //предусилитель
		TRX.AGC = true; //AGC
		TRX.ATT = false; //аттенюатор
		TRX.LPF = true; //ФНЧ
		TRX.BPF = true; //ДПФ
		TRX.TX_Amplifier = true; //Усилитель RF
		TRX.DNR = false; //цифровое шумоподавление
		TRX.Agc_speed = 3; //скорость AGC
		TRX.LCD_menu_freq_index = MENU_FREQ_KHZ; //выбранный разряд частоты для изменения
		TRX.BandMapEnabled = true; //автоматическая смена моды по карте диапазонов
		TRX.Volume = 20; //громкость
		TRX.InputType = 0; //тип входа для передачи: 0 - mic ; 1 - line ; 2 - usb
		TRX.Mute = false; //приглушить звук
		TRX.Fast = false; //ускоренная смена частоты при вращении энкодера
		TRX.CW_Filter = 500; //дефолтное значение ширины фильтра CW
		TRX.SSB_Filter = 2700; //дефолтное значение ширины фильтра SSB
		TRX.FM_Filter = 15000; //дефолтное значение ширины фильтра FM
		TRX.RF_Power = 20; //выходная мощность (%)
		TRX.FM_SQL_threshold = 1; //FM-шумодав
		TRX.RF_Gain = 50; //усиление ВЧ
		for (uint8_t i = 0; i < BANDS_COUNT; i++) TRX.saved_freq[i] = BANDS[i].startFreq + (BANDS[i].endFreq - BANDS[i].startFreq) / 2; //сохранённые частоты по диапазонам
		TRX.FFT_Zoom = 1; //приближение спектра FFT
		TRX.AutoGain = false; //авто-управление предусилителем и аттенюатором
		TRX.NotchFilter = false; //нотч-фильтр для вырезания помехи
		TRX.NotchFC = 1000; //частота среза нотч-фильтра
		TRX.CWDecoder = false; //автоматический декодер телеграфа
		//system settings
		TRX.FFT_Enabled = true; //использовать спектр FFT
		TRX.CW_GENERATOR_SHIFT_HZ = 500; //смещение гетеродина в CW моде
		TRX.Calibrated = false; //был ли калиброван экран
		TRX.Touchpad_x0 = 3935.0f; //данные для калибровки экрана
		TRX.Touchpad_y0 = 427.0; //данные для калибровки экрана
		TRX.Touchpad_x1 = 2118.0f; //данные для калибровки экрана
		TRX.Touchpad_y1 = 1181.0; //данные для калибровки экрана
		TRX.Touchpad_x2 = 345.0; //данные для калибровки экрана
		TRX.Touchpad_y2 = 991.0; //данные для калибровки экрана
		TRX.Touchpad_x3 = 3908.0; //данные для калибровки экрана
		TRX.Touchpad_y3 = 1948.0; //данные для калибровки экрана
		TRX.Touchpad_x4 = 369.0; //данные для калибровки экрана
		TRX.Touchpad_y4 = 2000.0; //данные для калибровки экрана
		TRX.Touchpad_x5 = 3900.0; //данные для калибровки экрана
		TRX.Touchpad_y5 = 3286.0; //данные для калибровки экрана
		TRX.Touchpad_x6 = 2146.0; //данные для калибровки экрана
		TRX.Touchpad_y6 = 3109.0; //данные для калибровки экрана
		TRX.Touchpad_x7 = 338.0; //данные для калибровки экрана
		TRX.Touchpad_y7 = 2893.0; //данные для калибровки экрана
		TRX.Touchpad_x8 = 2125.0; //данные для калибровки экрана
		TRX.Touchpad_y8 = 1983.0; //данные для калибровки экрана
		TRX.ENCODER_SLOW_RATE = 35; //замедление энкодера для больших разрешений
		TRX.LCD_Brightness = 60; //яркость экрана
		TRX.Standby_Time = 180; //время до гашения экрана по бездействию
		TRX.Beeping = true; //звуки нажатия на кнопки
		TRX.Key_timeout = 1000; //время отпуская передачи после последнего знака на ключе
		TRX.FFT_Averaging = 4; //усреднение FFT, чтобы сделать его более гладким
		TRX.SSB_HPF_pass = 300; //частота среза ФВЧ в SSB моде
		TRX.WIFI_Enabled = false; //активировать WiFi
		strcpy(TRX.WIFI_AP, "WIFI-AP"); //точка доступа WiFi
		strcpy(TRX.WIFI_PASSWORD, "WIFI-PASSWORD"); //пароль к точке WiFi
		TRX.WIFI_TIMEZONE = 3; //часовой пояс (для синхронизации времени)
		TRX.SPEC_Begin = 700; //старт диапазона анализатора спектра
		TRX.SPEC_End = 800; //старт диапазона анализатора спектра
	}
}

VFO *CurrentVFO(void)
{
	if (!TRX.current_vfo)
		return &TRX.VFO_A;
	else
		return &TRX.VFO_B;
}

void SaveSettings(void)
{
	NeedSaveSettings = false;
	FPGA_NeedSendParams = true;
	Flash_Sector_Erase();
	HAL_Delay(50);
	Flash_Write_Data();
}

static void Flash_Sector_Erase(void)
{
	return;
	for(uint8_t page=0; page<=(sizeof(TRX)/0xFF); page++)
	{
		Address[1] = page;
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
		HAL_SPI_Transmit(&hspi1, &Write_Enable, 1, HAL_MAX_DELAY); // Write Enable Command
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
		
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
		HAL_SPI_Transmit(&hspi1, &Sector_Erase, 1, HAL_MAX_DELAY);   // Erase Chip Command
		HAL_SPI_Transmit(&hspi1, Address, sizeof(Address), HAL_MAX_DELAY);      // Write Address ( The first address of flash module is 0x00000000 )
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
		HAL_Delay(10);
	}
}

static void Flash_Write_Data(void)
{
	return;
	for(uint8_t page=0; page<=(sizeof(TRX)/0xFF); page++)
	{
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
		HAL_SPI_Transmit(&hspi1, &Write_Enable, 1, HAL_MAX_DELAY); // Write Enable Command
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
	
		Address[1] = page;
		uint16_t size = sizeof(TRX) - 0xFF*page;
		if(size>0xFF) size=0xFF;
		
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
		HAL_SPI_Transmit(&hspi1, &Page_Program, 1, HAL_MAX_DELAY); // Page Program Command
		HAL_SPI_Transmit(&hspi1, Address, sizeof(Address), HAL_MAX_DELAY);      // Write Address ( The first address of flash module is 0x00000000 )
		HAL_SPI_Transmit(&hspi1, (uint8_t*)((uint32_t)&TRX + 0xFF*page), size, HAL_MAX_DELAY);       // Write
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
		HAL_Delay(10);
	}
}

static void Flash_Read_Data(void)
{
	return;
	for(uint8_t page=0; page<=(sizeof(TRX)/0xFF); page++)
	{
		Address[1] = page;
		uint16_t size = sizeof(TRX) - 0xFF*page;
		if(size>0xFF) size=0xFF;
		
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_RESET);     // CS to low
		HAL_SPI_Transmit(&hspi1, &Read_Data, 1, HAL_MAX_DELAY);  // Read Command
		HAL_SPI_Transmit(&hspi1, Address, sizeof(Address), HAL_MAX_DELAY);    // Write Address
		HAL_SPI_Receive(&hspi1, (uint8_t*)((uint32_t)&TRX + 0xFF*page), size, HAL_MAX_DELAY);      // Read
		HAL_GPIO_WritePin(W26Q16_CS_GPIO_Port, W26Q16_CS_Pin, GPIO_PIN_SET);       // CS to high
		HAL_Delay(10);
	}
}
