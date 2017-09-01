#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nordic_common.h"

#include "app_uart.h"

#include "fm260b.h"
#include "beep.h"
#include "led_button.h"
#include "ble_init.h"

static void ble_set_fig_free(void)
{
	static uint8_t fig_cmd_free[8]={0x1B,0xFF,0x00, 0x00,0x00, 0x00,0x00, 0x1A};
	//发送free指令，使模块进入低功耗
	for (uint32_t i = 0; i < sizeof(fig_cmd_free); i++)
	{
		while(app_uart_put(fig_cmd_free[i]) != NRF_SUCCESS);
	}
}

/*********************************
*指纹模块应答包处理模块
*********************************/
void fig_fm260b_reply_check(void)
{
//------------- 初始化上电后 --------接收到准备命令
		//返回包长2
			if( (fig_send_data_array_length==2) &&\
					(fig_send_data_array[0]==READY_CODE) && (fig_send_data_array[1]==READY_CODE ) )
			{
				//设置指纹模块的状态位ture
				fig_status = true;
				fig_send_data_array_length = 0;
			}
			
			//----------------ERROR CMD   REPLY----------错误的指令包
			//返回包长5
			if( (fig_send_data_array_length==5) &&\
						( 	(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == FIG_R_CMD_WRONG) ||\
							(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == FIG_R_PARAM_WRONG) ||\
							(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == FIG_R_SUM_WRONG)  )  )
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			
			//-------CMD 1-1  REPLY-----------------SetSys命令的应答包
			//返回包长5
			if( (fig_send_data_array_length==5)&&\
						(fig_send_data_array[FIG_R_CMD_CODE_SITE]==FIG_CMD_SETSYS) )
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length =0;
			}
			
			//---------CMD 2-1 REPLY--------------ReadInfo命令的应答包
			//检查单个指纹模板，返回包长5
			if( (fig_send_data_array_length==5) &&\
						(fig_send_data_array[FIG_R_CMD_CODE_SITE] == FIG_CMD_READINFO) &&\
						(fig_param_first == 0x0001) )
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			//-----------CMD 2-2 REPLY--------------ReadInfo命令的应该包
			//读取指纹库连续64个ID号对应位置是否有指纹，返回包长13
			if( (fig_send_data_array_length==13) &&\
						(fig_send_data_array[FIG_R_CMD_CODE_SITE] == FIG_CMD_READINFO) &&\
						(fig_param_first == 0x0010) )
			{
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			//-------------CMD 2-3 REPLY -------------ReadInfo命令的应答包
			//读取产品型号，返回包长15
			if( fig_send_data_array_length == 15 &&\
				fig_param_first == 0x0002 && fig_param_second == 0x0001 &&\
					fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_READINFO)
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			//-------------CMD 2-4 REPLY -------------ReadInfo命令的应答包
			//读取软件版本号和日期，返回包长17
			if( fig_send_data_array_length == 17 &&\
				fig_param_first == 0x0002 && fig_param_second == 0x0002 &&\
					fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_READINFO)
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			//-------------CMD 2-5 REPLY -------------ReadInfo命令的应答包
			//读取指纹库容量，返回包长7
			if( fig_send_data_array_length == 7 &&\
				fig_param_first == 0x0002 && fig_param_second == 0x0003 &&\
					fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_READINFO)
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			//-------------CMD 2-6 REPLY -------------ReadInfo命令的应答包
			//读取SN号，返回包长9
			if( fig_send_data_array_length == 9 &&\
				fig_param_first == 0x0002 && fig_param_second == 0x0004 &&\
					fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_READINFO)
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//set fig free
				ble_set_fig_free();
			}
			
			//-------------CMD 2-7 REPLY-----------ReadInfo命令的应答包
			//探测手指，返回包长5
			if( (fig_send_data_array_length == 5) && \
				(fig_param_first==0x0008) && (fig_param_second == 0x0001)  && \
				fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_READINFO )
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
			}
			
			//-------CMD 3-1 REPLY ----自动注册模板AutoEnroll
			//注释：直接上传结果给上位机，蓝牙芯片不做控制，其实手指一直放到上面即可
			//返回包长5
			if( (fig_send_data_array_length == 5) && \
					(fig_send_data_array[FIG_R_CMD_CODE_SITE]==FIG_CMD_AUTOENROLL) )
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//判断自动注册是否完成
				if( (fig_send_data_array[FIG_R_RESULT_CODE_SITE] == 0x01) || \
					(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == 0x02) )
				{
					is_autoenroll = false;
					//注册指纹完成，蜂鸣器响5次
					beep_didi(5);
				}
			}
				
			//-----------CMD 4-1 REPLY----------自动搜索模块AutoSearch,按下手指发，发送的前两步的指令
			//返回包长5
			if(  (fig_send_data_array_length == 5) && \
				(fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_AUTOSEARCH) )
			{
				//判断结果码
				switch(fig_send_data_array[FIG_R_RESULT_CODE_SITE])
				{
					case 0x00://第一个应答包，之后连续连续探测手指，将结果返回给上位机
						//蜂鸣器滴滴一声
				//		beep_didi(1);
						//将结果返回上位机
						ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
						fig_send_data_array_length = 0;
						break;
					case 0x05://第二个应答包，提示已录好要比对的指纹，将结果返回给上位机
						//蜂鸣器滴滴二声
				//		beep_didi(2);
						//将结果返回上位机
						ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
						fig_send_data_array_length = 0;
						break;
					
					default:
						break;	
				}
			}
			
			//-----------CMD 4-2 REPLY------自动搜索模板AutoSearch，第三步，如果检索到指纹，发送结果
			//返回包长9
			if(  (fig_send_data_array_length == 9) &&  \
					(fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_AUTOSEARCH)  && \
					(fig_send_data_array[FIG_R_RESULT_CODE_SITE] ==0x01) )
			{
				//蜂鸣器滴滴三声
				beep_didi(3);
				//开门
				ble_door_open();
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//发送free指令，使模块进入低功耗
				ble_set_fig_free();
			}
					
			//------------CMD 4-3 REPLY--------自动搜索模板AutoSearch,第三步，如果无比对的指纹
			//返回包长6
			if(  (fig_send_data_array_length == 6) &&  \
					(fig_send_data_array[FIG_R_CMD_CODE_SITE] ==FIG_CMD_AUTOSEARCH)  && \
					(fig_send_data_array[FIG_R_RESULT_CODE_SITE] ==0x02) )
			{
				//搜索指纹失败，不开门，将结果返回给上位机
				//蜂鸣器滴滴四声
				beep_didi(4);
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//发送free指令，使模块进入低功耗
				ble_set_fig_free();	
				}	
			
			//--------CMD 5-1 REPLY -------------删除指纹模块Delete
			//返回包长5
			if( (fig_send_data_array_length == 5) && \
				(fig_send_data_array[FIG_R_CMD_CODE_SITE] == FIG_CMD_DELETE) )
			{
				//将结果返回上位机
				ble_nus_string_send(&m_nus,fig_send_data_array,fig_send_data_array_length);
				fig_send_data_array_length = 0;
				//发送free指令，使模块进入低功耗
				ble_set_fig_free();
			}
}
