#ifndef BATTERY_H__
#define BATTERY_H__

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

#define SAMPLES_IN_BUFFER 5

extern nrf_saadc_value_t	battery_level_buffer[2][SAMPLES_IN_BUFFER];
extern uint16_t battery_level_value;

void saadc_sampling_event_init(void);
void saadc_sampling_event_enable(void);
void saadc_init(void);
void battery_level_init(void);

#endif //BATTERY_H__
