#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "bsp.h"
#include "ble_nus.h"
#include "ble_gap.h"
#include "pstorage.h"
#include "app_uart.h"

#include "operate_code.h"
#include "ble_init.h"
#include "rtc_chip.h"
#include "inter_flash.h"
#include "set_params.h"
#include "sm4_dpwd.h"
#include "my_time.h"
#include "led_button.h"
#include "fm260b.h"
#include "r301t.h"
#include "battery.h"


struct tm 	time_record;//读出记录的时间
time_t 		time_record_t;//读出的时间的int
struct tm 	time_record_compare;//要对比的时间
time_t 		time_record_compare_t;//要对比的时间的int

struct tm				fp_set_tm;//指纹设置的时间
time_t					fp_set_time_t;//指纹设置的时间int
struct fp_store_struct	fp_store_struct_set;//指纹设置的结构体

//与获取和设置时间相关的变量
struct tm					time_set;
struct tm					time_get;
time_t						time_get_t;

uint32_t					record_length_get;
uint32_t					key_store_length_get;

bool		is_superkey_checked = false;

/***********************************
*设置开锁密码命令
************************************/
static int cmd_set_key(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	bool is_keys_checked = false;
	//获取收到的时间
	err_code = rtc_time_read(&time_get);
	if(err_code == NRF_SUCCESS)
	{
		time_get_t = my_mktime(&time_get);
	}
			
	//1、先进行现存密码的比对
	is_keys_checked = keys_input_check_normal_keys(p_data, 6, time_get_t);
	if(is_keys_checked == true)
	{
		//开门
		ble_door_open();
		//记录开门
		door_open_record_flash((char *)p_data, 6, time_get_t);
					
		//返回ff00
		nus_data_send[0] = 0xff;
		nus_data_send[1] = 0x00;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		return 0;
	}
		
	//2、获取种子，进行动态密码对比，对比SET_KEY_CHECK_NUMBER次
	is_keys_checked = keys_input_check_sm4_keys(p_data, 6, time_get_t);
	if(is_keys_checked == true)
	{
		//开门
		ble_door_open();
		//记录开门
		door_open_record_flash((char *)p_data, 6, time_get_t);
				
		//返回ff00
		nus_data_send[0] = 0xff;
		nus_data_send[1] = 0x00;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);			
		return 0;
	}
	
	//失败返回ff01
	nus_data_send[0] = 0xff;
	nus_data_send[1] = 0x01;
	nus_data_send_length = 2;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	return 0;
	
}

/**********************************************
*同步时间命令，成功返回(命令码+0x40)+设置的时间
***********************************************/

