/********************************
*初始化LEDs
*初始化I2C的int_pin和中断处理函数
*********************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "custom_board.h"
#include "boards.h"
#include "app_gpiote.h"
#include "app_uart.h"

#include "led_button.h"
//#include "touch_tsm12.h"
#include "wt5700.h"
#include "moto.h"
#include "beep.h"
#include "set_params.h"
#include "inter_flash.h"
#include "rtc_chip.h"
#include "sm4_mcu.h"
#include "sm4_dpwd.h"
#include "my_time.h"
#include "operate_code.h"
//#include "fm260b.h"
#include "r301t.h"

#define APP_GPIOTE_MAX_USERS		1

app_gpiote_user_id_t				m_app_gpiote_id;

bool			is_key_value_get = false;

char			key_express_value;

//输入按键值，当作输入密码
char		key_input[KEY_MAX_NUMBER];
uint8_t		key_input_site;

char			key_marry[KEY_LENGTH];//匹配的密码

//输入的密码的时间
struct		tm key_input_time_tm;
time_t		key_input_time_t;

//检测密码的次数
uint8_t		key_input_checked_number = 0;
time_t		key_input_checked_locked_time_t;
bool		key_input_checked_locked = false;

//存储在flash的密码
uint8_t			flash_key_store[BLOCK_STORE_SIZE];

///开锁记录全局变量
struct door_open_record		open_record_now;

/***********************************************
 *初始化LED pins
 * 设置LED PINS high(led light when pin is low)
 **********************************************/
void leds_init(void) {
	uint32_t led_list[LEDS_NUMBER] = LEDS_LIST;

	//set output set high //led lit when set low
	for(int pin = 0; pin <LEDS_NUMBER; pin++) {
		nrf_gpio_cfg_output( led_list[pin] );
		nrf_gpio_pin_set( led_list[pin] );
	}

#if defined(BLE_DOOR_DEBUG)
	printf("all leds not lit\r\n");
#endif
	nrf_gpio_cfg_output( BATTERY_LEVEL_EN );
	nrf_gpio_pin_clear( BATTERY_LEVEL_EN );

}

/***********************************************
*LED等亮ms
*in:	led_pin	操作的led引脚
ms			led亮起的时间，单位0.1s
***********************************************/
void leds_on(uint8_t led_pin, uint32_t ms) {
	uint32_t tmp=1<<led_pin;
	if((tmp & LEDS_MASK)  == tmp) {
		nrf_gpio_pin_clear(led_pin);
		nrf_delay_ms(ms*100);
		nrf_gpio_pin_set(led_pin);
	}

}

/************************************************
*写入键值
************************************************/
static void write_key_expressed(void) {
	if(key_input_site < KEY_MAX_NUMBER) {
		key_input[key_input_site] = key_express_value;
		key_input_site ++;
	}

}

/***********************************************
*清除写入的键值
***********************************************/
static void clear_key_expressed(void) {
	for(int i = 0; i < KEY_MAX_NUMBER; i++) {
		key_input[i] = 0x0;
	}
	key_input_site = 0x0;

}

/***********************************************
*门打开函数
***********************************************/
int ble_door_open(void) {
	//亮绿灯
	leds_on(LED_13, LED_LIGHT_TIME);
	if(MOTO_DIR ==0) {
		//蜂鸣器响几次(BEER_DIDI_NUMBER)
		beep_didi(BEEP_DIDI_NUMBER);
		//开锁
		moto_open(OPEN_TIME);
		nrf_delay_ms(DOOR_OPEN_HOLD_TIME * 100);
		//蜂鸣器响
		beep_didi(BEEP_DIDI_NUMBER);
		//恢复moto状态
		moto_close(OPEN_TIME);
		return 0;
	} else {
		//蜂鸣器响几次(BEER_DIDI_NUMBER)
		beep_didi(BEEP_DIDI_NUMBER);
		//开锁
		moto_close(OPEN_TIME);
		nrf_delay_ms(DOOR_OPEN_HOLD_TIME * 100);
		//蜂鸣器响
		beep_didi(BEEP_DIDI_NUMBER);
		//恢复moto状态
		moto_open(OPEN_TIME);
	}

	return 0;

}

