#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "bsp.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_twi.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"
#include "app_util_platform.h"
#include "nrf_drv_config.h"

#include "wt5700.h"
#include "led_button.h"


nrf_drv_twi_t	m_twi_master_wt5700	= NRF_DRV_TWI_INSTANCE(1); //指定TWI1

/***********************************************
*初始化触摸屏
*in：	none
************************************************/
static ret_code_t touch_iic_init(void) {
	ret_code_t ret;
	const nrf_drv_twi_config_t config = {
		.scl                = WT5700_IIC_SCL_PIN,
		.sda                = WT5700_IIC_SDA_PIN,
		.frequency          = NRF_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH
	};

	do {
		ret = nrf_drv_twi_init(&m_twi_master_wt5700, &config, NULL, NULL);
		if(NRF_SUCCESS != ret) {
			break;
		}
		nrf_drv_twi_enable(&m_twi_master_wt5700);
	} while(0);

	return ret;
}

/**************************************
*往触摸屏的写某个地址的数据
*in：address		地址
		data			数据
*out：	ret			0成功
****************************************/
ret_code_t wt5700_i2c_write_byte(uint8_t address, uint8_t data) {
	ret_code_t ret;
	uint8_t buffer[3] = {0x00,address,data};
	ret = nrf_drv_twi_tx(&m_twi_master_wt5700, WT5700_IIC_REAL_ADDR, buffer, 3, false);

	return ret;
}

/*************************************************************
*从触摸屏的某个地址开始读数据
*in：	address		地址
			*p_read_byte		读出数据的地址指针
			length				读出数据的长度
*out：	ret			0成功
**************************************************************/
ret_code_t wt5700_i2c_read_byte(uint8_t address, uint8_t *p_read_byte, uint8_t length) {
	ret_code_t ret;

	do {
		//写地址
		uint8_t set_address[2] = {0x00,address};

		ret = nrf_drv_twi_tx(&m_twi_master_wt5700, WT5700_IIC_REAL_ADDR, set_address, 2, true);
		if(ret !=NRF_SUCCESS) {
			break;
		}
		//读数据
		ret = nrf_drv_twi_rx(&m_twi_master_wt5700, WT5700_IIC_REAL_ADDR, p_read_byte, length);
	} while(0);

	return ret;
}

/******************************************
*初始化触摸屏芯片
*in		none
*******************************************/
void wt5700_init(void) {
	uint8_t set_data;

	touch_iic_init();

	//设置为slow down model
	set_data = 0x72;//62
	wt5700_i2c_write_byte(WT5700_SYS_CTL, set_data);

	//将芯片设置成单触摸模式,1/2win sense
	set_data = 0x02; //c2
	wt5700_i2c_write_byte(WT5700_SYS_CTL02, set_data);

	//
	set_data = 0x02; //02
	wt5700_i2c_write_byte(WT5700_SYS_CTL03, set_data);

#if defined(BLE_DOOR_DEBUG)
	printf("touch button init success\r\n");
#endif

}

/***************************************************
*读取触摸屏键值
*in：	none
*out：	key_value		触摸屏的键值
***************************************************/
uint8_t wt5700_key_read(void) {

	uint8_t wt5700_key_value = 0;
	uint8_t wt5700_temp[2];
	uint8_t *p;

	p = wt5700_temp;

	if(is_key_value_get == true) {
		//读取通道8-11
		wt5700_i2c_read_byte(WT5700_SENSOR_DATA0, p, 0x01);
		//读取通道0-7
		wt5700_i2c_read_byte(WT5700_SENSOR_DATA1, (p+1), 0x01);

		if(*p !=0) {
			switch(*p) {
			case 0x01://SI8
				wt5700_key_value = '3';
				break;
			case 0x02://SI9
				wt5700_key_value = '6';
				break;
			case 0x04://SI10
				wt5700_key_value = '9';
				break;
			case 0x08://SI11
				wt5700_key_value = 'b';
				break;
			default:
				wt5700_key_value = 0;
				break;
			}
		} else if(*(p+1) !=0) {
			switch(*(p + 1)) {
			case 0x01://SI0
				wt5700_key_value = 'a';
				break;
			case 0x02://SI1
				wt5700_key_value = '7';
				break;
			case 0x04://SI2
				wt5700_key_value = '4';
				break;
			case 0x08://SI3
				wt5700_key_value = '1';
				break;
			case 0x10://SI4
				wt5700_key_value = '2';
				break;
			case 0x20://SI5
				wt5700_key_value = '5';
				break;
			case 0x40://SI6
				wt5700_key_value = '0';
				break;
			case 0x80://SI7
				wt5700_key_value = '8';
				break;
			default:
				wt5700_key_value = 0;
				break;
			}
		}
	}

	return wt5700_key_value;

}