static void sync_rtc_time(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	if(is_superkey_checked == true)//如果验证了超级密码
	{
		//是对时命令,[year0][year1][mon][day][hour][min][sec]
		time_set.tm_sec = (int)p_data[7];
		time_set.tm_min = (int)p_data[6];
		time_set.tm_hour = (int)p_data[5];
		time_set.tm_mday = (int)p_data[4];
		time_set.tm_mon = (int)p_data[3];
		//年小端
		time_set.tm_year = (int)((((int)p_data[2])<<8 | (int)p_data[1]) - 1990);
		
		//将时间写入RTC
		err_code =  rtc_time_write(&time_set);
		if(err_code ==NRF_SUCCESS)
		{
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x00;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
		else
		{
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x01;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
	}
	
}

/********************************************
*获取系统时间，返回(命令码+0x40)+系统时间
********************************************/
static void get_rtc_time(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	memset(nus_data_send, 0, BLE_NUS_MAX_DATA_LEN);
	err_code = rtc_time_read(&time_get);
	if(err_code == NRF_SUCCESS)
	{
		//年是小端
		nus_data_send[2] = (uint8_t)((time_get.tm_year + 1990)>>8);
		nus_data_send[1] = (uint8_t)(time_get.tm_year + 1990);
		nus_data_send[3] = (uint8_t)time_get.tm_mon;
		nus_data_send[4] = (uint8_t)time_get.tm_mday;
		nus_data_send[5] = (uint8_t)time_get.tm_hour;
		nus_data_send[6] = (uint8_t)time_get.tm_min;
		nus_data_send[7] = (uint8_t)time_get.tm_sec;
		
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send_length = 8;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	
}

/*************************************************
*获取现在的动态密码
*************************************************/
static void get_key_now(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	char no_seed[8] = "no seed";
	memset(nus_data_send, 0, BLE_NUS_MAX_DATA_LEN);
	err_code = rtc_time_read(&time_get);
	if(err_code == NRF_SUCCESS)
	{
		//将时间变换为64位
		time_get_t = my_mktime(&time_get);
		//获取种子
		interflash_read(flash_read_data, 32, SEED_OFFSET);
		if(flash_read_data[0] == 'w')
		{//设置了种子
			//获取种子
			memset(seed, 0, 16);
			memcpy(seed, &flash_read_data[1], 16);
			//计算动态密码
			SM4_DPasswd(seed, time_get_t, SM4_INTERVAL, SM4_COUNTER, SM4_challenge, key_store_tmp);
			//整合返回包
			nus_data_send[0] = p_data[0] + 0x40;
			memcpy(&nus_data_send[1], &key_store_tmp, 6);
			nus_data_send_length = KEY_LENGTH + 1;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
		else
		{//无种子，则发送no seed
			ble_nus_string_send(&m_nus, (uint8_t *)no_seed, strlen(no_seed));
		}	
	}
	
}

/*********************************************
*设置系统参数，成功返回(命令码+0x40)+设置参数
**********************************************/
static void set_param(uint8_t *p_data, uint16_t length)
{
	int ret_code;
	//设置电机转动时间
	OPEN_TIME = p_data[1];
	//设置开门时间
	DOOR_OPEN_HOLD_TIME = p_data[2];
	//设置蜂鸣器响动次数
	BEEP_DIDI_NUMBER = p_data[3];
	//设置电池电压报警
	VOL_VALUE = p_data[4];
	//键盘设置密码的有效时间(单位 10min)
	KEY_CHECK_NUMBER = p_data[5];
	//电机的转动方向
	MOTO_DIR = p_data[6];
		
	memset(flash_write_data, 0, BLOCK_STORE_SIZE);
	//写入标记'w'
	flash_write_data[0] = 'w';
	memcpy(&flash_write_data[1], &p_data[1], length -1);
		
	//将参数写入到flash
	ret_code =  interflash_write(flash_write_data, length, DEFAULT_PARAMS_OFFSET);
		
	//应答包
	//将命令加上0x40,返回给app
	nus_data_send[0] = p_data[0] + 0x40;
	nus_data_send[1] = ret_code;
	nus_data_send_length = 2;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	
}

/************************************************
*设置系统的种子，设置成功返回(命令码+0x40)+种子
*************************************************/
static void set_key_seed(uint8_t *p_data, uint16_t length)
{
	int ret_code;
	//传输的种子应该是小端字节
	memset(flash_write_data, 0, BLOCK_STORE_SIZE);
	//写入标记'w'
	flash_write_data[0] = 'w';
	memcpy(&flash_write_data[1],&p_data[1], SEED_LENGTH);
		
	//将种子写入到flash
	ret_code =  interflash_write(flash_write_data, BLOCK_STORE_SIZE, SEED_OFFSET);
		
	//应答包

		//将命令加上0x40,返回给app
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send[1] = (uint8_t) ret_code;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
}

/***********************************************
*设置mac，成功则返回(命令码+0x40)+mac
*			失败返回设置失败
***********************************************/
static void set_mac(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	if((p_data[6] &0xc0) ==0xc0)
	{//设置的mac最高2位为11，有效
		//存储mac地址
		memset(mac, 0, 8);
		mac[0] = 'w';
		mac[1] = 0x06;
		memcpy(&mac[3], &p_data[1], 6);
		interflash_write(mac, 8, MAC_OFFSET);
		//配置mac
		memset(addr.addr, 0, 6);
		//拷贝设置的mac
		memcpy(addr.addr, &p_data[1], 6);
		err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
		if(err_code == NRF_SUCCESS)
		{
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x00;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
	}
	else
	{
		//将命令加上0x40,返回给app
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send[1] = 0x01;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	
}

/*******************************************************
*获取mac地址，成功返回(命令码+0x40)+mac
*			  失败返回获取失败
*******************************************************/
static void get_mac(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	
	memset(addr.addr, 0, 6);
	err_code = sd_ble_gap_address_get(&addr);
	if(err_code == NRF_SUCCESS)
	{
		//将命令加上0x40,返回给app
		nus_data_send[0] = p_data[0] + 0x40;
		memcpy(&nus_data_send[1], &addr.addr[0], 6);
		nus_data_send_length = 6;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	else
	{
		//将命令加上0x40,返回给app
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send[1] = 0x01;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	
}

/***************************************************
*获取电池电量 0x2c~~5.0V 0x36~~6.0V ，大致1~~~0.1V
****************************************************/
static void get_battery_level(uint8_t *p_data, uint16_t length)
{
	uint8_t tmp = (battery_level_value &0x0ff0) >>4;
	
	//将命令加上0x40,返回给app
	nus_data_send[0] = p_data[0] + 0x40;
	memcpy(&nus_data_send[1], &tmp, 1);
	nus_data_send_length = 2;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	
}


/**********************************************************************
*设置管理员密码，如果是第一次设置，直接通过，返回设置成功
*如果不是第一次设置，则需要通过管理员密码的验证，通过的话，返回设置成功
*没有通过验证的话，返回设置失败
***********************************************************************/
static int set_super_key(uint8_t *p_data, uint16_t length)
{
	//1读取超级管理员存储区内容
	interflash_read(flash_read_data, BLOCK_STORE_SIZE, SPUER_KEY_OFFSET);
	
	if( flash_read_data[0] != 'w' )
	{//没有有管理员密码，直接存储
		memset(flash_write_data, 0, BLOCK_STORE_SIZE);
		flash_write_data[0] = 'w';//'w'
		memcpy(&flash_write_data[1],&p_data[1], SUPER_KEY_LENGTH);
		//超级密码就12位，取写入数据前面16位(16>(1+12))
		write_super_key(flash_write_data,SUPER_KEY_LENGTH +1);

		//将命令加上0x40,返回给app
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send[1] = 0x00;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		
	}
	else
	{
		//是不是跟原来密码一致
		//取存储的超级管理员密码
		memset(super_key, 0, sizeof(super_key));
		memcpy(super_key, &flash_read_data[1],sizeof(super_key));
		//1、对比管理员密码是否相同
		if(strncasecmp((char *)&p_data[1],super_key, SUPER_KEY_LENGTH) == 0)
		{	
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x00;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			return 0;
		}
		
		if(is_superkey_checked == true)
		{//存在管理员密码，但是验证了管理员密码，属于修改密码
			memset(flash_write_data, 0, BLOCK_STORE_SIZE);
			memcpy(&flash_write_data[1],&p_data[1], SUPER_KEY_LENGTH);
			flash_write_data[0] = 'w';//'w'
			//超级密码就12位，取写入数据前面16位(16>(1+12))
			write_super_key(flash_write_data,SUPER_KEY_LENGTH + 1);
			
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x00;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			return 0;
		}
		
		//已经有管理员密码了,且没有验证管理员密码或者与原密码不一致

			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x01;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	return 0;
}

/*******************************************
*验证超级管理员密码,验证通过则返回
*(命令码+0x40)+超级管理员密码
*验证失败，返回：没有设置管理员密码
*			或者验证失败
*******************************************/
static void check_super_key(uint8_t *p_data, uint16_t length)
{
	//1、从flash中读取超级管理员密码
	interflash_read(flash_read_data, 16, SPUER_KEY_OFFSET);		
	if(flash_read_data[0] == 'w')
	{//设置了超级管理员密码
		memset(super_key, 0, 12);
		memcpy(super_key, &flash_read_data[1],12);
		//2、对比管理员密码是否相同
		if(strncasecmp((char *)&p_data[1],super_key, SUPER_KEY_LENGTH) == 0)
		{
#if defined(BLE_DOOR_DEBUG)
			printf("spuer key checked\r\n");
#endif
			is_superkey_checked = true;
			
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x00;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
		else
		{

			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			nus_data_send[1] = 0x01;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
	}
	else
	{//未设置管理员密码
		
		//将命令加上0x40,返回给app
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send[1] = 0x02;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}

}

/*****************************************
*获取现在存储的密码
******************************************/
static void get_used_key(uint8_t *p_data, uint16_t length)
{
	memset(nus_data_send, 0, BLE_NUS_MAX_DATA_LEN);
	//获取密码的数量，小端字节
	interflash_read(flash_read_data, BLOCK_STORE_SIZE, KEY_STORE_OFFSET);
	memcpy(&key_store_length, flash_read_data, sizeof(struct key_store_length_struct));
		
	key_store_length_get = key_store_length.key_store_length;
		
	nus_data_send[0] = p_data[0] + 0x40;
	if(key_store_length_get ==0)
	{
		nus_data_send[1] = 0;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	else
	{
		if( key_store_length.key_store_full ==0x1 )
		{//记录满
			nus_data_send[1] = (uint8_t)KEY_STORE_NUMBER;
		}
		else if(key_store_length.key_store_length >0 &&key_store_length.key_store_full ==0x0)
		{
			nus_data_send[1] = (uint8_t)key_store_length.key_store_length;
		}
		for(int i=0; i<nus_data_send[1]; i++)
		{
			nus_data_send[2] = (uint8_t)i;
			interflash_read(flash_read_data, BLOCK_STORE_SIZE, \
							 (KEY_STORE_OFFSET+1+i));
			memcpy(&nus_data_send[3], flash_read_data, sizeof(struct key_store_struct));
			ble_nus_string_send(&m_nus, nus_data_send, sizeof(struct key_store_struct)+3);
		}
	}
	
}

/***************************************
*获取开锁记录的数量
****************************************/
static void get_record_number(uint8_t *p_data, uint16_t length)
{
	memset(flash_read_data, 0, BLOCK_STORE_SIZE);
	memset(nus_data_send, 0, BLE_NUS_MAX_DATA_LEN);
	//读取记录的数量,小端字节
	interflash_read(flash_read_data, BLOCK_STORE_SIZE, RECORD_OFFSET);
	memcpy(&record_length,flash_read_data, sizeof(struct record_length_struct));
		
	nus_data_send[0] = p_data[0] + 0x40;
	if(record_length.record_full ==0x1)
	{//记录满
		record_length_get = RECORD_NUMBER;
		memcpy(&nus_data_send[1], &record_length_get,4);
	}
	else
	{
		memcpy(&nus_data_send[1], &record_length.record_length,4);
	}
	//发送应答包
	nus_data_send_length = 5;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	
}

/****************************************
*获取最近的开锁记录
*****************************************/
static void get_recent_record(uint8_t *p_data, uint16_t length)
{
	memset(nus_data_send, 0, BLE_NUS_MAX_DATA_LEN);
	time_record_compare.tm_sec = p_data[7];
	time_record_compare.tm_min = p_data[6];
	time_record_compare.tm_hour = p_data[5];
	time_record_compare.tm_mday = p_data[4];
	time_record_compare.tm_mon = p_data[3];
	time_record_compare.tm_year = (int)((((int)p_data[2])<<8 | (int)p_data[1]) -1990);
	//计算秒数
	time_record_compare_t = my_mktime(&time_record_compare);
	//获取记录的数量,小端字节
	//读取记录的数量,小端字节
	interflash_read(flash_read_data, BLOCK_STORE_SIZE, RECORD_OFFSET);
	memcpy(&record_length,flash_read_data, sizeof(struct record_length_struct));
		
	if(record_length.record_full ==0x1)
	{
		nus_data_send[1] = (uint8_t)RECORD_NUMBER;
		for(int i=0; i<RECORD_NUMBER; i++)
		{
			//读出记录
			interflash_read(flash_read_data, BLOCK_STORE_SIZE, \
								(RECORD_OFFSET+1+i));
			memcpy(&door_open_record_get, flash_read_data, sizeof(struct door_open_record));
			//对比时间
			if(my_difftime(door_open_record_get.door_open_time, time_record_compare_t)>0)
			{
				nus_data_send[0] = p_data[0] + 0x40;
				nus_data_send[2] = i;
				memcpy(&nus_data_send[3], &door_open_record_get, sizeof(struct door_open_record));
				nus_data_send_length = sizeof(struct door_open_record)+3;
				ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			}
		}
	}
	else if(record_length.record_length >0)
	{
		nus_data_send[1] = (uint8_t)record_length.record_length;
		for(int i=0; i<record_length.record_length; i++)
		{
			//读出记录
			interflash_read(flash_read_data, BLOCK_STORE_SIZE, \
								(RECORD_OFFSET+1+i));
			memcpy(&door_open_record_get, flash_read_data, sizeof(struct door_open_record));
			//对比时间
			if(my_difftime(door_open_record_get.door_open_time, time_record_compare_t)>0)
			{
				nus_data_send[0] = p_data[0] + 0x40;
				nus_data_send[2] = i;
				memcpy(&nus_data_send[3], &door_open_record_get, sizeof(struct door_open_record));
				nus_data_send_length  = sizeof(struct door_open_record)+3;
				ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			}
		}
	}
	else
	{//无记录
		nus_data_send[0] = p_data[0] + 0x40;
		nus_data_send[2] = 0x00;
		nus_data_send_length  = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
	
}

/*************************************
*用户解除绑定命令
**************************************/
static void user_unbind_cmd(uint8_t *p_data, uint16_t length)
{
	
	static uint8_t fig_r301t_empty_cmd[12]= {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,\
													0x01, 0x00, 0x03, 0x0d, \
													0x00, 0x11};
	//用户解除绑定
	//1、清除内部flash所有数据
						
	for(int j=0;j < BLOCK_STORE_COUNT; j++)
	{
	pstorage_block_identifier_get(&block_id_flash_store, \
						(pstorage_size_t)j, &block_id_write);

	pstorage_clear(&block_id_write, BLOCK_STORE_SIZE);

	}
	
	//2、清除指纹内所有存储指纹
	for (uint32_t i = 0; i < 12; i++)
	{
		while(app_uart_put(fig_r301t_empty_cmd[i]) != NRF_SUCCESS);
	}
	//3、向上位机发送结果
	//将命令加上0x40,返回给app
	nus_data_send[0] = p_data[0] + 0x40;
	nus_data_send[1] = 0;
	nus_data_send_length = 2;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	
}





/*******************************************************
*指纹模块fm260b命令处理,主要调用uart端口
*******************************************************/
static void send_fig_fm260b_cmd(uint8_t *p_data, uint16_t length)
{
	//获取第一参数和第二参数,两字节，大端
	dr_fig_param_first = p_data[3] *256 + p_data[4];
	dr_fig_param_second = p_data[5] *256 + p_data[6];
	//将获取的指令发送给指纹模块
	for (uint32_t i = 0; i < length; i++)
	{
		while(app_uart_put(p_data[i]) != NRF_SUCCESS);
	}
	
	if(p_data[2] == DR_FIG_CMD_AUTOSEARCH)//自动注册模块指令
	{
		//设置自动注册状态位
		is_fm260b_autoenroll = true;
	}
	
}

/***********************************************************
*指纹模块r301t命令处理，调用uart端口
***********************************************************/
static void send_fig_r301t_cmd(uint8_t *p_data, uint16_t length)
{
	static uint8_t fig_r301t_autoenroll_reply[12]= {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,\
													0x07, 0x00, 0x03, 0x00, \
													0x00, 0x0A};
	//判断是不是自动注册命令
	if(p_data[GR_FIG_DATA_ID_SITE] == GR_FIG_DATA_ID_CMD && \
		p_data[GR_FIG_CMD_SITE]==GR_FIG_CMD_AUTOENROLL)
	{
		//设置标志位为true
		is_r301t_autoenroll = true;
		//发送上位机返回包
		memcpy(nus_data_send, fig_r301t_autoenroll_reply,sizeof(fig_r301t_autoenroll_reply));
		nus_data_send_length = sizeof(fig_r301t_autoenroll_reply);
		ble_nus_string_send(&m_nus,nus_data_send, nus_data_send_length);
	}
	else
	{
		//将获取的指令发送给指纹模块
		for (uint32_t i = 0; i < length; i++)
		{
			while(app_uart_put(p_data[i]) != NRF_SUCCESS);
		}
		//判断是不是存储模板命令，自动注册指纹的最后一步
		if(p_data[GR_FIG_DATA_ID_SITE] == GR_FIG_DATA_ID_CMD && \
			p_data[GR_FIG_CMD_SITE]== GR_FIG_CMD_STORECHAR)
		{
			//设置标志位为false
			is_r301t_autoenroll = false;
		}
	}
	
}

/********************************************************
*设置指纹的有效时间
********************************************************/
static void fp_store_set(uint8_t *p_data, uint16_t length)
{
	int err_code;
	//1、获取设置指纹的时间
	rtc_time_read(&fp_set_tm);
	fp_set_time_t = my_mktime(&fp_set_tm);
	//2、组织指纹的结构体
	memset(&fp_store_struct_set, 0, sizeof(struct fp_store_struct));
	fp_store_struct_set.fp_wr_flag = 'w';
	fp_store_struct_set.fp_id = p_data[1]*(0x100) + p_data[2];
	fp_store_struct_set.fp_use_time = p_data[3]*(0x100) + p_data[4];
	fp_store_struct_set.fp_store_time = fp_set_time_t;
	
	//3、将指纹密码结构体存储在flash中
	err_code = fp_write(&fp_store_struct_set);
	//4、将结果返回给上位机
	//将命令加上0x40,返回给app
	nus_data_send[0] = p_data[0] + 0x40;
	nus_data_send[1] = err_code;
	nus_data_send_length = 2;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
}

/************************************************************
*对nus servvice传来的数据进行分析
*in：	*p_data			处理的数据指针
			length				数据长度
***********************************************************/
void operate_code_check(uint8_t *p_data, uint16_t length)
{
	static char checked_superkey_false[16] = "skey check fail";
	switch(p_data[0])
	{
		case '0'://设置开锁秘钥
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		if(length ==0x0a)//10字节
		{
		//	if(is_superkey_checked == true)//如果验证了超级密码
		//	{
				cmd_set_key(p_data, length);
		//	}
		//	else
		//	{
				//向手机发送失败信息"skey check fail"
		//		ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
		//							strlen(checked_superkey_false) );
		//	}
		}
		break;
		
		case SYNC_TIME://同步时间
		if(length ==0x08)//8字节
		{
			if(is_superkey_checked == true)//如果验证了超级密码
			{
			sync_rtc_time(p_data, length);
			}
			else
			{
				//向手机发送失败信息"skey check fail"
				ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			}
		}
		break;
		
		case GET_TIME://获取时间
			get_rtc_time(p_data, length);
		break;
		
		case GET_KEY_NOW://获取现在的动态密码，后期要注释掉
			get_key_now(p_data, length);
		break;
		
		case SET_PARAMS://设置参量
		if(length == 0x7)//6字节
		{
			if(is_superkey_checked == true)//如果验证了超级密码
			{
				set_param(p_data, length);
			}
			else
			{
				//向手机发送失败信息"skey check fail"
				ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			}
		}
		break;
		
		case SET_KEY_SEED://写入种子
		if(length == 0x11)//17字节
		{
			if(is_superkey_checked == true)//如果验证了超级密码
			{
				set_key_seed(p_data, length);
			}
			else
			{
				//向手机发送失败信息"skey check fail"
				ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			}
		}
		break;
		
		case SET_MAC://配置mac,与显示的mac反向
		if(length ==0x07)//7字节
		{
			set_mac(p_data, length);
		}
		break;
		
		case GET_MAC://获取mac地址，
			get_mac(p_data, length);
		break;
		
		case GET_BATTERY_LEVEL://获取电池电量
			get_battery_level(p_data, length);
		break;
		
		case SET_SUPER_KEY://设置管理员密码
		if(length == 0x0d)//13字节
		{
			set_super_key(p_data, length);
		}
		break;
		
		case CHECK_SUPER_KEY://验证超级管理员密码
		if(length == 0x0d)
		{
			check_super_key(p_data,length);
		}
		break;
		
		case GET_USED_KEY://查询有效密码
			if(is_superkey_checked == true)//如果验证了超级密码
			{
				get_used_key(p_data, length);
			}
			else
			{
				//向手机发送失败信息"skey check fail"
				ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			}
		break;
		
		case GET_RECORD_NUMBER://查询开门记录数量
			get_record_number(p_data, length);
		break;
		
		case GET_RECENT_RECORD://查询指定日期后的记录
		if(length == 0x08)//8字节
		{
			if(is_superkey_checked == true)//如果验证了超级密码
			{
				get_recent_record(p_data, length);
			}
			else
			{
				//向手机发送失败信息"skey check fail"
				ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			}
		}
		break;
		
		case USER_UNBIND_CMD: //用户解除绑定
		if(is_superkey_checked == true)//如果验证了超级密码
			{
				user_unbind_cmd(p_data, length);
			}
			else
			{
				//向手机发送失败信息"skey check fail"
				ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			}
			break;
		
		case 0x1B://指纹模块fm260b指令，长度为8，直接通过串口发送给模块
			if(length == 8)//长度为8
			{
				if(is_superkey_checked == true)//如果验证了超级密码
				{
					send_fig_fm260b_cmd(p_data, length);
				}
				else
				{
					//向手机发送失败信息"skey check fail"
					ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
				}
			}
		break;
		
		case 0xEF://指纹模块r301t指令
			if(length >11) //传送命令包最少12位
			{
				if(is_superkey_checked == true)//如果验证了超级密码
				{
					send_fig_r301t_cmd(p_data, length);
				}
				else
				{
					//向手机发送失败信息"skey check fail"
					ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
				}
			}
		break;
			
		case SET_FP_USE_TIME://设置指纹的有效时间
			if(length == 5)
			{
				if(is_superkey_checked == true)//如果验证了超级密码
				{
					fp_store_set(p_data, length);
				}
				else
				{
					//向手机发送失败信息"skey check fail"
					ble_nus_string_send(&m_nus, (uint8_t *)checked_superkey_false, \
									strlen(checked_superkey_false) );
			
				}
			}
		break;
			 
		default:
		
		break;
	}
	
}
