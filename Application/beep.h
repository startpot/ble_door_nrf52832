#ifndef	BEEP_H__
#define	BEEP_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#define	BEEP_OPEN			50		//蜂鸣器打开时间(ms)
#define	BEEP_DELAY			100			//每次蜂鸣器响完等待的时间

void beep_init(void);
void beep_didi(uint8_t number);


#endif	//BEEP_H__
