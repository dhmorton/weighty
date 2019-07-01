/*
 * playnow_list.c
 *
 *  Created on: Dec 26, 2016
 *  Copyright: 2016-2017 David Morton
 *
 *   This file is part of weighty.
 *
 *   Weighty is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Weighty is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with weighty.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "playnow_list.h"

#include "current_song.h"
#include "list_utils.h"
#include "list_utils.h"
#include "myutils.h"
#include "songhash.h"
#include "sqlite.h"
#include "three_lists.h"

struct gen_node* list_head = NULL;//current playnow list
struct gen_node* rem = NULL;

static int songs_on_playnow_list = -1;
static int songs_played = 0;
static int play_full_album = 0;

static int playback_skip(void);
static int playback_random(void);

void playnow_list_init()
{
	if(list_head == NULL)
	{
		list_head = malloc(sizeof(struct gen_node));
		list_head->next = NULL;
		list_head->name = NULL;
	}
	if(rem == NULL)
	{
		rem = malloc(sizeof(struct gen_node));
		rem->next = NULL;
		rem->name = NULL;
	}
}
void push_to_playnow_list(char *file, int weight, int sticky)
{
	if(list_head == NULL)
		playnow_list_init();
	make_push_node(&list_head, file, weight, sticky);
	songs_on_playnow_list++;
}
void add_current_album_to_playlist()
{
	clear_playnow_lists();
	char *album = malloc(1024);
	memset(album, 0, 1024);
	get_tag_from_song(cursong, album, "TALB");
	printf("album = %s\n", album);
	int tot = add_to_playnow_list_by_field("TALB", album);
	printf("found %d songs\n", tot);
	free(album);
	play_full_album = 1;
}
void add_field_to_playlist(char *field, char *name)
{
	clear_playnow_lists();
	int tot = add_to_playnow_list_by_field(field, name);
	printf("found %d songs\n", tot);
	play_full_album = 1;
}
int set_cursong_from_playnow()
{
	if(rem == NULL)
	{
		printf("\tWTF null rem\n\n");
		return 1;
	}
	if(playback_skip() && ! play_full_album)
	{
		while(songs_on_playnow_list > 0)
		{
			int n = 0;
			if(playback_random())
				n = random() % songs_on_playnow_list;
			get_random_node(&list_head, &rem, n);
			songs_played++;
			songs_on_playnow_list--;
			if(should_play(rem->weight)) {
				//printf("testing should_play %d\n", rem->weight);
				break;
			}
			add_to_skipped(rem->name);
			if(songs_on_playnow_list == 0)
				songs_on_playnow_list--;
		}
	}
	else if(playback_random() && ! play_full_album && songs_on_playnow_list > 0)
	{
		printf("playback random\n");
		int n = random() % songs_on_playnow_list;
		get_random_node(&list_head, &rem, n);
		songs_on_playnow_list--;
		songs_played++;
	}
	else
	{
		printf("get first song on playnow list\n");
		get_random_node(&list_head, &rem, 0);
		songs_played++;
		songs_on_playnow_list--;
		if(songs_on_playnow_list == 0)
			play_full_album = 0;
	}
	if(songs_on_playnow_list < 0)
	{
		clear_playnow_lists();
		songs_on_playnow_list = 0;
		play_full_album = 0;
		return 1;
	}
	if(rem->sticky == 0 && rem->weight > 0)
	{
		rem->weight--;
		set_song_weight(rem->name, rem->weight);
	}
	printf("songs on playnow = %d\n", songs_on_playnow_list);
	set_cursong(rem->name);
	if(cursong == NULL)
		return 1;
	return 0;
}
//returns 1 or 0 based on current skip selection
int playback_skip()
{
	if (! strcasecmp(val.playby, "song"))
	{
		if (val.song_skip)
			return 1;
	}
	else if (! strcasecmp(val.playby, "album"))
	{
		if (val.album_skip)
			return 1;
	}
	else if (! strcasecmp(val.playby, "artist"))
	{
		if (val.artist_skip)
			return 1;
	}
	else if (! strcasecmp(val.playby, "genre"))
	{
		if (val.genre_skip)
			return 1;
	}
	return 0;
}
//returns 1 or 0 based on current random selection
int playback_random()
{
	if (! strcasecmp(val.playby, "song"))
	{
		if (val.song_rand)
			return 1;
	}
	else if (! strcasecmp(val.playby, "album"))
	{
		if (val.album_rand)
			return 1;
	}
	else if (! strcasecmp(val.playby, "artist"))
	{
		if (val.artist_rand)
			return 1;
	}
	else if (! strcasecmp(val.playby, "genre"))
	{
		if (val.genre_rand)
			return 1;
	}
	return 0;
}
void clear_playnow_lists()
{
	//printf("CLEARING LISTS\n");
	if(list_head != NULL)
	{
		free(list_head);
		list_head = NULL;
	}
	if(rem != NULL)
	{
		while(rem)
		{
			if(rem->name != NULL)
			{
				free(rem->name);
			}
			struct gen_node* temp = rem->next;
			free(rem);
			rem = temp;
		}
		rem = NULL;
	}
	playnow_list_init();
	songs_on_playnow_list = 0;
}
int get_playnow_left() {
	return songs_on_playnow_list;
}