/***************************
*对比超级管理员密码
***************************/
bool keys_input_check_super_keys(char *keys_input_p, uint8_t keys_input_length) {
	static char keys_input_check[13];
	static char super_key_store[13];

	//将输入的变成字符串
	memset(keys_input_check, 0, 13);
	memcpy(keys_input_check, keys_input_p, keys_input_length);
	keys_input_check[keys_input_length] = '\0';

	//读取管理员密码
	interflash_read(flash_read_data, 16, SUPER_KEY_OFFSET);
	if(flash_read_data[0] == 'w') {
		//超级密码设置了
		//将读取的密码存储到超级管理员密码数组中
		memset(super_key, 0, 12);
		memcpy(super_key, &flash_read_data[1],12);

		//变超级密码为字符串
		memset(super_key_store, 0, 13);
		memcpy(super_key_store, super_key, 12);
		super_key_store[12] = '\0';

		//对比
		if(strstr(keys_input_p, super_key) != NULL) {
			//密码为真
#if defined(BLE_DOOR_DEBUG)
			printf("it is spuer key\r\n");
#endif
			return true;
		}
	} else {
		//密码为假
		return false;
	}

}

/*************************************
*对比已经存储的密码
*************************************/
bool keys_input_check_normal_keys(char *keys_input_p, uint8_t keys_input_length, time_t keys_input_time_t) {
	static char keys_input_check[13];
	static char normal_keys_store[7];

	//将输入密码变成字符串
	memset(keys_input_check, 0, 13);
	memcpy(keys_input_check, keys_input_p, keys_input_length);
	keys_input_check[keys_input_length + 1] = '\0';

	//循环对比以存储的密码
	for(int i=0; i < KEY_STORE_NUMBER; i++) {
		//获取存储的密码
		interflash_read((uint8_t *)&key_store_get, sizeof(struct key_store_struct), \
		                (KEY_STORE_OFFSET + i));
		if(key_store_get.is_store == 'w') {
			memset(normal_keys_store, 0, 7);
			memcpy(normal_keys_store, key_store_get.key_store, 6);
			normal_keys_store[6] = '\0';
			//对比密码是否一致
			if( ( strstr(keys_input_check, normal_keys_store) != NULL ) &&\
			        (( (double)my_difftime(keys_input_time_t, key_store_get.key_store_time) < (double)key_store_get.key_use_time * 600) ||\
			         key_store_get.key_use_time ==0xffff )) {
				//密码相同，且在有效时间内
				memcpy(key_marry, normal_keys_store, KEY_LENGTH);
				//密码为真
#if defined(BLE_DOOR_DEBUG)
				printf("it is a dynamic key user set\r\n");
#endif
				return true;
			}
		}
	}
	return false;

}

/*************************************
* 对比动态种子产生的密钥
*************************************/
bool keys_input_check_sm4_keys(char *keys_input_p, uint8_t keys_input_length, time_t keys_input_time_t) {
	static char keys_input_check[13];
	static char sm4_keys[7];

	//将输入密码变成字符串
	memset(keys_input_check, 0, 13);
	memcpy(keys_input_check, keys_input_p, keys_input_length);
	keys_input_check[keys_input_length + 1] = '\0';

	//动态密码，获取种子
	interflash_read(flash_read_data, 32, SEED_OFFSET);
	if(flash_read_data[0] == 'w') {
		//设置了种子
		//获取种子16位，128bit
		memset(seed, 0, 16);
		memcpy(seed, &flash_read_data[1], 16);

		//计算KEY_CHECK_NUMBER 次数
		for(int i = 0; i<(KEY_CHECK_NUMBER ); i++) {
			SM4_DPasswd(seed, keys_input_time_t, SM4_INTERVAL, SM4_COUNTER, \
			            SM4_challenge, key_store_tmp);
			memset(sm4_keys, 0, 7);
			memcpy(sm4_keys, key_store_tmp, 6);
			sm4_keys[6] = '\0';

			if(strstr(keys_input_check, sm4_keys) != NULL) {
				//密码相同
				memcpy(key_marry, sm4_keys, KEY_LENGTH);
#if defined(BLE_DOOR_DEBUG)
				printf("key set success\r\n");
#endif
				//密码为真
#if defined(BLE_DOOR_DEBUG)
				printf("it is a dynamic key auto set\r\n");
#endif
				return true;
			} else {
				keys_input_time_t = keys_input_time_t - 60;
			}
		}
	}
	return false;
}

