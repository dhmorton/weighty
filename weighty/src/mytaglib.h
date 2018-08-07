/*
 * mytaglib.h
 *
 *  Created on: Jul 6, 2011
 *      Author: bob
 */

#ifndef SRC_MYTAGLIB_H_
#define SRC_MYTAGLIB_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <taglib/tag_c.h>

int get_generic_tags(const char*, int);
int write_taglib_tag(const char*, char*, char*);
int write_taglib_tags(const char*, int, char**, char**);
#endif /* SRC_MYTAGLIB_H_ */
