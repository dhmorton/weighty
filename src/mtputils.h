/*
 * mtputils.h
 *
 *  Created on: Jun 19, 2016
 *      Author: bob
 */

#ifndef SRC_MTPUTILS_H_
#define SRC_MTPUTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <libmtp.h>
#include <string.h>
#include <libgen.h>
#include <pthread.h>

#define TAG_SIZE 2048

int phone_thread_init(char[], int, int, int, char[], int, char[]);

#endif /* SRC_MTPUTILS_H_ */
