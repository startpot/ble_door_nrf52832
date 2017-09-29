/**************************************************
*	触摸按键分布
*	T8---1			T9---2			T10--3
*	T7---4			T12--5			T11--6
*	T6---7			T1---8			T2---9
*	T5---*			T4---0			T3---#
*(#就是开锁键)
**************************************************/

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

#include "touch_tsm12.h"

uint8_t key_value;

nrf_drv_twi_t	m_twi_master_touch	= NRF_DRV_TWI_INSTANCE(1); //指定TWI1

/***********************************************
*初始化触摸屏
*in：	none
************************************************/
ret_code_t touch_iic_init(void) {
	ret_code_t ret;
	const nrf_drv_twi_config_t config = {
		.scl                = TSM12_IIC_SCL_PIN,
		.sda                = TSM12_IIC_SDA_PIN,
		.frequency          = NRF_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH
	};

	do {
		ret = nrf_drv_twi_init(&m_twi_master_touch, &config, NULL, NULL);
		if(NRF_SUCCESS != ret) {
			break;
		}
		nrf_drv_twi_enable(&m_twi_master_touch);
	} while(0);

	return ret;
}

/**********************
*触摸屏使能
*********************/
void tsm12_en_start(void) {
	//设置使能端为输出
	nrf_gpio_cfg_output(TSM12_IIC_EN_PIN);
	nrf_gpio_pin_clear(TSM12_IIC_EN_PIN);
}

/*************************
*触摸屏停止
*************************/
void tsm12_en_stop(void) {
	//设置使能端为输出
	nrf_gpio_cfg_output(TSM12_IIC_EN_PIN);
	nrf_gpio_pin_set(TSM12_IIC_EN_PIN);
}

/**************************************
*往触摸屏的写某个地址的数据
*in：address		地址
		data			数据
*out：	ret			0成功
****************************************/
ret_code_t touch_i2c_device_write_byte(uint8_t address, uint8_t data) {
	ret_code_t ret;
	uint8_t buffer[2] = {address,data};
	ret = nrf_drv_twi_tx(&m_twi_master_touch, TSM12_IIC_REAL_ADDR, buffer, 2, false);

	return ret;
}

/*************************************************************
*从触摸屏的某个地址开始读数据
*in：	address		地址
			*p_read_byte		读出数据的地址指针
			length				读出数据的长度
*out：	ret			0成功
**************************************************************/
ret_code_t touch_i2c_device_read_byte(uint8_t address, uint8_t *p_read_byte, uint8_t length) {
	ret_code_t ret;

	do {
		//写地址
		uint8_t set_address;
		set_address = address;

		ret = nrf_drv_twi_tx(&m_twi_master_touch, TSM12_IIC_REAL_ADDR, &set_address, 1, true);
		if(ret !=NRF_SUCCESS) {
			break;
		}
		//读数据
		ret = nrf_drv_twi_rx(&m_twi_master_touch, TSM12_IIC_REAL_ADDR, p_read_byte, length);
	} while(0);

	return ret;
}

/******************************************
*初始化触摸屏芯片
*in		none
*******************************************/
void tsm12_init(void) {
	uint8_t set_data;
	//使能IIC管脚
	tsm12_en_start();

	touch_iic_init();

	//软件复位，睡眠模式开
	set_data = 0x0F;
	touch_i2c_device_write_byte(TSM12_CTRL2, set_data);
	//使能软件复位，睡眠模式关
	set_data = 0x07;
	touch_i2c_device_write_byte(TSM12_CTRL2, set_data);
	//设置通道1-2的灵敏度
	set_data = 0xBB;
	touch_i2c_device_write_byte(TSM12_Sensitivity1, set_data);
	//设置通道3-4的灵敏度
	touch_i2c_device_write_byte(TSM12_Sensitivity2, set_data);
	//设置通道5-6的灵敏度
	touch_i2c_device_write_byte(TSM12_Sensitivity3, set_data);
	//设置通道7-8的灵敏度
	touch_i2c_device_write_byte(TSM12_Sensitivity4, set_data);
	//设置通道9-10的灵敏度
	touch_i2c_device_write_byte(TSM12_Sensitivity5, set_data);
	//设置通道11-12的灵敏度
	touch_i2c_device_write_byte(TSM12_Sensitivity6, set_data);

	//基本设置
//	set_data = 0x22;
//	i2c_device_write_byte(TSM12_CTRL1, set_data);


	//不复位通道1-8的参考
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ref_rst1, set_data);
	//不复位通道9-12的参考
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ref_rst2, set_data);

	/*
	//使能通道1-8复位
	set_data = 0x00;
	i2c_device_write_byte(TSM12_Cal_hold1, set_data);
	//使能通道9-12复位
	set_data = 0x00;
	i2c_device_write_byte(TSM12_Cal_hold2, set_data);
	*/

	//打开1-8所有通道
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ch_hold1, set_data);
	//打开9-12所有通道
	set_data = 0x00;
	touch_i2c_device_write_byte(TSM12_Ch_hold2, set_data);
#if defined(BLE_DOOR_DEBUG)
	printf("touch button init success\r\n");
#endif
}

/***************************************************
*读取触摸屏键值
*in：	none
*out：	key_value		触摸屏的键值
***************************************************/
uint8_t tsm12_key_read(void) {
	uint8_t temp[3];
	uint8_t *p;

	p = temp;

	//读取通道1-4
	touch_i2c_device_read_byte(TSM12_Output1, p, 0x01);
	//读取通道5-8
	touch_i2c_device_read_byte(TSM12_Output2, (p+1), 0x01);
	//读取通道9-12
	touch_i2c_device_read_byte(TSM12_Output3, (p+2), 0x01);

	if(*p > 2) {
		switch(*p) {
		case 0x03://00000011
			key_value = '8';
			break;
		case 0x0c://00001100
			key_value = '9';
			break;
		case 0x30://00110000
			key_value = 'b';
			break;
		case 0xc0://11000000
			key_value = '0';
			break;
		default:
			key_value = 0;
			break;
		}
	} else if(*(p + 1) > 2) {
		switch(*(p + 1)) {
		case 0x03:
			key_value = 'a';
			break;
		case 0x0c:
			key_value = '7';
			break;
		case 0x30:
			key_value = '4';
			break;
		case 0xc0:
			key_value = '1';
			break;
		default:
			key_value = 0;
			break;
		}
	} else {
		switch(*(p + 2)) {
		case 0x03:
			key_value = '2';
			break;
		case 0x0c:
			key_value = '3';
			break;
		case 0x30:
			key_value = '6';
			break;
		case 0xc0:
			key_value = '5';
			break;
		default:
			key_value = 0;
			break;
		}
	}
	return key_value;

}
