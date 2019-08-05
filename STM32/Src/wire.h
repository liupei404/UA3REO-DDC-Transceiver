#ifndef WIRE_h
#define WIRE_h

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define WIRE_BUFSIZ 101

/* return codes from endTransmission() */
#define SUCCESS   0        /* transmission was successful */
#define EDATA     1        /* too much data */
#define ENACKADDR 2        /* received nack on transmit of address */
#define ENACKTRNS 3        /* received nack on transmit of data */
#define EOTHER    4        /* other error */

#define I2C_WRITE 0
#define I2C_READ  1
#define I2C_DELAY for(int wait_i=0;wait_i<100;wait_i++) { __asm("nop"); };

typedef struct {
	GPIO_TypeDef* SCK_PORT;
	uint16_t SCK_PIN;
	GPIO_TypeDef* SDA_PORT;
	uint16_t SDA_PIN;
} I2C_DEVICE;

extern void i2c_start(I2C_DEVICE dev);
extern void i2c_stop(I2C_DEVICE dev);
extern void i2c_begin(I2C_DEVICE dev);
extern void i2c_beginTransmission_u8(I2C_DEVICE dev, uint8_t);
extern void i2c_write_u8(I2C_DEVICE dev, uint8_t);
extern uint8_t i2c_endTransmission(I2C_DEVICE dev);
extern uint8_t i2c_requestFrom_ii(I2C_DEVICE dev, int address, int numBytes);
extern uint8_t i2c_read(I2C_DEVICE dev);

#endif
