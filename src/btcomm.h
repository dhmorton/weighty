/*
 * btcomm.h
 *
 *  Created on: Jul 26, 2015
 *      Author: bob
 */

#ifndef SRC_BTCOMM_H_
#define SRC_BTCOMM_H_

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

int bt_init();
void send_command_bt(char *, int);


#endif /* SRC_BTCOMM_H_ */
