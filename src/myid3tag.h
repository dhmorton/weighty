/*
 * myid3tag.h
 *
 *  Created on: Jul 5, 2011
 *      Author: bob
 */

#ifndef SRC_MYID3TAG_H_
#define SRC_MYID3TAG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <id3tag.h>

int get_mp3_tags(const char*, int);
int check_for_mp3_image(const char *);
int write_mp3_tag(const char*, char*, char*);
int write_mp3_tags(char*, int, char**, char**);
int clear_mp3_tag(const char*, char*);

#endif /* SRC_MYID3TAG_H_ */
