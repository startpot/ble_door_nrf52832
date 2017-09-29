#ifndef LED_BUTTON_H__
#define LED_BUTTON_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "app_timer.h"

#include "inter_flash.h"

enum keys_type {
	super_keys= 0x00,
	normal_keys,
	unkown_keys
};

extern bool			is_key_value_get;//是否是正在获取键盘按键

extern char			key_express_value;

#define KEY_INPUT_CHECKED_MAX_NUMBER	5

#define KEY_MAX_NUMBER	12
extern char			key_input[KEY_MAX_NUMBER];
extern uint8_t		key_input_site;

//输入的密码的时间
extern struct tm	key_input_time_tm;
extern time_t		key_input_time_t;

//检测密码的次数
extern uint8_t		key_input_checked_number;
extern time_t		key_input_checked_locked_time_t;
extern bool			key_input_checked_locked;

extern struct key_store_struct		key_store_check;


//存储在flash的密码
extern uint8_t		flash_key_store[BLOCK_STORE_SIZE];

extern struct door_open_record		open_record_now;


#define BUTTON_DETECTION_DELAY		APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)

void leds_init(void);
void leds_on(uint8_t led_pin, uint32_t ms);

int ble_door_open(void);

bool keys_input_check_super_keys(char *keys_input_p, uint8_t keys_input_length);
bool keys_input_check_normal_keys(char *keys_input_p, uint8_t keys_input_length, time_t keys_input_time_t);
bool keys_input_check_sm4_keys(char *keys_input_p, uint8_t keys_input_length, time_t keys_input_time_t);

void door_open_record_flash(char *keys_input_p, uint8_t keys_input_length,time_t keys_input_time_t);
void touch_finger_int_init(void);

#endif  //LED_BUTTON_H__
