#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "nordic_common.h"
#include "bsp.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_config.h"

#include "ble_hci.h"
#include "ble_dis.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "pstorage.h"
#include "device_manager.h"
#include "app_trace.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"

#include "ble_init.h"
#include "inter_flash.h"
#include "fm260b.h"
#include "beep.h"
#include "led_button.h"

dm_application_instance_t 				m_app_handle;
dm_handle_t											m_dm_handle;

ble_uuid_t                       	m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}};


ble_nus_t                        	m_nus;//ble 服务注册的nus服务
uint16_t                         		m_conn_handle = BLE_CONN_HANDLE_INVALID;

uint8_t mac[8];//第一位：标志位，第二位：长度
uint8_t device_name[20];//[0]标记位0x77，[1]长度[2...]名字

//自定义的nus服务中data_handle函数中暂存的数据，需要交给check命令
bool    			operate_code_setted = false;
uint8_t			nus_data_array[BLE_NUS_MAX_DATA_LEN];
uint16_t  		nus_data_array_length;

//指纹模块发送给蓝牙芯片的数据
uint8_t						fig_send_data_array[BLE_NUS_MAX_DATA_LEN];
uint16_t						fig_send_data_array_length = 0;

void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**************************************************
*配对超时处理函数
*in：		p_context	超时描述
**************************************************/
static void sec_req_timeout_handler(void * p_context)
{
    uint32_t             err_code;
    dm_security_status_t status;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        err_code = dm_security_status_req(&m_dm_handle, &status);
        APP_ERROR_CHECK(err_code);

        // In case the link is secured by the peer during timeout, the request is not sent.
        if (status == NOT_ENCRYPTED)
        {
            err_code = dm_security_setup_req(&m_dm_handle);
            APP_ERROR_CHECK(err_code);
        }
    }
}

//因为广播函数是在后面定义的，使用的话，先定义
void advertising_init(void);

/**************************************
*重复广播的定时器函数
**************************************/
static void ad_repeat_timeout_handler(void *p_context)
{
	UNUSED_PARAMETER(p_context);
	
	//执行广播
	advertising_init();
}


void timers_init(void)
{
	 uint32_t err_code;

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
	
	 // Create timers.
	// Create Security Request timer.
    err_code = app_timer_create(&m_sec_req_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                sec_req_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
	//重复广播的定时器
	err_code = app_timer_create(&m_ad_repeat_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                ad_repeat_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
}

void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
    uint32_t err_code;
    err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code); */
	uint32_t err_code;
    err_code = app_timer_start(m_ad_repeat_timer_id, AD_REPEAT_DELAY, NULL);
    APP_ERROR_CHECK(err_code);
}

void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
	/*
    for (uint32_t i = 0; i < length; i++)
    {
        while(app_uart_put(p_data[i]) != NRF_SUCCESS);
    }
    while(app_uart_put('\n') != NRF_SUCCESS);
*/
	//将获取的数据存到全局变量，供operate_code_check函数用
	for(int i = 0; i <length; i++)
	{
		nus_data_array[i] = p_data[i];
	}
	nus_data_array_length = length;
	operate_code_setted = true;
	//测试程序，将蓝牙串口的数据再返回给蓝牙串口
//	ble_nus_string_send(&m_nus, nus_data_array, nus_data_array_length);
	
}
/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
void services_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;
    
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;
    
    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
 /*   err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);
*/
    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
//            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
//            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for the application's SoftDevice event handler.
 *
 * @param[in] p_ble_evt SoftDevice event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
 //           err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
 //           APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
  //          err_code = bsp_indication_set(BSP_INDICATE_IDLE);
   //         APP_ERROR_CHECK(err_code);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
			//增加处理
			dm_device_delete_all(&m_app_handle);
            break;

		  case BLE_GAP_EVT_AUTH_STATUS:
			//判断配对是否成功，如果不成功断开连接，从而阻止其他人任意连接
			if(p_ble_evt->evt.gap_evt.params.auth_status.auth_status != BLE_GAP_SEC_STATUS_SUCCESS)
			{
				sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			}
			else
			{
#if defined(BLE_DOOR_DEBUG)
				printf("pair success\r\n");
#endif
			}
			break;
		
