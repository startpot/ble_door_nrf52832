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

struct key_store_struct				key_store_struct_set;

struct door_open_record				door_open_record_get;
struct tm 	time_record;//读出记录的时间
time_t 		time_record_t;//读出的时间的int
struct tm 	time_record_compare;//要对比的时间
time_t 		time_record_compare_t;//要对比的时间的int

//与获取和设置时间相关的变量
struct tm					time_set;
struct tm					time_get;
time_t						time_get_t;

uint32_t					record_length_get;
uint32_t					key_store_length_get;


/***********************************
*	设置开锁密码命令
************************************/
static int cmd_set_key(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	//获取收到的时间
	err_code = rtc_time_read(&time_get);
	if(err_code == NRF_SUCCESS)
	{
		time_get_t = my_mktime(&time_get);
	}
			
	//先进行现存密码的比对
	//获取普通密码的个数,小端字节
	inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, KEY_STORE_OFFSET, &block_id_flash_store);
	memcpy(&key_store_length,flash_read_data, sizeof(struct key_store_length_struct));
		
	if(key_store_length.key_store_full ==0x1)
	{
		key_store_length_get = KEY_STORE_NUMBER;
	}
	else if(key_store_length.key_store_length >0)
	{
		key_store_length_get = key_store_length.key_store_length;
	}
	if(key_store_length_get >0)
	{
		for(int i=0; i<key_store_length_get; i++)
		{
			//获取存储的密码
			inter_flash_read((uint8_t *)&key_store_check, sizeof(struct key_store_struct), \
													(KEY_STORE_OFFSET + 1 + i), &block_id_flash_store);
			//对比密码是否一致
			if(strncasecmp((char *)nus_data_recieve, (char *)&key_store_check.key_store, 6) == 0)
			{//密码相同，看是否在有效时间内
				if((double)(my_difftime(time_get_t, key_store_check.key_store_time) <\
								((double)key_store_check.key_use_time * 60)) )
					{
	
					ble_door_open();
#if defined(BLE_DOOR_DEBUG)
					printf("it is a dynamic key user set\r\n");
					printf("door open\r\n");
#endif
					//记录开门
					memset(&open_record_now, 0, sizeof(struct door_open_record));
					memcpy(&open_record_now.key_store, p_data, 6);
					memcpy(&open_record_now.door_open_time, &time_get_t, 4);
					record_write(&open_record_now);
					goto key_set_exit;
				}
			}					
		}
	}	
		
			//获取种子
			inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, SEED_OFFSET, &block_id_flash_store);
	//		memset(seed, 0, 16);
			if(flash_read_data[0] == 0x77)
			{//设置了种子
				//获取种子
				memcpy(seed, &flash_read_data[1], 16);
		
			//对比SET_KEY_CHECK_NUMBER次设置的密码
			for(int i=0; i<SET_KEY_CHECK_NUMBER; i++)
			{
				SM4_DPasswd(seed, time_get_t, SM4_INTERVAL, SM4_COUNTER, SM4_challenge, key_store_tmp);

				if(strncasecmp((char *)p_data, (char *)key_store_tmp, KEY_LENGTH) == 0)
				{//设置的密码相同

					//组织密码结构体
					memset(&key_store_struct_set, 0 , sizeof(struct key_store_struct));
					//写密码
					memcpy(&key_store_struct_set.key_store, p_data, 6);
					//写有效时间
					memcpy(&key_store_struct_set.key_use_time, &p_data[6], 2);
					//写控制字
					memcpy(&key_store_struct_set.control_bits, &p_data[8], 1);
					//写版本号
					memcpy(&key_store_struct_set.key_vesion, &p_data[9], 1);
					//写存入时间
					memcpy(&key_store_struct_set.key_store_time, &time_get_t, sizeof(time_t));
	
					//直接将钥匙记录到flash
					key_store_write(&key_store_struct_set);
#if defined(BLE_DOOR_DEBUG)
					printf("key set success\r\n");
#endif
					//开门
					ble_door_open();
					//记录开门
					memset(&open_record_now, 0, sizeof(struct door_open_record));
					memcpy(&open_record_now.key_store, p_data, 6);
					memcpy(&open_record_now.door_open_time, &time_get_t, sizeof(time_t));
					record_write(&open_record_now);
					
					goto key_set_exit;
				}
				else
				{
					time_get_t = time_get_t - 60;
				}
			}
		}
