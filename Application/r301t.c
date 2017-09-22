#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "nordic_common.h"
#include "nrf_gpio.h"

#include "app_uart.h"

#include "r301t.h"
#include "beep.h"
#include "led_button.h"
#include "ble_init.h"
#include "rtc_chip.h"
#include "my_time.h"
#include "custom_board.h"
#include "operate_code.h"


bool		is_r301t_autoenroll = false;
uint8_t 	r301t_autosearch_step = 0;


struct tm		fingprint_checked_tm;
time_t 			fingprint_checked_time_t;//指纹验证通过的时间
struct fp_store_struct fp_store_struct_get;//从内部flash中获取的指纹信息

void fig_r301t_send_getimage(void)
{
	static uint8_t send_getimage_data[12] = {0xEF,0x01, 0xFF, 0xFF, 0xFF, 0xFF,\
											 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
	//将命令通过uart发送给指纹模块
	for (uint32_t m = 0; m < 12; m++)
	{
		while(app_uart_put(send_getimage_data[m]) != NRF_SUCCESS);
	}
	
}

/**********************************************************
*	向指纹模块发送指令
*	data_id		包标识	bit6
*	data_len		包长度	bit7-8
*	data_code	包内容	bit9-...(共data_len-2个)
************************************************************/
void fig_r301t_send_cmd(uint8_t	data_id, uint16_t data_len, uint8_t	*data_code)
{
	uint8_t	send_data[UART_TX_BUF_SIZE];
	uint16_t sum = 0;
	//包头
	send_data[0]	=	0xEF;
	send_data[1]	=	0x01;
	//模块地址
	for(int i=GR_FIG_ADDR_SITE; i<(GR_FIG_ADDR_SITE+4); i++)
	{
		send_data[i] = GR_FIG_ADDR/(pow(2,(8*(3-i))));
		
	}
	//包标识
	send_data[GR_FIG_DATA_ID_SITE] = data_id;
	//包长度
	send_data[GR_FIG_DATA_LEN_SITE] = (data_len / 0x100);
	send_data[GR_FIG_DATA_LEN_SITE+1] = (data_len &0xFF);
	//包内容
	if(data_code !=NULL)
	{
	for(int n=9; n<(7+data_len); n++)
	{
		send_data[n] = data_code[n-9];
	}
	}
	
	//校验和,从6位开始计算
	for(int j=6; j<7+data_len; j++)
	{
		sum = sum+send_data[j];
	}
	send_data[GR_FIG_DATA_LEN_SITE+data_len - 1] = (sum/0x100);
	send_data[GR_FIG_DATA_LEN_SITE+data_len] = (sum &0xFF);
	
	//将命令通过uart发送给指纹模块
	for (uint32_t m = 0; m < (9 + data_len); m++)
	{
		while(app_uart_put(send_data[m]) != NRF_SUCCESS);
	}
	
}

/*********************************************
*指纹模块的应答处理
*********************************************/
void fig_r301t_reply_check(void)
{
	static uint8_t 		fp_keys_tmp[6];
	static uint16_t 	reply_data_len;
	static uint32_t		send_time;
	static uint32_t		send_left;
	static uint8_t		send_genchar_data[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,\
												 0x01,0x00,0x04,0x02,0x01, \
												 0x00,0x08};
	
	static uint8_t		send_search_data[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,\
												0x01,0x00,0x08,0x04,\
												0x01,0x00,0x00,0x0b,0xb8,\
												0x00,0xd1};
																													
																				
	//获取包长度
	reply_data_len = fig_recieve_data[GR_FIG_DATA_LEN_SITE]*0x100 + \
					 fig_recieve_data[GR_FIG_DATA_LEN_SITE +1];
		
	//当数据包的总长度等于包长度+9的时候收完总的数据包
	if(fig_recieve_data_length ==(9+reply_data_len))
	{
		//无论如何都会将结果返还给上位机的
		//将模块的返回包全部通过蓝牙串口返回给上位机
		if(fig_recieve_data_length <=BLE_NUS_MAX_DATA_LEN)
		{
			//数据长度小于20，一次发完
			memcpy(nus_data_send,fig_recieve_data, fig_recieve_data_length);
			nus_data_send_length = fig_recieve_data_length;
			ble_nus_string_send(&m_nus,nus_data_send, nus_data_send_length);
		}
		else
		{
			//计算一共发送几次整的20字节
			send_time = fig_recieve_data_length / BLE_NUS_MAX_DATA_LEN;
			send_left = fig_recieve_data_length % BLE_NUS_MAX_DATA_LEN;
				
			//发送整20字节的
			for(int i=0; i<send_time; i++)
			{
				memcpy(nus_data_send,&fig_recieve_data[i*BLE_NUS_MAX_DATA_LEN], BLE_NUS_MAX_DATA_LEN);
				nus_data_send_length = BLE_NUS_MAX_DATA_LEN;
				ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			}
			//发送剩余的字节
			if(send_left >0)
			{
				memcpy(nus_data_send,&fig_recieve_data[send_time * BLE_NUS_MAX_DATA_LEN], send_left);
				nus_data_send_length = send_left;
				ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			}		
		}
		
		//判断发送包指令码
		if(fp_cmd_code == GR_FIG_CMD_STORECHAR || \
			fp_cmd_code == GR_FIG_CMD_DELCHAR || \
			fp_cmd_code == GR_FIG_CMD_EMPTY)
		{
		//关闭指纹芯片电源电源
		nrf_gpio_pin_clear(BATTERY_LEVEL_EN);
		}
		
		
		//分析数据包,如果不是自动注册模式,且是手指按下进入了搜索模式
		if(is_r301t_autoenroll ==false)
		{
			if(	r301t_autosearch_step >0)
			{
				//手指按下设置的自动搜索模式，
				//应答包失败
				if(fig_recieve_data[9] !=0x00)
				{
					//应答失败，鸣笛4次
					beep_didi(4);
					//失败情况，如果是第一步则重复发GR_GetImage
					if(r301t_autosearch_step == 1)
					{
						//第一步执行失败，继续发送getimage命令
						fig_r301t_send_getimage();
					}
					else
					{//如果不是第一步，则直接退出
						r301t_autosearch_step = 0;
					}
					fig_recieve_data_length =0;
				}
				else
				{
					if(	r301t_autosearch_step == 1 &&fig_recieve_data_length != 0)
					{
						//第一步的应答包的话，发送第2个指令，设置步骤为2
						for (uint32_t m = 0; m < 13; m++)
						{
							while(app_uart_put(send_genchar_data[m]) != NRF_SUCCESS);
						}
						r301t_autosearch_step =2;
						fig_recieve_data_length = 0;
					}
					if( r301t_autosearch_step == 2 &&fig_recieve_data_length != 0)
					{
						//第二步的应答包的话，发送第3个指令，设置步骤为3
						for (uint32_t m = 0; m < 17; m++)
						{
							while(app_uart_put(send_search_data[m]) != NRF_SUCCESS);
						}
						r301t_autosearch_step =3;
						fig_recieve_data_length = 0;
					}
					if( r301t_autosearch_step ==3 &&fig_recieve_data_length != 0)
					{
						//最后一步，判断结果
						if(fig_recieve_data[9] == 0)
						{	//返回搜索到了指纹
							
							//记录指纹开锁
							//1.组织开锁记录(6B)(fp，2B的指纹ID，2B的matchscore)+时间
							//1.1获取指纹验证通过的时间
							rtc_time_read(&fingprint_checked_tm);
							fingprint_checked_time_t = my_mktime(&fingprint_checked_tm);
							
							//1.2 读取存储的指纹信息
							fp_read(&fp_store_struct_get, (fig_recieve_data[12]*(0x100) + fig_recieve_data[13]));
							
							//1.3如果设置了有效时间
							if(fp_store_struct_get.fp_wr_flag == 'w')
							{
								goto fp_check_use_time;
							}
							else
							{
								goto fp_uncheck_use_time;
							}
														
		fp_check_use_time:							
							if((double)my_difftime(fingprint_checked_time_t, fp_store_struct_get.fp_store_time) >= \
														(fp_store_struct_get.fp_use_time* 60))
							{
							
		fp_uncheck_use_time:
								//开锁
								ble_door_open();
								//1.2.1组织指纹的密码结构
								memset(&open_record_now, 0, sizeof(struct door_open_record));
								fp_keys_tmp[0] = 'f';
								fp_keys_tmp[1] = 'p';
								memcpy(&fp_keys_tmp[2], &fig_recieve_data[10], 2);
								memcpy(&fp_keys_tmp[4], &fig_recieve_data[12], 2);
							
								memcpy(&open_record_now.key_store, fp_keys_tmp, 6);
								memcpy(&open_record_now.door_open_time, &key_input_time_t, 4);
								//1.2.2记录指纹开锁
								record_write(&open_record_now);
								
								//关闭指纹芯片电源电源
								nrf_gpio_pin_clear(BATTERY_LEVEL_EN);
								
							}
						}
						//设置步骤为0，状态为false
						r301t_autosearch_step = 0;
						fig_recieve_data_length = 0;
					}
				}
			}
		}
		//收到数据长度清零
		fig_recieve_data_length = 0;
	}
	
}
