/**********************
*四脚蜂鸣器
*
**********************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "custom_board.h"
#include "boards.h"

#include "beep.h"

/*************************************
*	初始化蜂鸣器
**************************************/
void beep_init(void)
{
	nrf_gpio_cfg_output(BEEP_IN_PIN);
	//设置高
	nrf_gpio_pin_clear(BEEP_IN_PIN);
#if defined(BLE_DOOR_DEBUG)
	printf("beep status set low\r\n");
#endif
}

/*********************
* 蜂鸣器响一次
*********************/
static void	beep_didi_once(void)
{
	nrf_gpio_pin_set(BEEP_IN_PIN);
	nrf_delay_ms(BEEP_OPEN);
	nrf_gpio_pin_clear(BEEP_IN_PIN);
}

/********************************************
*蜂鸣器响几次
*in:	number	蜂鸣器响的次数
********************************************/
void beep_didi(uint8_t number)
{
	//蜂鸣器响
	for(int i = 0; i < number; i++)
	{
		beep_didi_once();
		nrf_delay_ms(BEEP_DELAY);
	}
	//恢复蜂鸣器状态
	nrf_gpio_pin_clear(BEEP_IN_PIN);	
}