key_set_exit:
	
	return 0;
}

/********************************************
*同步时间命令
********************************************/

static void sync_rtc_time(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
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
		memcpy(&nus_data_send[1], &p_data[1], (length -1));
		nus_data_send_length = length;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
	}
}

/********************************************
*获取系统时间
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
	inter_flash_read(flash_read_data, 32, SEED_OFFSET, &block_id_flash_store);
	memset(seed, 0, 16);
		if(flash_read_data[0] == 0x77)
		{//设置了种子
		//获取种子
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

/*****************************************
*设置系统参数
*****************************************/
static void set_param(uint8_t *p_data, uint16_t length)
{
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
		
	memset(flash_write_data, 0, 8);
	//写入标记'w'
	flash_write_data[0] = 'w';
	memcpy(&flash_write_data[1], &p_data[1], 6);
		
	//将参数写入到flash
	inter_flash_write(flash_write_data, 8, DEFAULT_PARAMS_OFFSET, &block_id_flash_store);
		
	//应答包
	//将命令加上0x40,返回给app
	nus_data_send[0] = p_data[0] + 0x40;
	memcpy(&nus_data_send[1], &p_data[1], (length -1) );
	nus_data_send_length = length;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
}

/**********************************************
*设置系统的种子
**********************************************/
static void set_key_seed(uint8_t *p_data, uint16_t length)
{
	//传输的种子应该是小端字节
	memset(flash_write_data, 0, BLOCK_STORE_SIZE);
	//写入标记'w'
	flash_write_data[0] = 0x77;
	memcpy(&flash_write_data[1],&p_data[1], SEED_LENGTH);
		
	//将种子写入到flash
	inter_flash_write(flash_write_data, BLOCK_STORE_SIZE, SEED_OFFSET, &block_id_flash_store);
		
	//应答包
	//将命令加上0x40,返回给app
	nus_data_send[0] = p_data[0] + 0x40;
	memcpy(&nus_data_send[1], &p_data[1], (length -1));
	nus_data_send_length = length;
	ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
}

/***********************************************
*设置mac
***********************************************/
static void set_mac(uint8_t *p_data, uint16_t length)
{
	uint32_t err_code;
	char set_fail[13] = "set mac fail";
	if((p_data[6] &0xc0) ==0xc0)
	{//设置的mac最高2位为11，有效
		//存储mac地址
		memset(mac, 0, 8);
		mac[0] = 'w';
		mac[1] = 0x06;
		memcpy(&mac[3], &p_data[1], 6);
		inter_flash_write(mac, 8, MAC_OFFSET, &block_id_flash_store);
		//配置mac
		memset(addr.addr, 0, 6);
		//拷贝设置的mac
		memcpy(addr.addr, &p_data[1], 6);
		err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,&addr);
		if(err_code == NRF_SUCCESS)
		{
			//将命令加上0x40,返回给app
			nus_data_send[0] = p_data[0] + 0x40;
			memcpy(&nus_data_send[1], &p_data[1], (length -1));
			nus_data_send_length = length;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
	}
	else
	{
		//向手机发送失败信息"set mac fail"
		ble_nus_string_send(&m_nus, (uint8_t *)set_fail, strlen(set_fail) );
	}
}

/*************************************************
*设置管理员密码
**************************************************/
static void set_super_key(uint8_t *p_data, uint16_t length)
{
	memset(flash_write_data, 0, BLOCK_STORE_SIZE);
	memcpy(&flash_write_data[1],&p_data[1], SUPER_KEY_LENGTH);
	flash_write_data[0] = 0x77;//'w'
	//超级密码就12位，取写入数据前面16位(16>(1+12))
	write_super_key(flash_write_data,16);
}

