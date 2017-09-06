#ifndef SET_PARAMS_H__
#define SET_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>

#include "pstorage.h"
#include "ble_gap.h"

#define SET_KEY_CHECK_NUMBER		3

//与动态口令相关的参量
#define SM4_INTERVAL		60
#define SM4_COUNTER			1234
extern uint8_t SM4_challenge[4];
extern uint8_t key_store_tmp[6];
//种子的数组
extern uint8_t seed[16];

extern pstorage_handle_t block_id_params;
extern uint8_t flash_store_params[8];

extern uint8_t 	KEY_LENGTH;
extern uint8_t	KEY_CHECK_NUMBER;
extern uint8_t 	LED_LIGHT_TIME;

extern uint32_t	OPEN_TIME;
extern uint8_t	DOOR_OPEN_HOLD_TIME;
extern uint8_t 	BEEP_DIDI_NUMBER;
extern uint8_t	VOL_VALUE;
extern uint8_t	KEY_INPUT_USE_TIME;
extern uint8_t	MOTO_DIR;

extern ble_gap_addr_t addr;

void set_default_params(void);

#endif	//SET_PARAMS_H__
