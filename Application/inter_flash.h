#ifndef INTER_FLASH_H__
#define INTER_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "pstorage.h"

//存储钥匙长度
struct key_store_length_struct
{
	uint32_t key_store_length;
	uint32_t key_store_full;
};


//存储的键盘钥匙
struct key_store_struct
{
	uint8_t 	key_store[6];
	uint16_t 	key_use_time;//有效时间，以1分钟为单位
	uint8_t		control_bits;
	uint8_t		key_vesion;
	time_t		key_store_time;
};

#define SUPER_KEY_LENGTH		12
#define KEY_LENGTH				6
//超级管理员秘钥
extern char						super_key[SUPER_KEY_LENGTH];

//开门记录的长度
struct record_length_struct
{
	uint32_t record_length;
	uint32_t record_full;
};

//开门记录
struct door_open_record
{
	uint8_t		key_store[6];
	time_t		door_open_time;//门打开的时间s,(从1970.1.1到现在的时间的s)
};

extern struct key_store_struct 		key_store_struct_set;
extern struct door_open_record		door_open_record_get;


//指纹存储信息
struct fig_info
{
	uint32_t	is_store;	//是否存储
	uint32_t	fig_info_id;//指纹信息的ID
	char		fig_info_data[4];//指纹的描述信息
};

extern struct fig_info	fig_info_set;
extern struct fig_info	fig_info_get;
/*********************************************************************************
*flash存储空间整个一个block_id
*[0]存储参数+[1]钥匙个数+钥匙记录(10)+[12]长度+开门记录[30]
* flash中各个存储量的偏移地址和长度
----------------------------------------------------------------------------------
		内部flash存储结构图
----------------------------------------------------------------------------------
-block_id_flash_store	BLOCK_STORE_SIZE * BLOCK_STORE_COUNT--
----------------------------------------------------------------------------------
block_offset				-|-		size				
----------------------------------------------------------------------------------
DEFAULT_PARAMS_OFFSET (0)	-|-		DEFAULT_PARAMS_NUMBER (1)
MAC_OFFSET (1)				-|-		MAC_NUMBER (1)
SPUER_KEY_OFFSET (2)		-|-		SUPER_KEY_NUMBER (1)
SEED_OFFSET (3)				-|-		SEED_NUMBER (1)
DEVICE_NAME_OFFSET (4)		-|-		DEVICE_NAME_NUMBER (1)
KEY_STORE_OFFSET (5)		-|-		KEY_STORE_LENGTH (1) + KEY_STORE_NUMBER (10)
RECORD_OFFSET (16)			-|-		RECORD_LENGTH (1) +  RECORD_NUMBER(30)
FIG_INFO_OFFSET (47)		-|-		FP_STORE_NUMBER (32)
**********************************************************************************/

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

#define SUPER_KEY_OFFSET			MAC_OFFSET + MAC_NUMBER
#define SUPER_KEY_NUMBER			1

#define SEED_OFFSET					SUPER_KEY_OFFSET + SUPER_KEY_NUMBER
#define SEED_NUMBER					1

#define DEVICE_NAME_OFFSET			SEED_OFFSET + SEED_NUMBER
#define DEVICE_NAME_NUMBER			1

#define	KEY_STORE_OFFSET			DEVICE_NAME_OFFSET + DEVICE_NAME_NUMBER
#define KEY_STORE_LENGTH			1	//第一个4字节表示条数，第二个字节表示是否满
#define	KEY_STORE_NUMBER			10

#define	RECORD_OFFSET				KEY_STORE_OFFSET + KEY_STORE_LENGTH + KEY_STORE_NUMBER
#define RECORD_LENGTH				1	//第一个4字节表示条数，第二个字节表示是否满
#define	RECORD_NUMBER				30

#define FIG_INFO_OFFSET				RECORD_OFFSET + RECORD_LENGTH + RECORD_NUMBER
#define FIG_INFO_NUMBER				32


#define BLOCK_STORE_COUNT			DEFAULT_PARAMS_NUMBER +MAC_NUMBER + SUPER_KEY_NUMBER + \
									SEED_NUMBER + DEVICE_NAME_NUMBER + \
									KEY_STORE_LENGTH + KEY_STORE_NUMBER + \
									RECORD_LENGTH + RECORD_NUMBER + \
									FIG_INFO_NUMBER

#define BLOCK_STORE_SIZE			32

#define SEED_LENGTH					16


extern pstorage_handle_t			block_id_flash_store;

extern pstorage_handle_t			block_id_default_params;
extern pstorage_handle_t			block_id_mac;
extern pstorage_handle_t			block_id_super_key;
extern pstorage_handle_t			block_id_seed;
extern pstorage_handle_t			block_id_device_name;
extern pstorage_handle_t			block_id_key_store;
extern pstorage_handle_t			block_id_record;
extern pstorage_handle_t			block_id_fig_info;

extern struct key_store_length_struct		key_store_length;
extern struct record_length_struct			record_length;

extern bool				key_store_length_setted;
extern bool				record_length_setted;

//内部flash读取和存储
extern uint8_t					interflash_write_data[BLOCK_STORE_SIZE];
extern uint8_t					interflash_read_data[BLOCK_STORE_SIZE];


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

int interflash_write(uint8_t *p_data, uint32_t data_len, \
					 pstorage_size_t block_id_offset);

int interflash_read(uint8_t *p_data, uint32_t data_len, \
					 pstorage_size_t block_id_offset);

int write_super_key(uint8_t *p_data, uint32_t data_len);
void key_store_write(struct key_store_struct *key_store_input);
void record_write(struct door_open_record *open_record);
int fig_info_write(struct fig_info *fp_info_set_p);
int fig_info_read(struct fig_info *fp_info_get_p);

#endif //INTER_FLASH_H__