/*****************************************
*获取现在存储的密码
******************************************/
static void get_used_key(uint8_t *p_data, uint16_t length)
{
	memset(nus_data_send, 0, BLE_NUS_MAX_DATA_LEN);
	//获取密码的数量，小端字节
	inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, KEY_STORE_OFFSET, &block_id_flash_store);
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
			inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, \
							 (KEY_STORE_OFFSET+1+i), &block_id_flash_store);
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
	inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, RECORD_OFFSET, &block_id_flash_store);
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
	char no_record[10] = "no record";
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
	inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, RECORD_OFFSET, &block_id_flash_store);
	memcpy(&record_length,flash_read_data, sizeof(struct record_length_struct));
		
	if(record_length.record_full ==0x1)
	{
		nus_data_send[1] = (uint8_t)RECORD_NUMBER;
		for(int i=0; i<RECORD_NUMBER; i++)
		{
			//读出记录
			inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, \
								(RECORD_OFFSET+1+i), &block_id_flash_store);
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
			inter_flash_read(flash_read_data, BLOCK_STORE_SIZE, \
								(RECORD_OFFSET+1+i), &block_id_flash_store);
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
	{//无记录，发送no record
		ble_nus_string_send(&m_nus, (uint8_t *)no_record, strlen(no_record) );
	}
}

/*******************************************************
*指纹模块fm260b命令处理,主要调用uart端口
*******************************************************/
static void send_fig_fm260b_cmd(uint8_t *p_data, uint16_t length)
{
	//获取第一参数和第二参数,两字节，大端
	fig_param_first = p_data[3] *256 + p_data[4];
	fig_param_second = p_data[5] *256 + p_data[6];
	//将获取的指令发送给指纹模块
	for (uint32_t i = 0; i < length; i++)
	{
		while(app_uart_put(p_data[i]) != NRF_SUCCESS);
	}
	
	if(p_data[2] == 0x20)//自动注册模块指令
	{
		//设置自动注册状态位
		is_autoenroll = true;
	}
}

/***********************************************************
*指纹模块r301t命令处理，调用uart端口
***********************************************************/
static void send_fig_r301t_cmd(uint8_t *p_data, uint16_t length)
{
	static uint8_t fig_r301t_autoenroll_reply[12]={0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,\
																				  0x07, 0x00, 0x03, 0x00, 0x00, 0x0A};
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

/************************************************************
*对nus servvice传来的数据进行分析
*in：	*p_data			处理的数据指针
			length				数据长度
***********************************************************/
void operate_code_check(uint8_t *p_data, uint16_t length)
{
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
			cmd_set_key(p_data, length);
		}
		break;
		
		case SYNC_TIME://同步时间
		if(length ==0x08)//8字节
		{
			sync_rtc_time(p_data, length);
		}
		break;
		
		case GET_TIME://获取时间
			get_rtc_time(p_data, length);
		break;
		
		case GET_KEY_NOW://获取现在的动态密码
			get_key_now(p_data, length);
		break;
		
		case SET_PARAMS://设置参量
		if(length == 0x7)//6字节
		{
			set_param(p_data, length);
		}
		break;
		
		case SET_KEY_SEED://写入种子
		if(length == 0x11)//17字节
		{
			set_key_seed(p_data, length);
		}
		break;
		
		case SET_MAC://配置mac,与显示的mac反向
		if(length ==0x07)//7字节
		{
			set_mac(p_data, length);
		}
		break;
		
		case SET_SUPER_KEY://设置管理员密码
		if(length == 0x0d)//13字节
		{
			set_super_key(p_data, length);
		}
		break;
		
		case GET_USED_KEY://查询有效密码
			get_used_key(p_data, length);
		break;
		
		case GET_RECORD_NUMBER://查询开门记录数量
			get_record_number(p_data, length);
		break;
		
		case GET_RECENT_RECORD://查询指定日期后的记录
		if(length == 0x08)//8字节
		{
			get_recent_record(p_data, length);
		}
		break;
		
		case 0x1B://指纹模块fm260b指令，长度为8，直接通过串口发送给模块
			if(length == 8)//长度为8
			{
				send_fig_fm260b_cmd(p_data, length);
			}
		break;
		
		case 0xEF:
			if(length >11) //传送命令包最少12位
			{
			send_fig_r301t_cmd(p_data, length);
			}
		break;
		
		default:
		
		break;
	}	
}
