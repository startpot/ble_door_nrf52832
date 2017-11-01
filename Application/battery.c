#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_delay.h"
#include "boards.h"

#include "app_error.h"
#include "app_util_platform.h"

#include "battery.h"
#include "set_params.h"

volatile uint8_t state = 1;
static const nrf_drv_timer_t  m_timer = NRF_DRV_TIMER_INSTANCE(3); //因为使用了协议栈，不能使用定时器0
static nrf_ppi_channel_t m_ppi_channel;
static uint32_t m_adc_evt_counter;

nrf_saadc_value_t	battery_level_buffer[2][SAMPLES_IN_BUFFER];
uint16_t battery_level_value;

static void timer_handler(nrf_timer_event_t event_type, void* p_context) {

}

void saadc_sampling_event_init(void) {
	ret_code_t err_code;

	err_code = nrf_drv_ppi_init();
	APP_ERROR_CHECK(err_code);


	//初始化定时器
	err_code = nrf_drv_timer_init(&m_timer,NULL, timer_handler);
	APP_ERROR_CHECK(err_code);

	//setup m_timer for compare event every 4ms
	uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer, 4);
	nrf_drv_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, \
	                               ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
	nrf_drv_timer_enable(&m_timer);

	uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer, NRF_TIMER_CC_CHANNEL0);
	uint32_t saadc_sample_event_addr = nrf_drv_saadc_task_address_get(NRF_SAADC_TASK_SAMPLE);

	//setup ppi channel so that timer compare event is triggering sample task in SAADC
	err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_channel_assign(m_ppi_channel, timer_compare_event_addr, saadc_sample_event_addr);
	APP_ERROR_CHECK(err_code);

}

void saadc_sampling_event_enable(void) {
	ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);
	APP_ERROR_CHECK(err_code);

}

static void saadc_callback(nrf_drv_saadc_evt_t const * p_event) {

	if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {
		ret_code_t err_code;

		err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
		APP_ERROR_CHECK(err_code);

		//	int i;
		/*   printf("ADC event number: %d\r\n",(int)m_adc_evt_counter);
		 for (i = 0; i < SAMPLES_IN_BUFFER; i++)
		 {
		     printf("%d\r\n", p_event->data.done.p_buffer[i]);
		 }
		 m_adc_evt_counter++;
		*/
		battery_level_value = battery_level_buffer[0][0];

		if(battery_level_value <= (uint16_t)(VOL_VALUE)<<4) { //0x02c0 大致为5.0V
			//亮起*灯
			//	nrf_gpio_pin_clear(LED_8);
		} else {
			//否则灭*灯
			//	nrf_gpio_pin_set(LED_8);
		}
	}

}

void saadc_init(void) {
	ret_code_t err_code;

	nrf_saadc_channel_config_t channel_config =
	    NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);//电压输入到AIN2
	err_code = nrf_drv_saadc_init(NULL, saadc_callback);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_channel_init(0, &channel_config);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(battery_level_buffer[0],SAMPLES_IN_BUFFER);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(battery_level_buffer[1],SAMPLES_IN_BUFFER);
	APP_ERROR_CHECK(err_code);

}

static void battery_level_en(void) {
	//设置引脚为输出，置低
	nrf_gpio_cfg_output( BATTERY_LEVEL_EN );
	nrf_gpio_pin_clear( BATTERY_LEVEL_EN );
//	nrf_gpio_pin_set( BATTERY_LEVEL_EN );

	//设置电压测量引脚为输入
	nrf_gpio_cfg_input(BATTERY_LEVEL_IN, NRF_GPIO_PIN_NOPULL);

}



void battery_level_init(void) {
	//使能电池电压输出端
	battery_level_en();
	//初始化saadc sample event
	saadc_sampling_event_init();
	//初始化saadc
	saadc_init();
	//使能saadc sample event
//	saadc_sampling_event_enable();

}


