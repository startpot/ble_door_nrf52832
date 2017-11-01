#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "nordic_common.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

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

uint8_t		r301t_indextable_data[32];
bool		is_get_r301t_indextable = false;
uint8_t		fig_info_get_site;


//发送获取指纹图像命令
uint8_t		r301t_send_getimg_cmd[1] = {0x01};
//将生成的图像生成到charbuff1
uint8_t		r301t_send_genchar1_cmd[2]  = {0x02, 0x01};
//将生成的图像生成到charbuff2
uint8_t		r301t_send_genchar2_cmd[2] = {0x02, 0x02};
//发送搜索指纹模式，一共有32个指纹
uint8_t		r301t_send_search_cmd[6] = {0x04, 0x01, 0x00, 0x00, 0x00, 0x20};
//将charbuff1与charbuffer2中的特征文件合并生成模板存于charbuff1与charbuff2
uint8_t		r301t_send_regmodel_cmd[1] = {0x05};
//将特征缓冲区的文件储存在flash指纹库这里是ID0
uint8_t		r301t_send_storechar_id0_cmd[4] = {0x06, 0x02, 0x00, 0x00};
uint8_t	r301t_send_storechar_idx_cmd[4];
//删除指纹命令,这里是ID0
uint8_t		r301t_send_deletechar_id0_cmd[5] = {0x0c, 0x00, 0x00, 0x00, 0x01};
uint8_t 	r301t_send_deletechar_idx_cmd[5];
//清空指纹库命令
uint8_t		r301t_send_empty_cmd[1] = {0x0d};
//获取有效模板个数
uint8_t		r301t_send_get_vtnum_cmd[1] = {0x1d};
//读取索引列表
uint8_t		r301t_send_get_indextb_cmd[2] = {0x1f, 0x00};
uint8_t		r301t_send_get_indextbx_cmd[2] = {0x1f, 0x00};


