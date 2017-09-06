#ifndef LED_BUTTON_H__
#define LED_BUTTON_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "app_timer.h"

#include "inter_flash.h"

extern char			key_express_value;

#define KEY_NUMBER	12
extern char			key_input[KEY_NUMBER];
extern uint8_t		key_input_site;

//输入的密码的时间
extern struct tm	key_input_time_tm;
extern time_t		key_input_time_t;

extern struct key_store_struct		key_store_check;


//存储在flash的密码
extern uint8_t		flash_key_store[BLOCK_STORE_SIZE];

extern struct door_open_record		open_record_now;


#define BUTTON_DETECTION_DELAY		APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)

void leds_init(void);
void leds_on(uint8_t led_pin, uint32_t ms);

int ble_door_open(void);
void touch_finger_int_init(void);

#endif  //LED_BUTTON_H__
