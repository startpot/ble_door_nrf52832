#ifndef  RTC_CHIP_H__
#define  RTC_CHIP_H__

#include <stdint.h>
#include <time.h>
#include "sdk_errors.h"

/*************************RTC-CHIP的地址****************************/
#define RTC_CHIP_DEVICE_ADDR			0xA2	
#define RTC_CHIP_REAL_ADDR 				(RTC_CHIP_DEVICE_ADDR>>1)

/*************************IIC-pins*********************************/
#define RTC_CHIP_IIC_SCL_PIN				RTC_IIC_SCL_PIN
#define RTC_CHIP_IIC_SDA_PIN				RTC_IIC_SDA_PIN

/**************RTC PF85163 REGISTER MAP****************/
#define PCF85163_Control_1_ADDR					0x00
#define	PCF85163_Control_2_ADDR					0x01
#define	PCF85163_VL_seconds_ADDR				0x02
#define	PCF85163_Minutes_ADDR					0x03
#define	PCF85163_Hours_ADDR						0x04
#define	PCF85163_Days_ADDR						0x05
#define	PCF85163_Weekdays_ADDR					0x06
#define	PCF85163_Century_months_ADDR			0x07
#define	PCF85163_Years_ADDR						0x08
#define	PCF85163_Minute_alarm_ADDR				0x09
#define	PCF85163_Hour_alarm_ADDR				0x0a
#define	PCF85163_Day_alarm_ADDR					0x0b
#define	PCF85163_Weekday_alarm_ADDR				0x0c
#define	PCF85163_CLKOUT_control_ADDR			0x0d
#define	PCF85163_Timer_control_ADDR				0x0e
#define	PCF85163_Timer_ADDR						0x0f


void rtc_init(void);
uint8_t	rtc_time_write(struct tm *time_write);
uint8_t rtc_time_read(struct tm *time_read);

#endif  //RTC_CHIP_H__
