/*
 * current_song.h
 *
 *  Created on: Dec 23, 2016
 *      Author: bob
 */

#ifndef SRC_CURRENT_SONG_H_
#define SRC_CURRENT_SONG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char* cursong;
extern int weight, sticky;

void set_cursong(char*);
void set_cursong_sticky(int);
void free_cursong(void);
void update_weight(int);
int update_image(void);

#endif /* SRC_CURRENT_SONG_H_ */
