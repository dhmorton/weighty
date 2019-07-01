/*
 * playnow_list.h
 *
 *  Created on: Dec 26, 2016
 *      Author: bob
 */

#ifndef SRC_PLAYNOW_LIST_H_
#define SRC_PLAYNOW_LIST_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void playnow_list_init(void);
void add_current_album_to_playlist(void);
void add_field_to_playlist(char*, char*);
void push_to_playnow_list(char*, int, int);
int get_playnow_left(void);
int set_cursong_from_playnow(void);
void clear_playnow_lists(void);

#endif /* SRC_PLAYNOW_LIST_H_ */
