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


//与获取和设置时间相关的变量
extern struct tm				time_set;
extern struct tm				time_get;
extern time_t					time_get_t;

//设置触摸屏密码
extern struct key_store_struct key_store_set;
extern struct key_store_struct key_store_get;

extern uint32_t				record_length_get;
extern uint32_t				key_store_length_get;

extern uint8_t				ble_operate_code;

extern uint8_t				fig_cmd_code;
extern bool					is_superkey_checked;

extern uint8_t				r301t_autoenroll_step;

extern uint8_t				enroll_fig_info_data[4];//注册指纹信息
extern uint8_t				delete_fig_info_data[4];//删除指纹信息
extern uint8_t 				enroll_fig_id[2];
extern uint8_t				delete_fig_id[2];

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
#define SET_TOUCH_KEY			0x8D	//设置触摸屏密码

#define USER_UNBIND_CMD			0x8E	//用户解除绑定

#define CHECK_SUPER_KEY			0x8F	//验证超级密码


#define	GET_KEY_NOW				0x87	//TODO 后期移除


#define ENROLL_FIG				0x70	//注册指纹
#define DELETE_FIG				0x71	//删除指纹
#define SEARCH_FIG				0x72	//搜素指纹
#define GET_FIG_INFO			0x73	//获取指纹信息
#define STOP_FIG				0x74	//停止指纹模块
#define DELETE_ALL_FIG			0x75	//删除所有指纹




void ble_reply(uint8_t operate_code, uint8_t *reply_code, uint16_t reply_code_length);
void operate_code_check(uint8_t *p_data, uint16_t length);

#endif  //OPERATE_CODE_H__
