#ifndef INTER_FLASH_H__
#define INTER_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pstorage.h"

//存储的钥匙
struct key_store_struct
{
	uint8_t 	key_store[6];
	uint16_t 	key_use_time;//有效时间，以10分钟为单位
	uint8_t		control_bits;
	uint8_t		key_vesion;
	time_t		key_store_time;
};

//存储钥匙长度
struct key_store_length_struct
{
	uint32_t key_store_length;
	uint32_t key_store_full;
};

#define SUPER_KEY_LENGTH		12
//超级管理员秘钥
extern char						super_key[SUPER_KEY_LENGTH];

//开门记录
struct door_open_record
{
	uint8_t		key_store[6];
	time_t		door_open_time;//门打开的时间s,(从1970.1.1到现在的时间的s)
};

//开门记录的长度
struct record_length_struct
{
	uint32_t record_length;
	uint32_t record_full;
};

/********************************************************
*flash存储空间整个一个block_id
*[0]存储参数+[1]钥匙个数+钥匙记录(10)+[12]长度+开门记录[30]
* flash中各个存储量的偏移地址和长度
*********************************************************/
#define BLOCK_STORE_SIZE			32

/*********************************************************
*默认的参数:(1byte)
*			电机转动时间(OPEN_TIME)
*			开门后等待时间(DOOR_OPEN_HOLD_TIME)
*			蜂鸣器响动次数(BEEP_DIDI_NUMBER)
*			亮灯时间(LED_LIGHT_TIME)
*			密码校对次数(KEY_CHECK_NUMBER)
**********************************************************/
#define DEFAULT_PARAMS_OFFSET		0
#define DEFAULT_PARAMS_NUMBER		1

#define MAC_OFFSET					DEFAULT_PARAMS_OFFSET + DEFAULT_PARAMS_NUMBER
#define MAC_NUMBER					1

#define SPUER_KEY_OFFSET			MAC_OFFSET + MAC_NUMBER
#define SUPER_KEY_NUMBER			1

#define SEED_OFFSET					SPUER_KEY_OFFSET + SUPER_KEY_NUMBER
#define SEED_NUMBER					1

#define DEVICE_NAME_OFFSET			SEED_OFFSET + SEED_NUMBER
#define DEVICE_NAME_NUMBER			1

#define	KEY_STORE_OFFSET			DEVICE_NAME_OFFSET + DEVICE_NAME_NUMBER
#define KEY_STORE_LENGTH			1	//第一个4字节表示条数，第二个字节表示是否满
#define	KEY_STORE_NUMBER			10

#define	RECORD_OFFSET				KEY_STORE_OFFSET + KEY_STORE_LENGTH + KEY_STORE_NUMBER
#define RECORD_LENGTH				1	//第一个4字节表示条数，第二个字节表示是否满
#define	RECORD_NUMBER				30


#define BLOCK_STORE_COUNT			DEFAULT_PARAMS_NUMBER +MAC_NUMBER + SUPER_KEY_NUMBER + \
									SEED_NUMBER + DEVICE_NAME_NUMBER + \
									KEY_STORE_LENGTH + KEY_STORE_NUMBER + \
									RECORD_LENGTH + RECORD_NUMBER

#define SEED_LENGTH					16


extern pstorage_handle_t			block_id_flash_store;

extern pstorage_handle_t			block_id_default_params;
extern pstorage_handle_t			block_id_mac;
extern pstorage_handle_t			block_id_super_key;
extern pstorage_handle_t			block_id_seed;
extern pstorage_handle_t			block_id_device_name;
extern pstorage_handle_t			block_id_key_store;
extern pstorage_handle_t			block_id_record;

extern struct key_store_length_struct		key_store_length;
extern struct record_length_struct			record_length;

extern bool				key_store_length_setted;
extern bool				record_length_setted;

extern pstorage_handle_t			block_id_write;
extern pstorage_handle_t			block_id_read;
//从flash中读出的数据
extern uint8_t					flash_write_data[BLOCK_STORE_SIZE];
extern uint8_t					flash_read_data[BLOCK_STORE_SIZE];
extern uint8_t					flash_read_key_store_data[BLOCK_STORE_SIZE];
extern uint8_t					flash_write_key_store_data[BLOCK_STORE_SIZE];
extern uint8_t					flash_read_record_data[BLOCK_STORE_SIZE];
extern uint8_t					flash_write_record_data[BLOCK_STORE_SIZE];
extern uint8_t					flash_read_temp[BLOCK_STORE_SIZE];

void flash_init(void);
void inter_flash_write(uint8_t *p_data, uint32_t data_len, \
					   pstorage_size_t block_id_offset, pstorage_handle_t *block_id_write_source);
void inter_flash_read(uint8_t *p_data, uint32_t data_len, \
					 pstorage_size_t block_id_offset, pstorage_handle_t *block_id_read_source);

void write_super_key(uint8_t *p_data, uint32_t data_len);
void key_store_write(struct key_store_struct *key_store_input);
void record_write(struct door_open_record *open_record);

#endif //INTER_FLASH_H__
