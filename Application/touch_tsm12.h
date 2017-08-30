#ifndef  TOUCH_TSM12_H__
#define  TOUCH_TSM12_H__

#include "stdint.h"
#include "custom_board.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"
#include "nrf_drv_twi.h"


/*************TSM12M的地址和引脚*********/

#define TSM12_DEVICE_ADDR			0xD0
#define TSM12_IIC_REAL_ADDR 		(TSM12_DEVICE_ADDR>>1)

#define TSM12_INT_PIN				TOUCH_IIC_INT_PIN
#define TSM12_IIC_SCL_PIN			TOUCH_IIC_SCL_PIN
#define	TSM12_IIC_SDA_PIN			TOUCH_IIC_SDA_PIN
#define TSM12_IIC_EN_PIN			TOUCH_IIC_EN_PIN


/**************TSM12_TOUCH 寄存器地址****************/

#define TSM12_Sensitivity1		0x02
#define	TSM12_Sensitivity2		0x03
#define	TSM12_Sensitivity3		0x04
#define TSM12_Sensitivity4		0x05
#define	TSM12_Sensitivity5		0x06
#define	TSM12_Sensitivity6		0x07
#define	TSM12_Sensitivityx_rst_val		10111011b

#define	TSM12_CTRL1						0x08
#define	TSM12_CTRL2						0x09
#define	TSM12_CTRL1_rst_val				00100010b
//#define	TSM12_CTRL2_rst_val						000001xx

#define	TSM12_Ref_rst1				0x0A
#define	TSM12_Ref_rst2				0x0B
#define TSM12_Ref_rst1_rst_val		11111110b
#define	TSM12_Ref_rst2_rst_val		00001111b

#define	TSM12_Ch_hold1				0x0C
#define	TSM12_Ch_hold2				0x0D
#define	TSM12_Ch_hold1_rst_val		11111110b
#define	TSM12_Ch_hold2_rst_val		00001111b

#define	TSM12_Cal_hold1				0x0E
#define	TSM12_Cal_hold2				0x0F
#define	TSM12_Cal_hold1_rst_val		00000000b
#define	TSM12_Cal_hold2_rst_val		00000000b

#define	TSM12_Output1				0x10
#define	TSM12_Output2				0x11
#define	TSM12_Output3				0x12
#define	TSM12_Output1_rst_val		00000000b
#define	TSM12_Output2_rst_val		00000000b
#define	TSM12_Output3_rst_val		00000000b

/*********************************************/

ret_code_t touch_iic_init(void);

void tsm12_en_start(void);
void tsm12_en_stop(void);

ret_code_t touch_i2c_device_write_byte(uint8_t address, uint8_t data);
ret_code_t touch_i2c_device_read_byte(uint8_t address, uint8_t *p_read_byte, uint8_t length);

void tsm12_init(void);
uint8_t tsm12_key_read(void);


#endif  //TOUCH_TSM12M_H
