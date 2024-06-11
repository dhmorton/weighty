/*
 * three_lists.h
 *
 *  Created on: Dec 25, 2016
 *      Author: bob
 */

#ifndef SRC_THREE_LISTS_H_
#define SRC_THREE_LISTS_H_
#include <stdlib.h>
#include <string.h>

#include "myutils.h"//for the struct song

void three_lists_init(void);

void add_to_history(char*);
void add_to_skipped(char*);
int get_songs_in_history(song**);
void update_history(void);
void clear_history_files(void);
void add_to_album_history(char*);
void add_to_artist_history(char*);
void add_to_genre_history(char*);
void filter_from_history(void);
void filter_from_album_history(void);
void filter_from_artist_history(void);
void filter_from_genre_history(void);

void make_push_song_node(char*, int, int);
void generate_playlist(void);
void generate_playlist_from_list(void);
void generate_playlist_from_albumlist(void);
void generate_playlist_from_artistlist(void);
void generate_playlist_from_genrelist(void);

void push_cursong_to_back(void);
void set_cursong_from_back(void);
int get_backcount(void);

#endif /* SRC_THREE_LISTS_H_ */