/**********************************************************
*	向指纹模块发送指令
*	data_id		包标识	bit6
*	data_len		包长度-2	bit7-8
*	data_code	包内容	bit9-...(共data_len-2个)
************************************************************/
void fig_r301t_send_cmd(uint8_t	data_id, uint16_t data_len, uint8_t	*data_code) {
	uint8_t	send_data[UART_TX_BUF_SIZE];
	uint16_t sum = 0;
	//包头
	send_data[0]	=	0xEF;
	send_data[1]	=	0x01;
	//模块地址
	for(int i=GR_FIG_ADDR_SITE; i<(GR_FIG_ADDR_SITE+4); i++) {
		send_data[i] = (GR_FIG_ADDR>>(8*(i-GR_FIG_ADDR_SITE)) &0xff);

	}
	//包标识
	send_data[GR_FIG_DATA_ID_SITE] = data_id;
	//包长度
	send_data[GR_FIG_DATA_LEN_SITE] = ((data_len +2) / 0x100);
	send_data[GR_FIG_DATA_LEN_SITE+1] = ((data_len + 2 )&0xFF);
	//包内容
	if(data_code !=NULL) {
		memcpy(&send_data[9], data_code, data_len );
	}

	//校验和,从6位开始计算
	for(int j=6; j<(7 +1+ data_len + 2 -1); j++) {
		sum = sum+send_data[j];
	}
	send_data[GR_FIG_DATA_LEN_SITE +1+data_len + 2 - 1] = (sum/0x100);
	send_data[GR_FIG_DATA_LEN_SITE+1+ data_len + 2] = (sum &0xFF);
	//将命令通过uart发送给指纹模块
	for (uint32_t m = 0; m < (9 + data_len + 2 ); m++) {
		while(app_uart_put(send_data[m]) != NRF_SUCCESS);
		//    printf("%02X ",send_data[m]);
	}

}
/******************************
*将指纹模块的信息返回给上位机
******************************/
static void send_fig_r301t_reply_data(void) {
	static uint32_t		send_time;
	static uint32_t		send_left;
	//无论如何都会将结果返还给上位机的
	//将模块的返回包全部通过蓝牙串口返回给上位机
	if(fig_recieve_data_length <=BLE_NUS_MAX_DATA_LEN) {
		//数据长度小于20，一次发完
		memcpy(nus_data_send,fig_recieve_data, fig_recieve_data_length);
		nus_data_send_length = fig_recieve_data_length;
		ble_nus_string_send(&m_nus,nus_data_send, nus_data_send_length);
	} else {
		//计算一共发送几次整的20字节
		send_time = fig_recieve_data_length / BLE_NUS_MAX_DATA_LEN;
		send_left = fig_recieve_data_length % BLE_NUS_MAX_DATA_LEN;

		//发送整20字节的
		for(int i=0; i<send_time; i++) {
			memcpy(nus_data_send,&fig_recieve_data[i*BLE_NUS_MAX_DATA_LEN], BLE_NUS_MAX_DATA_LEN);
			nus_data_send_length = BLE_NUS_MAX_DATA_LEN;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
		//发送剩余的字节
		if(send_left >0) {
			memcpy(nus_data_send,&fig_recieve_data[send_time * BLE_NUS_MAX_DATA_LEN], send_left);
			nus_data_send_length = send_left;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
	}

}

/***************************************
*获取指纹模块索引列表的返回包
***************************************/
static void send_get_r301t_indextable_reply_data(void) {
	static uint32_t		send_time;
	static uint32_t		send_left;
	//判断返回索引表的字节数(不带校验和)
	if((fig_recieve_data_length - 12)<=(BLE_NUS_MAX_DATA_LEN - 1)) {
		//数据长度小于20，一次发完
		//命令码+0x40
		nus_data_send[0] = ble_operate_code + 0x40;
		memcpy(&nus_data_send[1],&fig_recieve_data[10], (fig_recieve_data_length -12));
		nus_data_send_length = (fig_recieve_data_length - 12);
		ble_nus_string_send(&m_nus,nus_data_send, (nus_data_send_length -12));
	} else {
		//计算一共发送几次整的20字节
		send_time = (fig_recieve_data_length - 12)/ (BLE_NUS_MAX_DATA_LEN - 1);
		send_left = (fig_recieve_data_length - 12) % (BLE_NUS_MAX_DATA_LEN - 1);

		//发送整20字节的
		for(int i=0; i<send_time; i++) {
			//命令码+0x40
			nus_data_send[0] = ble_operate_code + 0x40;
			memcpy(&nus_data_send[1],&fig_recieve_data[ 10 + i*(BLE_NUS_MAX_DATA_LEN - 1)], (BLE_NUS_MAX_DATA_LEN - 1));
			nus_data_send_length = BLE_NUS_MAX_DATA_LEN;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
		//发送剩余的字节
		if(send_left >0) {
			//命令码+0x40
			nus_data_send[0] = ble_operate_code + 0x40;
			memcpy(&nus_data_send[1],&fig_recieve_data[10 + send_time * (BLE_NUS_MAX_DATA_LEN - 1)], send_left);
			nus_data_send_length = send_left + 1;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}
	}

}

/***********************************************
*获取指纹信息索引列表
***********************************************/
static void get_r301t_indextable_data(void) {
	//1获取指纹模块返回信息
	memset(r301t_indextable_data, 0, sizeof(r301t_indextable_data));
	memcpy(r301t_indextable_data, &fig_recieve_data[10],sizeof(r301t_indextable_data));
	//2设置标志量
	is_get_r301t_indextable = true;

}

/*******************************************
*同步指纹模块
********************************************/
static void updata_fig_info(void) {
	for(int i=0; i < (FIG_INFO_NUMBER/8); i++) {
		for(int j=0; j < 8; j++) {
			//1、如果指纹ID号有指纹则写信息
			if( ((r301t_indextable_data[i] &(uint8_t)(0x01 <<j)) >> j) == 1) {
				//1.1、获取查询的ID号
				fig_info_get_site = i*8 + j;

				//1.2、获取内部flash存储区的信息
				pstorage_block_identifier_get(&block_id_flash_store, \
				                              (pstorage_size_t)(FIG_INFO_OFFSET + fig_info_get_site), &block_id_fig_info);
				pstorage_load(interflash_read_data, &block_id_fig_info, BLOCK_STORE_SIZE, 0);
				memset(&fig_info_get, 0, sizeof(struct fig_info));
				memcpy(&fig_info_get, interflash_read_data, sizeof(struct fig_info));

				if(fig_info_get.is_store != 'w') {
					//1.3、如果没有记录,则写入记录信息
					//1.3.1、设置信息
					memset(&fig_info_set, 0, sizeof(struct fig_info));
					fig_info_set.is_store = 'w';
					fig_info_set.fig_info_id = fig_info_get_site;
					//1.3.2、清除区域
					pstorage_clear(&block_id_fig_info, BLOCK_STORE_SIZE);
					//1.3.3、存储信息
					memset(interflash_write_data, 0, BLOCK_STORE_SIZE);
					memcpy(interflash_write_data, &fig_info_set, sizeof(struct fig_info));
					pstorage_store(&block_id_fig_info, interflash_write_data, BLOCK_STORE_SIZE, 0);
					//拷贝设置的信息
					memcpy(&fig_info_get, &fig_info_set, sizeof(struct fig_info));
				}
				//写了指纹信息,将指令码+40，再把指纹信息返回给上位机
				nus_data_send[0] = ble_operate_code + 0x40;
				nus_data_send[1] = fig_info_get.fig_info_id / 0x100;
				nus_data_send[2] = fig_info_get.fig_info_id &0xff;
				memcpy(&nus_data_send[3], fig_info_get.fig_info_data, sizeof(fig_info_get.fig_info_data));
				nus_data_send_length = 3+sizeof(fig_info_get.fig_info_data);
				ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			}
		}
	}
	fig_info_get_site = 0;
}

/*********************************************
*指纹模块的应答处理
*********************************************/
int fig_r301t_reply_check(void) {
	int err_code;
	uint8_t fig_input[6];

	uint8_t end_reply[4] = {0, 0, 0, 0};

	//send_fig_r301t_reply_data();
	//判断发送包的指令码
	switch(fig_cmd_code) {
	case GR_FIG_CMD_DELCHAR:
		//关闭指纹芯片
		close_fig();

		//将命令加上0x40,返回给app
		nus_data_send[0] = ble_operate_code + 0x40;
		//判断结果码
		if(fig_recieve_data[9] ==0x00) {
			nus_data_send[1] = 0x00;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			//删除记录的指纹信息
			//1.1、获取内部flash存储区的信息
redelete_fig_info:
			pstorage_block_identifier_get(&block_id_flash_store, \
			                              (pstorage_size_t)(FIG_INFO_OFFSET+(delete_fig_id[0]*0x100 + delete_fig_id[1])), &block_id_fig_info);
			err_code = pstorage_clear(&block_id_fig_info, BLOCK_STORE_SIZE);
			if(err_code !=0) {
				goto redelete_fig_info;
			}
		} else {
			nus_data_send[1] = 0x01;
			nus_data_send_length = 2;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		}

		break;

	case GR_FIG_CMD_EMPTY:
		//1、关闭指纹芯片
		close_fig();

		//2、清除所有指纹库信息
		for(int i = 0; i <FIG_INFO_NUMBER; i++) {
			pstorage_block_identifier_get(&block_id_flash_store, \
			                              (pstorage_size_t)(FIG_INFO_OFFSET+ i), &block_id_fig_info);
			pstorage_clear(&block_id_fig_info, BLOCK_STORE_SIZE);
		}
		//3、返回包
		nus_data_send[0] = ble_operate_code + 0x40;
		nus_data_send[1] = 0x00;
		nus_data_send_length = 2;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		break;

	case GR_FIG_CMD_VTNUM:
		//关闭指纹芯片
		close_fig();
		//返回包
		nus_data_send[0] = ble_operate_code + 0x40;
		nus_data_send[1] = fig_recieve_data[9];
		nus_data_send[2] = fig_recieve_data[10];
		nus_data_send[3] = fig_recieve_data[11];
		nus_data_send_length = 4;
		ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
		break;
	case GR_FIG_CMD_RDINDEXTB:
		//关闭指纹芯片
		close_fig();
		if(fig_recieve_data[9] == 0x00) {

			get_r301t_indextable_data();
updata_fig_info_label:
			//2、同步指纹信息
			if(is_get_r301t_indextable ==true) {
				updata_fig_info();
				is_get_r301t_indextable = false;
			} else {
				goto updata_fig_info_label;
			}
			//3、获取flash中的指纹信息
			/*		for(int i = 0; i < FIG_INFO_NUMBER; i++) {
						//1.1、获取内部flash存储区的信息
						pstorage_block_identifier_get(&block_id_flash_store, \
						                              (pstorage_size_t)(FIG_INFO_OFFSET+i), &block_id_fig_info);
						pstorage_load(interflash_read_data, &block_id_fig_info, BLOCK_STORE_SIZE, 0);
						memset(&fig_info_get, 0, sizeof(struct fig_info));
						memcpy(&fig_info_get, interflash_read_data, sizeof(struct fig_info));

						if(fig_info_get.is_store == 'w') {
							//写了指纹信息,将指令码+40，再把指纹信息返回给上位机
							nus_data_send[0] = ble_operate_code + 0x40;
							nus_data_send[1] = fig_info_get.fig_info_id / 0x100;
							nus_data_send[2] = fig_info_get.fig_info_id &0xff;
							memcpy(&nus_data_send[3], fig_info_get.fig_info_data, sizeof(fig_info_get.fig_info_data));
							nus_data_send_length = 3+sizeof(fig_info_get.fig_info_data);
							ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
						}
					}*/
			//4、结束包
			ble_reply(ble_operate_code, end_reply, sizeof(end_reply));
		}
		break;

	default:

		break;
	}

	//分析数据包,如果不是自动注册模式,且是手指按下进入了搜索模式
	if(is_r301t_autoenroll ==false && r301t_autosearch_step >0) {
		//手指按下设置的自动搜索模式，
		//应答包失败
		if(fig_recieve_data[9] !=0x00) {//失败的话，关闭电源模块，将步骤和错误码发送给上位机
			//失败情况，如果是第一步则重复发GR_GetImage
			/*	if(r301t_autosearch_step == 1) {
					//第一步执行失败，继续发送getimage命令
					fig_r301t_send_cmd(0x01, sizeof(r301t_send_getimg_cmd), \
					                   r301t_send_getimg_cmd);
					fig_recieve_data_length =0;
				} else {*/
			//关闭指纹模块
			close_fig();

			//返回第几步
			//将命令加上0x40,返回给app
			nus_data_send[0] = ble_operate_code;
			//第几步
			nus_data_send[1] = r301t_autosearch_step;
			nus_data_send[2] = fig_recieve_data[9];
			nus_data_send_length = 3;
			ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
			//应答失败，鸣笛4次
			beep_didi(4);
			//如果不是第一步，则直接退出
			r301t_autosearch_step = 0;
			//	}
			fig_recieve_data_length =0;
		} else {
			//判断自动搜索的步骤
			switch(r301t_autosearch_step) {
			case 1://第一步的应答包的话，发送第2个指令，设置步骤为2,发送genchar生成特征值命令
				r301t_autosearch_step =2;
				fig_recieve_data_length = 0;
				fig_cmd_code = GR_FIG_CMD_GENCHAR;
				fig_r301t_send_cmd(0x01, sizeof(r301t_send_genchar1_cmd), \
				                   r301t_send_genchar1_cmd);
				break;

			case 2://第二步的应答包的话，发送第3个指令，设置步骤为3,发送搜索指纹命令
				r301t_autosearch_step =3;
				fig_recieve_data_length = 0;
				fig_cmd_code = GR_FIG_CMD_SEARCH;
				fig_r301t_send_cmd(0x01, sizeof(r301t_send_search_cmd), \
				                   r301t_send_search_cmd);
				break;

			case 3://第三步，判断结果码
				//最后一步，判断结果
				if(fig_recieve_data[9] == 0) {
					//返回搜索到了指纹
					//向上位机发送匹配的ID号和匹配得分
					nus_data_send[0] = SEARCH_FIG + 0x40;
					nus_data_send[1] = fig_recieve_data[10];
					nus_data_send[2] = fig_recieve_data[11];
					nus_data_send[3] = fig_recieve_data[12];
					nus_data_send[4] = fig_recieve_data[13];
					nus_data_send_length = 5;
					ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
					//打开门
					ble_door_open();
					//TODO记录指纹开锁
					//获取按下开锁键的时间
					rtc_time_read(&key_input_time_tm);
					key_input_time_t = my_mktime(&key_input_time_tm);
					memset(fig_input, 0, sizeof(fig_input));
					fig_input[0] = 'f';
					fig_input[1] = 'g';
					memcpy(&fig_input[2], &fig_recieve_data[10], 4);
					door_open_record_flash((char *)fig_input,6, key_input_time_t);
				}
				//设置步骤为0，状态为false
				r301t_autosearch_step = 0;
				fig_recieve_data_length = 0;
				//关闭指纹芯片
				close_fig();
				break;

			default:

				break;
			}
		}
	} else { //自动注册模式
		if(r301t_autoenroll_step > 0) {
			//上位机设置自动注册模式
			if(fig_recieve_data[9] != 0x00) {
				if(r301t_autoenroll_step == 1) {
					//第二步继续发送getimg
					fig_r301t_send_cmd(0x01, sizeof(r301t_send_getimg_cmd), \
					                   r301t_send_getimg_cmd);
				} else {
					//注册失败，将命令加上0x40,+01,返回给app
					nus_data_send[0] = ble_operate_code + 0x40;
					//第几步
					nus_data_send[1] = 0x01;
					nus_data_send_length = 2;
					ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);

					//应答包，失败
					//设置步骤为0，接收数据长度清零
					r301t_autoenroll_step = 0x00;
					fig_recieve_data_length = 0;
					is_r301t_autoenroll = false;
					//关闭指纹模块
					close_fig();
					return 0;
				}
			} else {
				switch(r301t_autoenroll_step) {
				case 1://第1步，发送Genchar1命令，设置步骤为2
					r301t_autoenroll_step = 2;
					fig_recieve_data_length = 0;
					fig_cmd_code = GR_FIG_CMD_GENCHAR;
					fig_r301t_send_cmd(0x01, sizeof(r301t_send_genchar1_cmd), \
					                   r301t_send_genchar1_cmd);
					break;

				case 2://第2步，发送getimg命令，设置步骤为3
					r301t_autoenroll_step = 3;
					fig_recieve_data_length = 0;
					fig_cmd_code = GR_FIG_CMD_GETIMG;
					fig_r301t_send_cmd(0x01, sizeof(r301t_send_getimg_cmd), \
					                   r301t_send_getimg_cmd);
					break;

				case 3://第3步，发送genchar2命令，设置步骤为4
					r301t_autoenroll_step = 4;
					fig_recieve_data_length = 0;
					fig_cmd_code = GR_FIG_CMD_GENCHAR;
					fig_r301t_send_cmd(0x01, sizeof(r301t_send_genchar2_cmd), \
					                   r301t_send_genchar2_cmd);
					break;

				case 4://第4步，发送regmodel命令，设置步骤为5
					r301t_autoenroll_step = 5;
					fig_recieve_data_length = 0;
					fig_cmd_code = GR_FIG_CMD_REGMODEL;
					fig_r301t_send_cmd(0x01, sizeof(r301t_send_regmodel_cmd), \
					                   r301t_send_regmodel_cmd);
					break;

				case 5://第5步，发送储storechar命令，完成，设置步骤为0,设置标志位为false
					r301t_autoenroll_step = 6;
					fig_recieve_data_length = 0;
					//设置命令码为storechar
					fig_cmd_code = GR_FIG_CMD_STORECHAR;
					//组织发送存储命令
					memset(r301t_send_storechar_idx_cmd, 0, 4);
					memcpy(r301t_send_storechar_idx_cmd, \
					       r301t_send_storechar_id0_cmd, 4);
					//设置ID号
					r301t_send_storechar_idx_cmd[2] = enroll_fig_id[0];
					r301t_send_storechar_idx_cmd[3] = enroll_fig_id[1];

					fig_r301t_send_cmd(0x01, sizeof(r301t_send_storechar_idx_cmd), \
					                   r301t_send_storechar_idx_cmd);
					break;
				case 6://第6步，判断结果
					//关闭指纹芯片
					close_fig();
					r301t_autoenroll_step = 0;
					fig_recieve_data_length = 0;
					//将命令加上0x40,返回给app
					nus_data_send[0] = ble_operate_code + 0x40;
					//判断结果码
					if(fig_recieve_data[9] ==0x00) {
						nus_data_send[1] = 0x00;
						nus_data_send[2] = enroll_fig_id[0];
						nus_data_send[3] = enroll_fig_id[1];
						nus_data_send_length = 4;
						ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
						//成功将设置指纹信息函数存储在内部flash
restore_fig_info:
						err_code = fig_info_write(&fig_info_set);
						if(err_code != 0) {
							goto restore_fig_info;
						}
						//				nrf_delay_ms(2000);
					} else {
						nus_data_send[1] = 0x01;
						nus_data_send_length = 2;
						ble_nus_string_send(&m_nus, nus_data_send, nus_data_send_length);
					}
					is_r301t_autoenroll = false;
					break;
				default:

					break;
				}
			}
		}
	}

	//收到数据长度清零
	fig_recieve_data_length = 0;
	return 0;
}
