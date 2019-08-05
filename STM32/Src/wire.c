#include "stm32f4xx_hal.h"
#include "main.h"
#include "functions.h"
#include "wire.h"
#include "settings.h"

/* low level conventions:
 * - SDA/SCL idle high (expected high)
 * - always start with i2c_delay rather than end
 */
 
static uint8_t i2c_rx_buf[WIRE_BUFSIZ] = { 0 };      /* receive buffer */
static uint8_t i2c_rx_buf_idx = 0;               /* first unread idx in rx_buf */
static uint8_t i2c_rx_buf_len = 0;               /* number of bytes read */
static uint8_t i2c_tx_addr = 0;                  /* address transmitting to */
static uint8_t i2c_tx_buf[WIRE_BUFSIZ] = { 0 };      /* transmit buffer */
static uint8_t i2c_tx_buf_idx = 0;  /* next idx available in tx_buf, -1 overflow */
static bool i2c_tx_buf_overflow = false;

static bool i2c_get_ack(I2C_DEVICE dev);
static uint8_t i2c_writeOneByte(I2C_DEVICE dev, uint8_t);
static void    i2c_shift_out(I2C_DEVICE dev, uint8_t val);
static uint8_t i2c_readOneByte(I2C_DEVICE dev, uint8_t address, uint8_t *byte);
static uint8_t i2c_requestFrom_u8i(I2C_DEVICE dev, uint8_t address, int num_bytes);

void i2c_start(I2C_DEVICE dev) {
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SDA_PORT, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_RESET);
}

void i2c_stop(I2C_DEVICE dev) {
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SDA_PORT, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_SET);
	
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	I2C_DELAY;
	I2C_DELAY;
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_SET);
	I2C_DELAY;
}

static bool i2c_get_ack(I2C_DEVICE dev) {
	GPIO_InitTypeDef GPIO_InitStruct;
	int time = 0;
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_RESET);
	
	GPIO_InitStruct.Pin = dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SDA_PORT, &GPIO_InitStruct);

	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	I2C_DELAY;

	while (HAL_GPIO_ReadPin(dev.SDA_PORT, dev.SDA_PIN))
	{
		time++;
		if (time > 50)
		{
			i2c_stop(I2C_WM8731);
			return false;
		}
		I2C_DELAY;
	}

	GPIO_InitStruct.Pin = dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SDA_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_RESET);

	//I2C_DELAY;
	//HAL_Delay(1);
	return true;
}

static void i2c_send_ack(I2C_DEVICE dev) {
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_RESET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_RESET);
}

static void i2c_send_nack(I2C_DEVICE dev) {
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_SET);
	I2C_DELAY;
	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
}

static uint8_t i2c_shift_in(I2C_DEVICE dev) {
	uint8_t data = 0;
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SDA_PORT, &GPIO_InitStruct);
	
	int i;
	for (i = 0; i < 8; i++) {
		I2C_DELAY;
		HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
		I2C_DELAY;
		
		data += HAL_GPIO_ReadPin(dev.SDA_PORT, dev.SDA_PIN) << (7 - i);
		
		I2C_DELAY;
		HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_RESET);
	}
	
	return data;
}

static void i2c_shift_out(I2C_DEVICE dev, uint8_t val) {
	int i;
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.Pin = dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SDA_PORT, &GPIO_InitStruct);
	
	for (i = 0; i < 8; i++) {
		I2C_DELAY;
		HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, bitRead(val,7-i));
		
		I2C_DELAY;
		HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);

		I2C_DELAY;
		I2C_DELAY;
		HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_RESET);
	}
}

/*
 * Joins I2C bus as master on given SDA and SCL pins.
 */
