#ifndef OPERATE_CODE_H__
#define OPERATE_CODE_H__


#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "inter_flash.h"
#include "ble_nus.h"



extern bool						is_ble_cmd_exe;//ble命令是否在执行

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

extern uint8_t				ble_operate_code;

extern uint8_t				fig_cmd_code;
extern bool					is_superkey_checked;

extern uint8_t				r301t_autoenroll_step;

extern uint8_t 				enroll_fig_id[2];
extern uint8_t				delete_fig_id[2];

/**********************************
* 数据包的分析
***********************************/
#define OPERATE_CODE_BIT		0	//命令位，除去用户设置密码的命令

//系统设置相关
#define	SET_SUPER_KEY			0x80	//设置管理员密码
#define CHECK_SUPER_KEY			0x81	//验证超级密码

#define	SYNC_TIME				0x82	//对时
#define	SET_KEY_SEED			0x83	//设置种子

#define	SET_MAC					0x84	//设置mac
#define GET_MAC					0x85	//获取mac

#define	SET_PARAMS				0x86	//设置工作参数

#define	GET_KEY_NOW				0x87	//TODO 后期移除

#define GET_TIME				0x88	//获取时间

#define GET_BATTERY_LEVEL		0x8C	//返回电池电量

#define USER_UNBIND_CMD			0x8F	//用户解除绑定


//触摸按键相关
#define SET_TOUCH_KEY			0x90	//设置触摸屏密码
#define DELETE_TOUCH_KEY		0x91	//删除触摸屏密码
#define	GET_TOUCH_KEY_STORE		0x92	//查询设置的键盘密码

//开门记录相关
#define	GET_RECORD_NUMBER		0x9A	//获取记录数量
#define	GET_RECENT_RECORD		0x9B	//获取最近的记录


//指纹模块相关
#define ENROLL_FIG				0xA0	//注册指纹
#define DELETE_FIG				0xA1	//删除指纹
#define SEARCH_FIG				0xA2	//搜素指纹
#define GET_FIG_INFO			0xA3	//获取指纹信息
#define STOP_FIG				0xA4	//停止指纹模块
#define DELETE_ALL_FIG			0xA5	//删除所有指纹
#define	GET_FIG_NUMBER			0xA6	//读取指纹个数
#define	GET_FIG_INDEXTABLE		0xA7	//读取指纹模块存储的指纹信息

void ble_reply(uint8_t operate_code, uint8_t *reply_code, uint16_t reply_code_length);
void open_fig(void);
void close_fig(void);
void operate_code_check(uint8_t *p_data, uint16_t length);

#endif  //OPERATE_CODE_H__
