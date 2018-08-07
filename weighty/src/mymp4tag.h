/*
 * mymp4tag.h
 *
 *  Created on: Jul 11, 2013
 *      Author: bob
 */

#ifndef SRC_MYMP4TAG_H_
#define SRC_MYMP4TAG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <mp4v2/mp4v2.h>
#include <mp4v2/itmf_tags.h>

int get_mp4_tags(const char*, int);
int check_for_mp4_image(const char*);
int write_mp4_tag(const char*, char*, char*);
int write_mp4_tags(const char*, int, char**, char**);

#endif /* SRC_MYMP4TAG_H_ */