/*****************************************
*将指定位数的密码和系统密码对比，返回结果
******************************************/
static bool touch_keys_input_check(char *keys_input_p, uint8_t keys_input_length,time_t keys_input_time_t) {
	bool is_keys_checked = false;

	//1首先进行普通密码的对比
	is_keys_checked = keys_input_check_normal_keys(keys_input_p, keys_input_length, keys_input_time_t);
	if(is_keys_checked == true) {
		return true;
	}
	return is_keys_checked;

}

/******************************
*记录开门
******************************/
void door_open_record_flash(char *keys_input_p, uint8_t keys_input_length,time_t keys_input_time_t) {
	//记录开门
	if(keys_input_length == 6) {
		memset(&open_record_now, 0, sizeof(struct door_open_record));
		memcpy(&open_record_now.key_store, keys_input_p, keys_input_length);
		memcpy(&open_record_now.door_open_time, &keys_input_time_t, 4);
		record_write(&open_record_now);
	}
}

/**************************************************************
*检验所有按下的键值
**************************************************************/
static void check_keys(void) {
	static bool is_check_locked = false;
	//获取按下开锁键的时间
	rtc_time_read(&key_input_time_tm);
	key_input_time_t = my_mktime(&key_input_time_tm);

	//1、判断验证是否锁定了
	if(key_input_checked_locked ==true ) {
		//1.1、是的话验证下输入的时间和锁定的时间是不是差10分钟
		if( (double)my_difftime(key_input_time_t, key_input_checked_locked_time_t) >= (10*60) ) {
			//大于10分钟，则解除验证锁定
			is_check_locked = false;
		} else { //1.2
			//小于10分钟，锁定验证
			is_check_locked = true;
		}
	} else { //2
		is_check_locked = false;
	}


	//3.如果验证没锁定
	if(is_check_locked == false) {
		//判断输入的按键值
		if(touch_keys_input_check(key_input, key_input_site, key_input_time_t)) {
			ble_door_open();
			key_input_checked_number = 0;
			key_input_checked_locked_time_t = 0;
			key_input_checked_locked = false;
#ifdef BLE_DOOR_DEBUG
			printf("door open\r\n");
#endif

			//记录开门
			door_open_record_flash(key_input, key_input_site, key_input_time_t);
		} else {
			key_input_checked_number++;
			if(key_input_checked_number == KEY_INPUT_CHECKED_MAX_NUMBER) {
				//记录第5次验证失败的时间
				key_input_checked_locked_time_t = key_input_time_t;
				key_input_checked_locked = true;
			}
#ifdef BLE_DOOR_DEBUG
			printf("input keys check fail\r\n");
#endif
		}
	}

	//判断完输入的按键序列后，删除所有按键值
	clear_key_expressed();
#if defined(BLE_DOOR_DEBUG)
	printf("clear all express button\r\n");
#endif

}

/*****************************************
*检验按下的键值，并记录，
*如果是开锁键(b)进行密码校验
*in：	express_value	按下的键值
******************************************/

static void check_key_express(char express_value) {
	static uint8_t board_leds[LEDS_NUMBER-1] = {LED_1, LED_3, LED_5,\
	        LED_7, LED_10, LED_11,\
	        LED_2, LED_4, LED_6,\
	        LED_8, LED_9, LED_12
	                                           };
	static char board_buttons[LEDS_NUMBER-1] = {'1', '2', '3',\
	        '4', '5', '6',\
	        '7', '8', '9',\
	        'a', '0', 'b'
	                                           };

	//判断按键，亮相应的灯
	for(int i=0; i< (LEDS_NUMBER-1); i++) {
		if(board_buttons[i] == express_value) {
			//leds_on(board_leds[i], LED_LIGHT_TIME);
			//设置引脚为输出，置低
			nrf_gpio_pin_set( BATTERY_LEVEL_EN );
			nrf_delay_ms(500);
			nrf_gpio_pin_clear( BATTERY_LEVEL_EN );
		}
	}
	//如果按键是'b'，检验所有按键，其他键则记录下来
	if(express_value == 'b') {
		if(key_input_site >=6) {
			check_keys();
		} else {
			clear_key_expressed();
		}
	} else {
		write_key_expressed();
	}

}

