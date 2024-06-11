/*
 * myflactag.h
 *
 *  Created on: Jul 6, 2011
 *      Author: bob
 */

#ifndef SRC_MYFLACTAG_H_
#define SRC_MYFLACTAG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <FLAC/metadata.h>

int get_flac_tags(const char*, int);
int check_for_flac_image(const char*);
int write_flac_tags(char*, int, char**, char**);
#endif /* SRC_MYFLACTAG_H_ */
