#ifndef OPERATE_CODE_H__
#define OPERATE_CODE_H__


#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "inter_flash.h"
#include "ble_nus.h"


extern struct tm				time_record;
extern time_t					time_record_t;
extern struct tm				time_record_compare;
extern time_t					time_record_compare_t;

extern struct tm				fp_set_tm;
extern time_t					fp_set_time_t;
extern struct fp_store_struct	fp_store_struct_set;

//与获取和设置时间相关的变量
extern struct tm				time_set;
extern struct tm				time_get;
extern time_t					time_get_t;

extern uint32_t				record_length_get;
extern uint32_t				key_store_length_get;

extern bool		is_superkey_checked;

/**********************************
* 数据包的分析
***********************************/
#define OPERATE_CODE_BIT		0	//命令位，除去用户设置密码的命令


#define	SYNC_TIME				0x80
#define	SET_PARAMS				0x81
#define	SET_KEY_SEED			0x82
#define	SET_MAC					0x83
#define	SET_BLE_UUID			0x84
#define	SET_SUPER_KEY			0x85	//设置管理员密码
#define GET_TIME				0x86
#define	GET_USED_KEY			0x88
#define	GET_RECORD_NUMBER		0x89
#define	GET_RECENT_RECORD		0x8A
#define GET_MAC					0x8B	//返回mac
#define GET_BATTERY_LEVEL		0x8C	//返回电池电量

#define USER_UNBIND_CMD			0x8E	//用户解除绑定

#define CHECK_SUPER_KEY			0x8F	//验证超级密码


#define	GET_KEY_NOW				0x87	//TODO 后期移除


#define SET_FP_USE_TIME			0x70	//设置指纹有效期


void operate_code_check(uint8_t *p_data, uint16_t length);

#endif  //OPERATE_CODE_H__
