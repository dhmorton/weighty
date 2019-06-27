/*
 * mp3play.h
 *
 *  Created on: Nov 12, 2018
 *      Author: bob
 */

#ifndef SRC_MP3PLAY_H_
#define SRC_MP3PLAY_H_

#include <ao/ao.h>
#include <mpg123.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

int mp3play(char*);
int mp3_finished(void);
void mp3stop(void);
float mp3_progress(void);

#endif /* SRC_MP3PLAY_H_ */