/************************************
*用户解绑操作
************************************/
static void nrst_all(void) {

	//1、清除内部flash所有数据
	for(int j=0; j < BLOCK_STORE_COUNT; j++) {
		pstorage_block_identifier_get(&block_id_flash_store, \
		                              (pstorage_size_t)j, &block_id_write);

		pstorage_clear(&block_id_write, BLOCK_STORE_SIZE);

	}

	//2、清除指纹内所有存储指纹
	//2.1、打开指纹模块电源
	nrf_gpio_pin_set(BATTERY_LEVEL_EN);
	//上电需要0.5s的准备时间
	nrf_delay_ms(1000);
	//2.2、发送删除指令
	fig_r301t_send_cmd(0x01, sizeof(r301t_send_empty_cmd), r301t_send_empty_cmd);
	//2.3、获取指令码,此指令码需要在回复命令处理中使用
	fig_cmd_code = r301t_send_empty_cmd[0];
	//3、获取指令码
	ble_operate_code = USER_UNBIND_CMD;

}

/**************************************************************
*触摸屏中断和指纹中断处理函数
*in：	event_pins_low_to_high	状态由低到高的引脚
*		event_pins_high_to_low	状态由高到低的引脚
**************************************************************/
static void touch_finger_int_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low) {
	//触摸按键中断响应
	if (event_pins_high_to_low & (1 << TOUCH_IIC_INT_PIN)) {
		//触摸中断由高变低
		//	key_express_value = (char)tsm12_key_read();
		if(is_key_value_get !=true) {
			key_express_value = (char)wt5700_key_read();
			check_key_express(key_express_value);
		}
		is_key_value_get = false;
	}
	//指纹中断响应
	if (event_pins_high_to_low & (1 << FIG_WAKE_N_PIN)) {
		/*//指纹中断由高变低,自动化过程，判断如果不是上位机设置的注册模式，发送自动搜索
		if(  is_fm260b_autoenroll == false )
		{
			//未处在自动注册状态，发送自动搜索模板功能
			fig_fm260b_send_autosearch();
		}*/

		if(r301t_autosearch_step == 0 && is_r301t_autoenroll == false) {
			//打开模块芯片电源
			nrf_gpio_pin_set(BATTERY_LEVEL_EN);
			//上电需要0.5s的准备时间
			nrf_delay_ms(1000);
			//设置步骤为1,设置命令码为getimg
			r301t_autosearch_step = 1;
			ble_operate_code = GR_FIG_CMD_GETIMG;
			//指纹模块r301t
			//发送获取图像命令
			fig_r301t_send_cmd(0x01, sizeof(r301t_send_getimg_cmd), \
			                   r301t_send_getimg_cmd);
		}
	}
	//初始化按键中断响应
	if (event_pins_high_to_low & (1 << NRST_IN)) {
		nrst_all();
	}

}

/***************************************************************
* 初始化触摸屏中断引脚和指纹中断引脚
*in：	none
***************************************************************/
void touch_finger_int_init(void) {
	uint32_t err_code;

	//初始化键值，变量
	key_express_value = 0x0;
	//初始化按键记录,及按键值保存位置
	clear_key_expressed();

	uint32_t   low_to_high_bitmask = 0x00000000;
	uint32_t   high_to_low_bitmask = (1 << TOUCH_IIC_INT_PIN) | (1 << FIG_WAKE_N_PIN) | (1 <<NRST_IN);

	// Configure IIC_INT_PIN、FIG_WAKE_N_PIN、NRST_IN   with SENSE enabled
	nrf_gpio_cfg_sense_input(TOUCH_IIC_INT_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
	nrf_gpio_cfg_sense_input(FIG_WAKE_N_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
	nrf_gpio_cfg_sense_input(NRST_IN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

	APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);

	err_code = app_gpiote_user_register(&m_app_gpiote_id,\
	                                    low_to_high_bitmask,\
	                                    high_to_low_bitmask, \
	                                    touch_finger_int_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_gpiote_user_enable(m_app_gpiote_id);
	APP_ERROR_CHECK(err_code);
#if defined(BLE_DOOR_DEBUG)
	printf("touch button int init success\r\n");
#endif

}
