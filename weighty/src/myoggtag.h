/*
 * myoggtag.h
 *
 *  Created on: Jul 5, 2011
 *      Author: bob
 */

#ifndef SRC_MYOGGTAG_H_
#define SRC_MYOGGTAG_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


int get_ogg_tags(const char*, int);
int write_ogg_tags(char*, int, char**, char**);
#endif /* SRC_MYOGGTAG_H_ */
