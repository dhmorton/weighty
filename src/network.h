/*
 * network.h
 *
 *  Created on: Aug 8, 2008
 *      Author: bob
 */

#ifndef SRC_NETWORK_H_
#define SRC_NETWORK_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//Number of microseconds to sleep for each cycle
#define TICK 300000

//socket and data commands
/*
 * The main program loop.
 * It takes a socket number as a parameter and selects() on that socket.
 * Then it checks if the alarm has gone off or if it's in sleep mode.
 * The loop usleeps for TICK microseconds then updates the progress bar.
 *
 */
void weighty(int);
int create_new_socket(void);

//for btcomm.c
void net_play(void);

#endif /* SRC_NETWORK_H_ */
