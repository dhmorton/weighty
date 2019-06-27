/*
 * myutils.h
 *
 *  Created on: Aug 12, 2008
 *      Author: bob
 */

#ifndef SRC_MYUTILS_H_
#define SRC_MYUTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>  //for file open/close
#include <unistd.h>
#include <math.h>

extern char* homedir;
extern char* configfile;
extern char* alarmconfig;
extern char* sleepconfig;
extern char* streamconfig;
extern char* database;
extern char* errorlog;
extern char* discogs;
extern struct config val;

struct config {
	char musicdir[255];
	int threshhold;
	char type[16];
	char playby[16];
	int song_rand;
	int song_skip;
	int album_rand;
	int album_skip;
	int artist_rand;
	int artist_skip;
	int genre_rand;
	int genre_skip;
	int var;
};
typedef struct {
	char *file; //full path to song
	int weight;
	int sticky;
} song;

void init_files(void);
void itoa(int, char*);
int check_file(const char*);
void translate_field(char*, char*);
void back_translate(char*, char**);
void print_data(char*, int);
int error(char *);
int read_config(void);
void write_config(void);
//math functions
float linear(int, int, int);
float exponential(int, int, int);
float gaussian(int, int, int);
float flat(int, int, int);
float get_constant(int);
int should_play(int);
void send_discogs_key(void);

#endif /* SRC_MYUTILS_H_ */
