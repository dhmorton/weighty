/*
 * sqlite.h
 *
 *  Created on: Aug 6, 2008
 *      Author: bob
 */

#ifndef SRC_SQLITE_H_
#define SRC_SQLITE_H_

#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "list_utils.h"//for the struct gen_node
#include "myutils.h"//for the struct song

typedef struct tag_data {
	char *field;
	char *tag;
} tag_data;

int create_database(void);
int connect_to_database(int);
void begin_transaction(void);
void end_transaction(void);
int close_database(int);
//set methods
int add_new_file(const char*);
int update_tag_data(const char*, char*, char*);
int delete_tag_data(const char*, char*);
int set_song_weight(char*, int);
int set_album_weight(char*, int);
int set_artist_weight(char*, int);
int set_genre_weight(char*, int);
int set_song_sticky(char*, int);
int set_album_sticky(char*, int);
int set_artist_sticky(char*, int);
int set_genre_sticky(char*, int);
int set_tag_data(char*, char*, unsigned int);
int delete_song_entry(const char*);
int update_song_entry(const char*, const char*);
//get methods
int get_multisong_albums(song**);
int get_msalbums_for_ll(struct gen_node**);
int get_albums(song**);
int get_artists_for_ll(struct gen_node**);
int get_artists(song**);
int get_genres_for_ll(struct gen_node**);
int get_genres(song**);
int get_songs_by_field(song**, char*, char*);
int get_songs_and_tags_by_field(song**, char*, char*);
int add_to_playnow_list_by_field(char*, char*);
int get_complete_field_list(char);
int get_recent_albums(song **, int);
int get_recent_artists(song **, int);
int get_recent_songs(song **, int);
int get_tag_from_song(char*, char*, char*);
int get_songs_and_tags_in_dir(char*, song**);
int get_all_songs(struct gen_node**);
int get_tag_info(char, char*);
int search(char*, char*, song**);

#endif /* SRC_SQLITE_H_ */
