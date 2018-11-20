/*
 * weighty.h
 *
 *  Created on: Aug 6, 2008
 *      Author: bob
 */

#ifndef SRC_WEIGHTY_H_
#define SRC_WEIGHTY_H_

#define _GNU_SOURCE
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

extern char* errorlog;

int daemonize(void);

#endif /* SRC_WEIGHTY_H_ */
