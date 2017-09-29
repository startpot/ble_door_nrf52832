/*********************************************************************************
*	FM260B是一个指纹模块0x100个，采用uart通讯，默认波特率 57600bps
*   模块端：3--TXD   4--RXD     6---touch(手指探测信号)
*
*	模块上电后，在100ms内完成系统引导和初始化，向上位机发送
*	0x55 0x55，表示ready
*	通讯协议
*	命令包格式：包头(1byte：0x1B)
*							地址码(1byte：0xFF)
*							指令码(1byte：)
*							参数1(2byte：命令附加第一参数，若无，为0x0000)高字节
*							参数2(2byte：命令附加第二参数，若无，为0x0000)高字节
*							校验和(1byte：从包头到参数2的所有字节的算术累计的低字节)
*	应答包格式：包头(1byte：0x1B)
*							指令码(1byte：原操作码)
*							结果码(1byte：执行结果码)
*							参数长度(1byte：返回参数字节数n，若无，则为0x00)
*							参数1(1byte：)
*							参数2(1byte：)
*							........
*							参数n(1byte：)
*							校验和(1byte：从包头到参数n的所有字节的算术累计和的低字节)
*
*
************************************************************************************/
#ifndef	FM260B_H__
#define	FM260B_H__

#include <stdbool.h>
#include <stdint.h>


//上电后，系统发送准备好的码 DR:导纳电子
#define DR_READY_CODE		0x55


//包头、地址
#define	DR_FIG_START			0x1B
#define	DR_FIG_ADDER			0xFF
//命令包指令码
#define	DR_FIG_CMD_SETSYS		0x00
#define	DR_FIG_CMD_READINFO		0x01
#define	DR_FIG_CMD_AUTOENROLL	0x20
#define	DR_FIG_CMD_AUTOSEARCH	0x22
#define	DR_FIG_CMD_DELETE		0x1C

//应答包的数据位置
#define DR_FIG_R_CMD_CODE_SITE		1  //bit1
#define DR_FIG_R_RESULT_CODE_SITE 	2  //bit2
#define DR_FIG_R_PARAM_LEN_SITE		3  //bit3

//正确命令包的应答结果码
#define	DR_FIG_R_EXE			0x00

#define	DR_FIG_R_FINISH_TRUE	0x01
#define	DR_FIG_R_FINISH_FALSE	0x02

#define	DR_FIG_R_FIG_MOVE		0x05
#define	DR_FIG_R_FIG_TOUCH		0x06

//错误命令包的应答结果码
#define	DR_FIG_R_CMD_WRONG		0x40
#define	DR_FIG_R_PARAM_WRONG	0x41
#define	DR_FIG_R_SUM_WRONG		0x42


extern bool		fig_status;
extern bool		exe_result;
extern bool		is_fm260b_autoenroll;

extern uint16_t	dr_fig_param_first ;
extern uint16_t	dr_fig_param_second ;

void fig_fm260b_send_autosearch(void);
void fig_fm260b_reply_check(void);

#endif		//FM260B_H__
