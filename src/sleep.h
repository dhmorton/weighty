/*
 * sleep.h
 *
 *  Created on: Dec 29, 2011
 *      Author: bob
 */

#ifndef SRC_SLEEP_H_
#define SRC_SLEEP_H_

#include <stdio.h>
#include <time.h>


struct sleep_time {
	int play;//minutes to play
	int fade;//minutes to fade
};

int check_sleep(void);
void set_sleep(int, int);
void stop_sleep(void);
void start_sleep(void);
int read_sleep_config(void);

#endif /* SRC_SLEEP_H_ */
