/** *****************************************************
*-----------------------------------------------------------------
* bluetooth-door
* ------------------------------- --------------------------------
*	13 LEDs ; iic(sda,scl,en,int)
*-----------------------------------------------------------------
*	2-wire moto ; 1-wire beer;uart(fig)
*-----------------------------------------------------------------
*	ble-softdevice-s132_nrf52_2.0.0-7.alpha
*-----------------------------------------------------------------
*	nrf52832-P.x	define in
*	custom_board.h
********************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "nordic_common.h"
#include "bsp.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_config.h"

#include "ble_hci.h"
#include "ble_dis.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "pstorage.h"
#include "device_manager.h"
#include "app_trace.h"
#include "app_util_platform.h"
#include "uicr_config.h"

#include "led_button.h"
#include "touch_tsm12.h"
#include "moto.h"
#include "beep.h"
#include "rtc_chip.h"
#include "inter_flash.h"
#include "operate_code.h"
#include "set_params.h"
#include "ble_init.h"
#include "sm4_mcu.h"
#include "sm4_dpwd.h"
#include "my_time.h"

/***************************
* 动态口令算法测试程序
***************************/
void sm4_test(void)
{
	struct sm4_context sm4_ctx;
	uint8_t output[16];
	uint8_t key[16] = {	0x01, 0x23, 0x45, 0x67, \
						0x89, 0xab, 0xcd, 0xef, \
						0xfe, 0xdc, 0xba, 0x98, \
						0x76, 0x54, 0x32, 0x10};
	uint8_t data[16] = { 0x01, 0x23, 0x45, 0x67, \
						 0x89, 0xab, 0xcd, 0xef, \
						 0xfe, 0xdc, 0xba, 0x98, \
						 0x76, 0x54, 0x32, 0x10};
	
	sm4_ctx.mode = SM4_ENCRYPT;
	sm4_ctx.sk[0] = 0x00;
							 
	sm4_setkey_enc(&sm4_ctx, key);
					 
	// output = 			 0x68, 0x1e, 0xdf, 0x34, 0xd2, 0x06, 0x96, 0x5e
	//						 0x86, 0xb3, 0xe9, 0x4f, 0x53, 0x6e, 0x42, 0x46
	sm4_crypt_ecb( &sm4_ctx, SM4_ENCRYPT, 16, data, output);

	uint8_t DynPwd[6];
	
	// DPasswd
	key[0] = 0x12; key[1] = 0x34; key[2] = 0x56; key[3] = 0x78;
	key[4] = 0x90; key[5] = 0xab; key[6] = 0xcd; key[7] = 0xef;
	key[8] = 0x12; key[9] = 0x34; key[10] = 0x56; key[11] = 0x78;
	key[12] = 0x90; key[13] = 0xab; key[14] = 0xcd; key[15] = 0xef;
	uint16_t Interval = 0x01;
	uint64_t time = 0x4feab9cd;
	uint32_t counter = 0x000004d2;
	uint8_t Challenge[4] = {0x35, 0x36, 0x37, 0x38};
	
	// SM4 encoder output is
	// 88 0d 6a e7 7e cf 8e e5 23 5c 71 98 e1 3f 15 9c
	// TruncateSM4 output is
	// 0b 78 81 00
	// DPasswd is : 446720
	SM4_DPasswd(key, time, Interval, counter, Challenge, DynPwd);
}


static void set_mac_from_interflash(void)
{
	uint32_t err_code;
	//如果配置了mac，则使用配置的mac
	if(mac[0] =='w')
	{
		memset(addr.addr, 0, 6);
		//拷贝设置的mac
		memcpy(addr.addr, &mac[2], 6);
		err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
		if(err_code == NRF_SUCCESS)
		{
#if defined(BLE_DOOR_DEBUG)
			printf(" use mac setted by user:");
			for(int i=0; i<6;i++)
			{
				printf("%x ",mac[3+i]);
			}
			printf("\r\n");
#endif
		}		
	}
}

static void set_peer_password(void)
{
	uint32_t err_code;
	//添加配对密码
	char *passcode = "123456";
	ble_opt_t static_option;
	
	static_option.gap_opt.passkey.p_passkey = (uint8_t *)passcode;
	err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &static_option);
	APP_ERROR_CHECK(err_code);
	
#if defined(BLE_DOOR_DEBUG)
	printf("ble pair pin set:%s \n",passcode);
	printf("\r\n");
#endif
}

/*****************************
*Application main function.
*****************************/
int main(void)
{
    uint32_t err_code;
    bool erase_bonds;
    
	
	NRF_UICR->NFCPINS = 0;
	
    //Initialize
    timers_init();
	uart_init();
    buttons_leds_init(&erase_bonds);
    ble_stack_init();
	device_manager_init(erase_bonds);
	//初始化内部flash，和各个存储变量，紧跟在device_manager_init后面，读取mac和蓝牙名称
	flash_init();
	
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
    
	//初始化所有参数
	set_default_params();
	//初始化灯，拉高，灭
	leds_init();
//	led test
//	nrf_gpio_pin_clear(LED_12);
//	nrf_gpio_pin_clear(LED_13);
	leds_on(LED_13,5);
//	leds_on(LED_11,5);
	
	//初始化电机
	moto_init();
	//moto test
	//HB设置为HL
/*	nrf_gpio_pin_set(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
	//延迟ms
	nrf_delay_ms(5);
	nrf_gpio_pin_set(MOTO_BI);
			
	//延迟5ms
	nrf_delay_ms(5);
	//设置为stand-by状态
	nrf_gpio_pin_clear(MOTO_FI);
	nrf_gpio_pin_clear(MOTO_BI);
	*/
	//初始化蜂鸣器
	beep_init();
	//初始化触摸屏
	tsm12_init();
	//初始化RTC
	rtc_init();
	//初始化触摸屏和指纹的中断函数
	touch_finger_int_init();
	
	//从内部flash中读取mac，设置
	set_mac_from_interflash();
	//设置配对密码
	set_peer_password();

	application_timers_start();
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
    
    // Enter main loop.
    for (;;)
    {
        power_manage();
		
		//判断命令
		if(operate_code_setted ==true)
		{
			operate_code_check(nus_data_recieve, nus_data_recieve_length);
			operate_code_setted = false;
		}
    }
}
