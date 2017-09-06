#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "my_time.h"


time_t my_mktime(struct tm *time)
{
	time->tm_mon =time->tm_mon +1;
	if((int)(time->tm_mon  -= 2)<=0)
	{//1....12->11,12,1,....10
		time->tm_mon +=12;
		time->tm_year -=1;
	}
	
	return ( ( ( 
				(unsigned long) ((time->tm_year +1990)/4 - (time->tm_year+1990)/100 + (time->tm_year+1990)/400 + 367*time->tm_mon/12 + time->tm_mday) +\
								(time->tm_year +1990)*365 - 719499)*24 + time->tm_hour /**//* now have hours */
			 )*60 + time->tm_min /**//* now have minutes */
			)*60 + time->tm_sec - 8*3600; /**//* finally seconds */

}

double my_difftime(time_t time1,time_t time2)
{
	return (double)(time1 - time2);
	
}
