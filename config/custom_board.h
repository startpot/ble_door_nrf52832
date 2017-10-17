#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

/*******************************************
-----13 LEDS---------
LED			nRF52832
1			p0.1
2          	p0.19
3		 	p0.28
4			p0.8
5			p0.25
6			p0.2
7			p0.20
8			p0.12
9			p0.11
10			p0.6
11			p0.31
12			p0.7
13			p0.3

--------BEEP----------
beep_in		nRF52832
2			p0.13

--------touch button  TSM12----------
touch ic			nRF52832
TOUCH_IIC_EN_PIN		29
TOUCH_IIC_INT_PIN		30
TOUCH_IIC_SCL_PIN		27
TOUCH_IIC_SDA_PIN		26

---------rtc ic pcf85163t---------
rtc ic			nRF52832
RTC_IIC_SCL_PIN		27
RTC_IIC_SDA_PIN		26

---------moto ic hg7881c----------
moto ic			nRF52832
MOTO_FI				15
MOTO_BI				14

----------uart figprint ic r301t------------
控制小板		nRF52832	指纹模块r301t
J4-1(vt 3.3v)	---			6(touch_power)
J4-2(vcc 5v)	---			1(vcc 5v)
J4-3(rxd)		18			3(txd)
J4-4(txd)		17			4(rxd)
J4-5(gnd)		---			2(gnd)
J4-6(touch_in)	16			5(touch_in)

********************************************/
//LEDs
#define	LED_1        	23
#define	LED_2          	19
#define	LED_3		 	28
#define	LED_4			8
#define	LED_5			25
#define	LED_6			2
#define	LED_7			20
#define	LED_8			12
#define	LED_9			11
#define	LED_10			6
#define	LED_11			31
#define	LED_12			7
#define	LED_13			3

#define BSP_LED_0      LED_1
#define BSP_LED_1      LED_2
#define BSP_LED_2      LED_3
#define BSP_LED_3      LED_4
#define BSP_LED_4      LED_5
#define BSP_LED_5      LED_6
#define BSP_LED_6      LED_7
#define BSP_LED_7      LED_8
#define BSP_LED_8      LED_9
#define BSP_LED_9      LED_10
#define BSP_LED_10     LED_11
#define BSP_LED_11     LED_12
#define BSP_LED_12     LED_13

#define LEDS_NUMBER   13

#define LEDS_LIST { LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, \
					LED_8, LED_9, LED_10, LED_11, LED_12, LED_13}

#define BSP_LED_0_MASK    (1<<LED_1)
#define BSP_LED_1_MASK    (1<<LED_2)
#define BSP_LED_2_MASK    (1<<LED_3)
#define BSP_LED_3_MASK    (1<<LED_4)
#define BSP_LED_4_MASK    (1<<LED_5)
#define BSP_LED_5_MASK    (1<<LED_6)
#define BSP_LED_6_MASK    (1<<LED_7)
#define BSP_LED_7_MASK    (1<<LED_8)
#define BSP_LED_8_MASK    (1<<LED_9)
#define BSP_LED_9_MASK    (1<<LED_10)
#define BSP_LED_10_MASK   (1<<LED_11)
#define BSP_LED_11_MASK   (1<<LED_12)
#define BSP_LED_12_MASK   (1<<LED_13)

#define LEDS_MASK      (BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK |	\
						BSP_LED_3_MASK | BSP_LED_4_MASK | BSP_LED_5_MASK | \
						BSP_LED_6_MASK | BSP_LED_7_MASK | BSP_LED_8_MASK | \
						BSP_LED_9_MASK | BSP_LED_10_MASK | BSP_LED_11_MASK | \
						BSP_LED_12_MASK )
												
/* all LEDs are lit when GPIO is low */
#define LEDS_INV_MASK  LEDS_MASK


//beep
#define BEEP_IN_PIN		13

//battery level en
#define BATTERY_LEVEL_EN	5
#define BATTERY_LEVEL_IN	4


//uart
#define RX_PIN_NUMBER		18
#define TX_PIN_NUMBER		17
#define CTS_PIN_NUMBER		21
#define RTS_PIN_NUMBER		24
#define HWFC				false
#define FIG_WAKE_N_PIN		16


//TOUCH-IIC
#define	TOUCH_IIC_EN_PIN		29
#define	TOUCH_IIC_INT_PIN		30
#define TOUCH_IIC_SCL_PIN		27
#define	TOUCH_IIC_SDA_PIN		26


//RTC-IIC
#define	RTC_IIC_SCL_PIN		27
#define	RTC_IIC_SDA_PIN		26


//MOTO
#define MOTO_FI				15
#define MOTO_BI				14

//nRST重置按键
#define	NRST_IN				22


// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC      NRF_CLOCK_LFCLKSRC_XTAL_20_PPM

#endif
