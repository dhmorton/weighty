/*
 * songhash.h
 *
 *  Created on: Jul 4, 2011
 *      Author: bob
 */

#ifndef SRC_SONGHASH_H_
#define SRC_SONGHASH_H_

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define HASH_SIZE 10000
#define SMALL_HASH 1000
#define CHAIN_LENGTH 25

typedef struct {
	char *file; //full path to song
	int weight;
	int sticky;
	int flag;//0 = unseen, 1 = seen, 2 = new
} hash_element;

typedef struct {
	char *path;
	char *file;
} added_element;


int add_to_hash(const char*, int, int, int);
void change_weight_hash(const char*,int);
void change_sticky_hash(const char*,int);
int add_to_history_hash(const char*);
void clear_history(void);
int check_song_in_history(const char*);
void init_added(void);
int check_song(const char*);
int retrieve_by_song(const char*, int*);
int check_all_songs_in_hash(void);

#endif /* SRC_SONGHASH_H_ */
