/*
 * queue.h
 *
 *  Created on: Jan 5, 2017
 *      Author: bob
 */

#ifndef SRC_QUEUE_H_
#define SRC_QUEUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void queue_init(void);
void enqueue(char*);
void get_queue(void);
void clear_queue(void);
void clear_queue_by_index(int);
void highlight_playlist(void);
void send_remaining(void);
void set_cursong_from_qpointer(void);
int get_qcount(void);

#endif /* SRC_QUEUE_H_ */
