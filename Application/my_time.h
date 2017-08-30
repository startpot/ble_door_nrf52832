#ifndef MY_TIME_H__
#define MY_TIME_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

time_t my_mktime(struct tm *time);
double my_difftime(time_t time1,time_t time2);

#endif //MY_TIME_H__
