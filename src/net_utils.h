/*
 * net_utils.h
 *
 *  Created on: Dec 29, 2011
 *      Author: bob
 */

#ifndef SRC_NET_UTILS_H_
#define SRC_NET_UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "myutils.h"//for the struct song

#define PORT 23529
#define BUFF_SIZE 65536
#define BUFF 65536

extern struct counts stats;

struct counts {
	int mp3;
	int ogg;
	int mpc;
	int wav;
	int flac;
	int m4a;
	int wma;
	int other;
	int zero;
	int hund;
	int sticky;
	int count[101];
	int total;
	int albums;
	int artists;
	int genres;
};

void update_socks(int);
void clear_sock(int);
void send_update(char, char*);
int send_weight_sticky_data(int, int);
int send_playing(char*);
int send_song_data(song*, int, char);
void send_stats(void);
void send_command(char*, int);

#endif /* SRC_NET_UTILS_H_ */
