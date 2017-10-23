#ifndef WT_5700_H__
#define WT_5700_H__

#include <stdint.h>
#include "custom_board.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"
#include "nrf_drv_twi.h"

/****************WT5700芯片的iic地址************/
#define WT5700_DEVICE_ADDR			0xE0
#define WT5700_IIC_REAL_ADDR		(WT5700_DEVICE_ADDR>>1)

//实际板子的引脚
#define WT5700_INT_PIN				TOUCH_IIC_INT_PIN
#define WT5700_IIC_SCL_PIN			TOUCH_IIC_SCL_PIN
#define	WT5700_IIC_SDA_PIN			TOUCH_IIC_SDA_PIN
#define WT5700_IIC_EN_PIN			TOUCH_IIC_EN_PIN

/****************WT5700寄存器地址****************/
#define	WT5700_SYS_CTL				0x00
#define	WT5700_SYS_CTL02			0x02
#define	WT5700_SYS_CTL03			0x03

#define	WT5700_SENSOR_DATA0			0x08
#define	WT5700_SENSOR_DATA1			0x09

/********************************************/
ret_code_t wt5700_i2c_write_byte(uint8_t address, uint8_t data);
ret_code_t wt5700_i2c_read_byte(uint8_t address, uint8_t *p_read_byte, uint8_t length);

void wt5700_init(void);
uint8_t wt5700_key_read(void);

#endif	//WT_5700_H__
