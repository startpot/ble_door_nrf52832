#include <stdint.h>
#include <stdbool.h>

#include "pstorage.h"
#include "ble_gap.h"
#include "app_error.h"
#include "inter_flash.h"

#include "set_params.h"
#include "moto.h"

pstorage_handle_t block_id_params;
uint8_t flash_store_params[8];

//对比动态密码的变量
uint8_t SM4_challenge[4] = {0x31,0x30,0x33,0x36};
uint8_t key_store_tmp[6];

//种子的数组
uint8_t seed[16];

uint8_t KEY_LENGTH;
uint8_t	KEY_CHECK_NUMBER;
uint8_t LED_LIGHT_TIME;

uint32_t 	OPEN_TIME;
uint8_t		DOOR_OPEN_HOLD_TIME;
uint8_t 	BEEP_DIDI_NUMBER;
uint8_t		VOL_VALUE;	//实际值<<4位
uint8_t		KEY_INPUT_USE_TIME;
uint8_t		MOTO_DIR;


//与设置mac有关的变量
ble_gap_addr_t addr;

/*******************************************
* 初始化参数
* in：		none
******************************************/
void set_default_params(void)
{
	uint32_t err_code;

	//设置动态密码的长度为6位ASCII
	KEY_LENGTH = 6;
	KEY_CHECK_NUMBER = 5;
	LED_LIGHT_TIME = 5;
	
	//读出设置的参数(判读是否是后期设置的,如果不是，设定参数)
	//初始化参数
	//([0x77(w，如果为w则已经设置参数，如果不是则初始化参数),
	//			25(OPEN_TIME *0.1s),10(DOOR_OPEN_HOLD_TIME *0.1s),
	//			5(BEEP_DIDI_NUMBER 次数)，5(LED_LIGHT_TIME *0.1s),
	//			5(KEY_CHECK_NUMBER) 次数]后面补0)
	err_code = pstorage_block_identifier_get(&block_id_flash_store, \
								(pstorage_size_t)DEFAULT_PARAMS_OFFSET, &block_id_params);
	APP_ERROR_CHECK(err_code);
	pstorage_load(flash_store_params, &block_id_params, 8, 0);
	if(flash_store_params[0] == 0x77)
	{
		OPEN_TIME = flash_store_params[1];//电机转动时间
		DOOR_OPEN_HOLD_TIME = flash_store_params[2];//开门保持时间
		BEEP_DIDI_NUMBER = flash_store_params[3];//蜂鸣器响次数
		VOL_VALUE = flash_store_params[4];//电池电压报警
		KEY_INPUT_USE_TIME = flash_store_params[5];//键盘密码输入密码有效时间，以10min为单位
		MOTO_DIR = flash_store_params[6];//电机的方向
	}
	else
	{
		OPEN_TIME = 0x03;//电机转动时间
		DOOR_OPEN_HOLD_TIME = 0x0a;//开门保持时间
		BEEP_DIDI_NUMBER = 0x05;//蜂鸣器响次数
		VOL_VALUE = 0x2C;//电池电压报警,左移4位，大致是5.0V(设定的欠压值)
		KEY_INPUT_USE_TIME = 0x05;//键盘密码输入密码有效时间，以10min为单位
		MOTO_DIR = 0;
	}
#if defined(BLE_DOOR_DEBUG)
	printf("params set:\r\n");
	printf("moto time:%d\r\n", OPEN_TIME);
	printf("door open time:%d\r\n", DOOR_OPEN_HOLD_TIME);
	printf("beep  didi number:%d\r\n", BEEP_DIDI_NUMBER);
	printf("led ligth time:%d\r\n", LED_LIGHT_TIME);
	printf("key check number:%d\r\n", KEY_CHECK_NUMBER);
#endif
	
}
