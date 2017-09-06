#ifndef R301T_H__
#define R301T_H__

/**********************************************************
*r301t是一个指纹模块，容量0x0bb8  (3000)
*   pin1------5v
*   pin2------GND
*   pin3------TXD
*   pin4------RXD
*   pin5------TOUCH
*   pin6------Touch Power
*   UART波特率(N*9600) N默认是6，即57600bps
*
*	命令包格式
*	包头(2bytes,0xEF01)--模块地址(4bytes,0xFFFFFFFF)--包标识(1byte,0x01)--包长度(2bytes)--指令(1byte)--参数(Nbytes)--校验和(2bytes)
*
*	数据包格式
*	包头(2bytes,0xEF01)--模块地址(4bytes,0xFFFFFFFF)--包标识(1byte,0x02)---包长度(2bytes)--数据(Nbytes)---校验和(2bytes)
*
*	结束包格式
*	包头(2bytes,0xEF01)--模块地址(4bytes,0xFFFFFFFF)--包标识(1byte,0x08)---包长度(2bytes)--数据(Nbytes)---校验和(2bytes)
*
*	应答包格式
*	包头(2bytes,0xEF01)--模块地址(4bytes,0xFFFFFFFF)--包标识(1byte,0x07)---包长度(2bytes)--确认码(1byte)--数据(Nbytes)---校验和(2bytes)
*
************************************************************/

//状态寄存器	GR:杭州城章
#define	GR_SYS_PARAM_BUSY_BIT       	0   //1表示系统正在执行命令，0表示空闲
#define	GR_SYS_PARAM_PASS_BIT       	1   //1表示指纹验证通过
#define	GR_SYS_PARAM_PWD_BIT         	2   //1表示设备握手口令通过验证。
#define	GR_SYS_PARAM_IBS_BIT           	3   //IBS ImgBufStat    1表示指纹图像缓冲区存在有效指纹图像

#define	GR_FIG_HEADER_SITE				0
#define	GR_FIG_HEADER					0xEF01	//包头
#define	GR_FIG_ADDR_SITE				2
#define	GR_FIG_ADDR						0xFFFFFFFF	//模块地址

#define	GR_FIG_DATA_ID_SITE				6
#define	GR_FIG_DATA_ID_CMD				0x01	//命令包标识
#define	GR_FIG_DATA_ID_DATA				0x02	//数据包标识
#define	GR_FIG_DATA_ID_END				0x08	//结束包标识
//应答是将有关命令执行情况与结果上报给上位机，应答包含有参数，
//并可跟后续数据包。上位机只有在收到模块的应答包后才能确认模块
//收包情况与指令执行情况
#define	GR_FIG_DATA_ID_REPLY			0x07	//应答包标识

#define	GR_FIG_REPLY_ACK_OK				0x00	//应答包中确认码,表示指令执行完毕或OK
#define	GR_FIG_REPLY_ACK_ER1			0x01	//应答包中确认码,表示数据包接收错误
#define	GR_FIG_REPLY_ACK_ER2			0x02	//应答包中确认码,表示传感器上没手指
#define	GR_FIG_REPLY_ACK_ER3			0x03	//应答包中确认码,表示录入指纹失败(IFPFAIL input fingerprint fail)
#define	GR_FIG_REPLY_ACK_ER4			0x04	//应答包中确认码,表示指纹图像太干、太淡而生不成特征
#define	GR_FIG_REPLY_ACK_ER5			0x05	//应答包中确认码,表示指纹图像太湿、太糊而生不成特征
#define	GR_FIG_REPLY_ACK_ER6			0x06	//应答包中确认码,表示指纹图像太乱而生不成特征
#define	GR_FIG_REPLY_ACK_ER7			0x07	//应答包中确认码,表示指纹图像正常，但是特征点太少(或面积太小)而生不成特征
#define	GR_FIG_REPLY_ACK_ER8			0x08	//应答包中确认码,表示指纹不匹配
#define	GR_FIG_REPLY_ACK_ER9			0x09	//应答包中确认码,没有搜索到指纹
#define	GR_FIG_REPLY_ACK_ER10			0x0A	//应答包中确认码,表示特征合并失败
#define	GR_FIG_REPLY_ACK_ER11			0x0B	//应答包中确认码,表示访问指纹库时地址序号超出指纹库范围
#define	GR_FIG_REPLY_ACK_ER12			0x0C	//应答包中确认码,表示从指纹库读模板出错或者无效
#define	GR_FIG_REPLY_ACK_ER13			0x0D	//应答包中确认码,表示上传特征失败
#define	GR_FIG_REPLY_ACK_ER14			0x0E	//应答包中确认码,表示模块不能接受后续数据包
#define	GR_FIG_REPLY_ACK_ER15			0x0F	//应答包中确认码,表示上传图像失败
#define	GR_FIG_REPLY_ACK_ER16			0x10	//应答包中确认码,表示删除模板失败
#define	GR_FIG_REPLY_ACK_ER17			0x11	//应答包中确认码,表示清空指纹库失败
#define	GR_FIG_REPLY_ACK_ER18			0x12	//应答包中确认码,表示口令不正确

