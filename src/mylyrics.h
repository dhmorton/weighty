/*
 * mylyrics.h
 *
 *  Created on: Jul 28, 2011
 *      Author: bob
 */

#ifndef SRC_MYLYRICS_H_
#define SRC_MYLYRICS_H_

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <stdsoap2.h>

int get_lyrics(char*, char*);
void parse_lyrics();

#endif /* SRC_MYLYRICS_H_ */