/*		
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;
*/
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a SoftDevice event to all modules with a SoftDevice 
 *        event handler.
 *
 * @details This function is called from the SoftDevice event interrupt handler after a 
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  SoftDevice event.
 */

static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
	
		//在断开连接事件后，初始化广播数据
	if(p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED)
	{
		advertising_init();
	}
	
	ble_conn_params_on_ble_evt(p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
	
	dm_ble_evt_handler(p_ble_evt);
	
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);
    
}

static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    ble_advertising_on_sys_evt(sys_evt);
}



/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
    uint32_t err_code;
    
    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);
    
    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);
        
    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);
    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
	
	// Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
static void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            err_code = ble_advertising_restart_without_whitelist();
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to 
 *          a string. The string will be be sent over BLE when the last character received was a 
 *          'new line' i.e '\n' (hex 0x0D) or if the string has reached a length of 
 *          @ref NUS_MAX_DATA_LENGTH.
 */
/**@snippet [Handling the data received over UART] */

static void ble_set_fig_free(void)
{
	static uint8_t fig_cmd_free[8]={0x1B,0xFF,0x00, 0x00,0x00, 0x00,0x00, 0x1A};
	//发送free指令，使模块进入低功耗
	for (uint32_t i = 0; i < sizeof(fig_cmd_free); i++)
	{
		while(app_uart_put(fig_cmd_free[i]) != NRF_SUCCESS);
	}
}

static void uart_event_handle(app_uart_evt_t * p_event)
{
    uint32_t       err_code;
	
	//由于指纹模块是一个自动化的模块，只需将返回结果直接通过蓝牙串口返还给上位机即可

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&fig_send_data_array[fig_send_data_array_length]));
            fig_send_data_array_length++;
			
		//------------- 初始化上电后 --------接收到准备命令
		//返回包长2
			if( (fig_send_data_array_length==2) &&\
					(fig_send_data_array[0]==0x55) && (fig_send_data_array[1]==0x55 ) )
			{
				//设置指纹的状态位ture
				fig_status = true;
				fig_send_data_array_length = 0;
			}
			
			//----------------ERROR CMD   REPLY----------错误的指令包
			//返回包长5
			if( (fig_send_data_array_length==5) &&\
						( 	(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == FIG_RESULT_COM_WRONG) ||\
							(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == FIG_RESULT_PARA_WRONG) ||\
							(fig_send_data_array[FIG_R_RESULT_CODE_SITE] == FIG_RESULT_SUM_WRONG)  )  )
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
			
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
void uart_init(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud57600
    };

    APP_UART_FIFO_INIT( &comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOW,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */


/**@brief Function for initializing the Advertising functionality.
 */
void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS,
                                 APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), 
                                 bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for placing the application in low power state while waiting for events.
 */
void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/*****************************************************
*DM处理函数
******************************************************/
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    uint32_t err_code;

    APP_ERROR_CHECK(event_result);

    switch (p_event->event_id)
    {
        case DM_EVT_CONNECTION:
			m_dm_handle = (*p_handle);
            // Start Security Request timer.
    //        if (m_dm_handle.device_id != DM_INVALID_ID)
            {
				//在有蓝牙设备请求连接的时候，开启安全请求的timer
                err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
                APP_ERROR_CHECK(err_code);
            }
            break;
		case DM_EVT_DISCONNECTION:
		//	dm_device_delete_all(&m_app_handle);
			break;
		case DM_EVT_SECURITY_SETUP:
			
			break;
		case DM_EVT_SECURITY_SETUP_COMPLETE:
			
			break;
		case DM_EVT_SERVICE_CONTEXT_DELETED:
		//	if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
		//	{
		//		sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		//	}
			break;
		case DM_EVT_LINK_SECURED:
			break;
		
        default:
            break;
    }

#ifdef BLE_DFU_APP_SUPPORT
	if(p_event->event_id == DM_EVT_LINK_SECURED)
	{
		app_context_load(p_handle);
	}
#endif	//BLE_DFU_APP_SUPPORT
    return NRF_SUCCESS;
}


void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));
    
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}
