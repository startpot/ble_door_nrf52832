#ifndef MOTO_H__
#define MOTO_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

void moto_init(void);

void moto_open(uint32_t open_time);
void moto_close(uint32_t close_time);

#endif //MOTO_H__