void i2c_begin(I2C_DEVICE dev) {
	GPIO_InitTypeDef GPIO_InitStruct;

	HAL_GPIO_WritePin(dev.SCK_PORT, dev.SCK_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(dev.SDA_PORT, dev.SDA_PIN, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = dev.SCK_PIN | dev.SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
	HAL_GPIO_Init(dev.SCK_PORT, &GPIO_InitStruct);
}

void i2c_beginTransmission_u8(I2C_DEVICE dev, uint8_t slave_address) {
	i2c_tx_addr = slave_address;
	i2c_tx_buf_idx = 0;
	i2c_tx_buf_overflow = false;
	i2c_rx_buf_idx = 0;
	i2c_rx_buf_len = 0;
}

uint8_t i2c_endTransmission(I2C_DEVICE dev) {
	if (i2c_tx_buf_overflow) return EDATA;
	i2c_start(dev);

	//I2C_DELAY;
	i2c_shift_out(dev, (i2c_tx_addr << 1) | I2C_WRITE);
	if (!i2c_get_ack(dev)) 
	{
		i2c_stop(dev);
		return ENACKADDR;
	}

	// shift out the address we're transmitting to
	for (uint8_t i = 0; i < i2c_tx_buf_idx; i++) {
		uint8_t ret = i2c_writeOneByte(dev, i2c_tx_buf[i]);
		if (ret)
		{
			i2c_stop(dev);
			return ret;    // SUCCESS is 0
		}
	}
	I2C_DELAY;
	I2C_DELAY;
	i2c_stop(dev);

	i2c_tx_buf_idx = 0;
	i2c_tx_buf_overflow = false;
	return SUCCESS;
}

static uint8_t i2c_requestFrom_u8i(I2C_DEVICE dev, uint8_t address, int num_bytes)
{
	if (num_bytes > WIRE_BUFSIZ) num_bytes = WIRE_BUFSIZ;

	i2c_rx_buf_idx = 0;
	i2c_rx_buf_len = 0;
	
	i2c_start(dev);
	i2c_shift_out(dev,(address << 1) | I2C_READ);
	if (!i2c_get_ack(dev))
	{
		i2c_stop(dev);
		return ENACKADDR;
	}
	
	while (i2c_rx_buf_len < num_bytes) {
		i2c_rx_buf[i2c_rx_buf_len] = i2c_shift_in(dev);
		i2c_rx_buf_len++;
		if(i2c_rx_buf_len != num_bytes)
			i2c_send_ack(dev);
	}
	
	i2c_send_nack(dev);
	i2c_stop(dev);
	
	return i2c_rx_buf_len;
}

uint8_t i2c_requestFrom_ii(I2C_DEVICE dev, int address, int numBytes)
{
	return i2c_requestFrom_u8i(dev, (uint8_t)address, (uint8_t)numBytes);
}

uint8_t i2c_read(I2C_DEVICE dev)
{
	if (i2c_rx_buf_idx == i2c_rx_buf_len) return 0;
	return i2c_rx_buf[i2c_rx_buf_idx++];
}

static uint8_t i2c_readOneByte(I2C_DEVICE dev, uint8_t address, uint8_t *byte)
{
	i2c_start(dev);

	i2c_shift_out(dev,(address << 1) | I2C_READ);
	if (!i2c_get_ack(dev))
	{
		i2c_stop(dev);
		return ENACKADDR;
	}

	*byte = i2c_shift_in(dev);

	i2c_send_nack(dev);
	i2c_stop(dev);

	return SUCCESS;
}

void i2c_write_u8(I2C_DEVICE dev, uint8_t value) {
	if (i2c_tx_buf_idx == WIRE_BUFSIZ) {
		i2c_tx_buf_overflow = true;
		return;
	}

	i2c_tx_buf[i2c_tx_buf_idx++] = value;
}

// private methods
static uint8_t i2c_writeOneByte(I2C_DEVICE dev, uint8_t byte) {
	i2c_shift_out(dev, byte);
	if (!i2c_get_ack(dev)) return ENACKTRNS;
	return SUCCESS;
}