#define	GR_FIG_REPLY_ACK_ER21			0x15	//应答包中确认码,表示缓冲区内没有有效原始图而生不成图像
#define	GR_FIG_REPLY_ACK_ER24			0x18	//应答包中确认码,表示读写FLASH 出错
#define	GR_FIG_REPLY_ACK_ER25			0x19	//应答包中确认码,表示未定义错误
#define	GR_FIG_REPLY_ACK_ER26			0x1A	//应答包中确认码,表示无效寄存器号
#define	GR_FIG_REPLY_ACK_ER27			0x1B	//应答包中确认码,表示寄存器设定内容错误号
#define	GR_FIG_REPLY_ACK_ER28			0x1C	//应答包中确认码,表示记事本页码指定错误
#define	GR_FIG_REPLY_ACK_ER29			0x1D	//应答包中确认码,表示端口操作失败
#define	GR_FIG_REPLY_ACK_ER30			0x1E	//应答包中确认码,表示自动注册（enroll）失败
#define	GR_FIG_REPLY_ACK_ER31			0x1F	//应答包中确认码,表示指纹库满
//20-efH：Reserved

#define	GR_FIG_DATA_LEN_SITE			7	//包长度，占2bytes


//系统默认口令为0，若默认口令未被修改，则系统不要求验证口令，上位机可以直接与芯片通讯；
//若口令被修改，则上位机与芯片通讯的第一个指令必须是验证口令，只有口令验证通过后，芯片才接收其它指令。

#define	GR_FIG_CMD_SITE					9	//命令的位置

#define	GR_FIG_CMD_GETIMG				0x01	//从传感器上读入图像存于图像缓冲区
#define	GR_FIG_CMD_GENCHAR				0x02	//根据原始图像生成指纹特征存于CharBuffer1 或CharBuffer2
#define	GR_FIG_CMD_MATCH				0x03	//精确比对CharBuffer1 与CharBuffer2 中的特征文件
#define	GR_FIG_CMD_SEARCH				0x04	//以CharBuffer1 或CharBuffer2 中的特征文件搜索整个或部分指纹库
#define	GR_FIG_CMD_REGMODEL				0x05	//将CharBuffer1 与CharBuffer2 中的特征文件合并生成模板存于CharBuffer1与CharBuffer2
#define	GR_FIG_CMD_STORECHAR			0x06	//将特征缓冲区中的文件储存到flash 指纹库中
#define	GR_FIG_CMD_LOADCHAR				0x07	//从flash 指纹库中读取一个模板到特征缓冲区
#define	GR_FIG_CMD_UPCHAR				0x08	//将特征缓冲区中的文件上传给上位机
#define	GR_FIG_CMD_DOWNCHAR				0x09	//从上位机下载一个特征文件到特征缓冲区
#define	GR_FIG_CMD_UPIMG				0x0A	//上传原始图像
#define	GR_FIG_CMD_DOWNIMG				0x0B	//下载原始图像
#define	GR_FIG_CMD_DELCHAR				0x0C	//删除flash 指纹库中的一个特征文件
#define	GR_FIG_CMD_EMPTY				0x0D	//清空flash 指纹库
#define	GR_FIG_CMD_WRREG				0x0E	//设置系统参数
#define	GR_FIG_CMD_RDSYSPARA			0x0F	//读系统基本参数
#define	GR_FIG_CMD_SETPWD				0x12	//设置设备握手口令
#define	GR_FIG_CMD_VFYPWD				0x13	//验证设备握手口令
#define	GR_FIG_CMD_GETRDC				0x14	//采样随机数
#define	GR_FIG_CMD_SETADDR				0x15	//设置模块地址
#define	GR_FIG_CMD_POETCTRL				0x17	//通讯端口（UART/USB）开关控制
#define	GR_FIG_CMD_WRNOTPAD				0x18	//写记事本
#define	GR_FIG_CMD_RDNOTPAD				0x19	//读记事本
#define	GR_FIG_CMD_GENBINIMG			0x1C	//生成二值化指纹图像
#define	GR_FIG_CMD_VTNUM				0x1D	//读有效模板个数
#define	GR_FIG_CMD_RDINDEXTB			0x1F	//读索引表
#define	GR_FIG_CMD_AUTOENROLL			0xFF	//设置自动注册，add by ln


extern bool		is_r301t_autoenroll;
extern uint8_t 	r301t_autosearch_step;

void fig_r301t_send_getimage(void);
void fig_r301t_send_cmd(uint8_t	data_id, uint16_t data_len, uint8_t	*data_code);
void fig_r301t_reply_check(void);

#endif   //R301T_H__
