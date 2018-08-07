/*
 * alarm.h
 *
 *  Created on: Dec 29, 2011
 *      Author: bob
 */

#ifndef SRC_ALARM_H_
#define SRC_ALARM_H_

#include <time.h>

struct alarm_time {
	int set[7];
	int hr[7];
	int min[7];
	int start_vol;
	int end_vol;
	int fade;
	char *file;//song to play
};
void check_alarm(void);
void set_alarm(int, int, int, int);
void set_alarm_settings(int , int , int , char*);
int read_alarm_config(void);

#endif /* SRC_ALARM_H_ */
