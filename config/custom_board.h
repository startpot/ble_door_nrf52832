#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

/************************************************************
*----------------------custom_board.h-------------------------
*	板子的主要引脚分布 io置1为亮
*-------------------------------------------------------------
*	按键键位LEDx：
*	LED1		P.18			LED5		P.17			LED9		P.22
*	LED2		P.15			LED6		P.7				LED10		P.6
*	LED3		P.14			LED7		P.8				LED11		P.9
*	LED4		P.13			LED8		P.12			LED13		P.10
*																			LED12		P.11
*																(LED13(绿)LED12(蓝)对应1个键位)
*--------------------------------------------------------------
*	电机驱动芯片()
*	6(bi--backward input)			P.3
*	7(fi--forward input)			P.2
*--------------------------------------------------------------
*	触摸屏(iic)
*	4(iic_en)			P.25
*	3(int)				P.24
*	2(scl)				P.0
*	1(sda)				p.1
*---------------------------------------------------------------
*	蜂鸣器()
*	1(驱动脚)			P.4
*---------------------------------------------------------------
*	UART(JK5)
*	3(TX)					P.29
*	4(RX)					P.28
*************************************************************/


//LEDs
#define	LED_1        	23
#define 	LED_2          	19
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

//beep
#define BEEP_IN_PIN		13


#define LEDS_NUMBER   13


//#define BSP_BUTTON_0_MASK (1<<BSP_BUTTON_0)

#define LEDS_LIST { LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8, \
										LED_9, LED_10, LED_11, LED_12, LED_13}


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


#define RX_PIN_NUMBER		18
#define TX_PIN_NUMBER  	17
#define CTS_PIN_NUMBER 	21
#define RTS_PIN_NUMBER 	24
#define HWFC           false
#define FIG_WAKE_N_PIN	16



//TOUCH-IIC

#define	TOUCH_IIC_EN_PIN		29
#define	TOUCH_IIC_INT_PIN		30
#define 	TOUCH_IIC_SCL_PIN		27
#define	TOUCH_IIC_SDA_PIN	26

//RTC-IIC
#define	RTC_IIC_SCL_PIN		27
#define	RTC_IIC_SDA_PIN		26

//MOTO
#define MOTO_FI    			15
#define MOTO_BI				14

// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC      NRF_CLOCK_LFCLKSRC_XTAL_20_PPM

#endif
