/*
 * three_lists.c
 *
 *  Created on: Dec 25, 2016
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
#include "three_lists.h"

#include "current_song.h"
#include "list_utils.h"
#include "net_utils.h"
#include "playnow_list.h"
#include "queue.h"
#include "songhash.h"
#include "sqlite.h"

char *historyfile = NULL;
char *skippedfile = NULL;
char *albumhistory = NULL;
char *artisthistory = NULL;
char *genrehistory = NULL;

struct gen_node* head = NULL;//the main list
struct gen_node* removed = NULL;//the dead list
struct gen_node* msalbum_head = NULL;//multisong albums
struct gen_node* msalbum_removed = NULL;
struct gen_node* artist_head = NULL;//artists
struct gen_node* artist_removed = NULL;
struct gen_node* genre_head = NULL;//genres
struct gen_node* genre_removed = NULL;
struct gen_node* history_head = NULL;
struct gen_node* back_head = NULL;


static int songs_on_list = 0;
static int songs_played = 0;
static int albums_on_list = 0;
static int albums_played = 0;
static int artists_on_list = 0;
static int artists_played = 0;
static int genres_on_list = 0;
static int genres_played = 0;
static int hist_count = 0;
static int back_count = 0;


void three_lists_init() {
	int len = strlen(homedir);
	init_head(&head);
	init_head(&removed);
	init_head(&msalbum_head);
	init_head(&msalbum_removed);
	init_head(&artist_head);
	init_head(&artist_removed);
	init_head(&genre_head);
	init_head(&genre_removed);
	init_head(&history_head);
	init_head(&back_head);
	playnow_list_init();

	songs_on_list = get_all_songs(&head);
	printf("found %d songs\n", songs_on_list);
	albums_on_list = get_msalbums_for_ll(&msalbum_head);
	printf("found %d albums\n", albums_on_list);
	artists_on_list = get_artists_for_ll(&artist_head);
	printf("found %d artists\n", artists_on_list);
	genres_on_list = get_genres_for_ll(&genre_head);
	printf("found %d genres\n", genres_on_list);
	historyfile = malloc(len + 13);
	memset(historyfile, 0, len + 13);
	memcpy(historyfile, homedir, len);
	strcat(historyfile, "/history.txt");
	skippedfile = malloc(len + 13);
	memset(skippedfile, 0, len + 13);
	memcpy(skippedfile, homedir, len);
	strcat(skippedfile, "/skipped.txt");
	albumhistory = malloc(len + 15);
	memset(albumhistory, 0, len + 15);
	memcpy(albumhistory, homedir, len);
	strcat(albumhistory, "/albumhist.txt");
	artisthistory = malloc(len + 16);
	memset(artisthistory, 0, len + 16);
	memcpy(artisthistory, homedir, len);
	strcat(artisthistory, "/artisthist.txt");
	genrehistory = malloc(len + 15);
	memset(genrehistory, 0, len + 15);
	memcpy(genrehistory, homedir, len);
	strcat(genrehistory, "/genrehist.txt");
	filter_from_history();
	filter_from_album_history();
	filter_from_artist_history();
	filter_from_genre_history();
}
// file history functions--------------------------------------------
void add_to_history(char *song)
{
	if(back_count > 0)
		return;
	int a[2];
	retrieve_by_song(song, a);
	make_push_node(&history_head, song, a[0], a[1]);
	if(! check_song_in_history(song))
	{
		FILE *fp = fopen(historyfile, "a");
		if (fp == NULL) {
			printf("can't open history\n");
			error("history file error");
			return;
		}
		fprintf(fp, "%s\n", song);
		fclose(fp);
		add_to_history_hash(song);
	}
	update_history();
	hist_count++;
}
void add_to_skipped(char* song)
{
	if(back_count != 0)
		return;
	FILE *fp = fopen(skippedfile, "a");
	if (fp == NULL) {
		printf("can't open skipped file\n");
		error("skippedfile error");
		return;
	}
	fprintf(fp, "%s\n", song);
	fclose(fp);
	add_to_history_hash(song);
}
void add_to_album_history(char *album)
{
	FILE *fp = fopen(albumhistory, "a");
	if (fp == NULL) {
		printf("can't open album history\n");
		error("albumhist error");
		return;
	}
	fprintf(fp, "%s\n", album);
	fclose(fp);
}
void add_to_artist_history(char *artist)
{
	FILE *fp = fopen(artisthistory, "a");
	if (fp == NULL) {
		printf("can't open album history\n");
		error("albumhist error");
		return;
	}
	fprintf(fp, "%s\n", artist);
	fclose(fp);
}
void add_to_genre_history(char *genre)
{
	FILE *fp = fopen(genrehistory, "a");
	if (fp == NULL) {
		printf("can't open album history\n");
		error("albumhist error");
		return;
	}
	fprintf(fp, "%s\n", genre);
	fclose(fp);
}
void filter_from_history()
{
	printf("reading history...\n");
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	//add history from file into hash and make a linked list
	if ((fp = fopen(historyfile, "r")) == NULL) {
		printf("can't open %s for reading\n", historyfile);
		fp = fopen(historyfile, "a+");
		fclose(fp);
		return;
	}
	else
	{
		while ((read = getline(&line, &len, fp)) != -1) {
			int s = strlen(line);
			line[s-1] = '\0';
			add_to_history_hash(line);
			int a[2];
			retrieve_by_song(line, a);
			make_push_node(&history_head, line, a[0], a[1]);
			hist_count++;
		}
		fclose(fp);
	}
	//find all the songs that have been played and delete the nodes
	struct gen_node *cur = head;
	while(cur)
	{
		if(check_song_in_history(cur->name))
		{
			struct gen_node *temp = cur->next;
			move_node(&head, &removed, cur);
			songs_played++;
			songs_on_list--;
			cur = temp;
		}
		else
			cur = cur->next;
	}
	//reads the skipped.txt file for songs that have been looked at but not played
	//adds them to the history hash so they can be removed from the playlist
	printf("reading skipped...\n");
	if ((fp = fopen(skippedfile, "r")) == NULL) {
		printf("can't open %s for reading\n", skippedfile);
	}
	else
	{
		line = NULL;
		len = 0;

		while ((read = getline(&line, &len, fp)) != -1) {
			int s = strlen(line);
			line[s-1] = '\0';
			add_to_history_hash(line);
		}
		fclose(fp);
	}
	//move all the songs that have been played on to the removed list
	//together the history and removed lists contain all of the songs
	cur = head;
	while(cur)
	{
		if(check_song_in_history(cur->name))
		{
			struct gen_node *temp = cur->next;
			move_node(&head, &removed, cur);
			songs_played++;
			songs_on_list--;
			cur = temp;
		}
		else
			cur = cur->next;
	}
	printf("history count = %d\n", hist_count);
	printf("songs played = %d\n", songs_played);
}
void filter_from_album_history()
{
	printf("reading album history...\n");
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	if ((fp = fopen(albumhistory, "a+")) == NULL) {
		printf("can't open %s for reading\n", albumhistory);
	}
	else
	{
		while ((read = getline(&line, &len, fp)) != -1) {
			int s = strlen(line);
			line[s-1] = '\0';
			struct gen_node *cur = msalbum_head;
			for(; cur; cur = cur->next)
			{
				if(strcmp(line, cur->name) == 0)
				{
					move_node(&msalbum_head, &msalbum_removed, cur);
					albums_played++;
					albums_on_list--;
					break;
				}
			}
		}
		fclose(fp);
	}
}
void filter_from_artist_history()
{
	printf("reading artist history...\n");
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	if ((fp = fopen(artisthistory, "a+")) == NULL) {
		printf("can't open %s for reading\n", artisthistory);
	}
	else
	{
		while ((read = getline(&line, &len, fp)) != -1) {
			int s = strlen(line);
			line[s-1] = '\0';
			struct gen_node *cur = artist_head;
			for(; cur; cur = cur->next)
			{
				if(strcmp(line, cur->name) == 0)
				{
					move_node(&artist_head, &artist_removed, cur);
					artists_played++;
					artists_on_list--;
					break;
				}
			}
		}
		fclose(fp);
	}
}
void filter_from_genre_history()
{
	printf("reading genre history...\n");
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	if ((fp = fopen(genrehistory, "a+")) == NULL) {
		printf("can't open %s for reading\n", genrehistory);
	}
	else
	{
		while ((read = getline(&line, &len, fp)) != -1) {
			int s = strlen(line);
			line[s-1] = '\0';
			struct gen_node *cur = genre_head;
			for(; cur; cur = cur->next)
			{
				if(strcmp(line, cur->name) == 0)
				{
					move_node(&genre_head, &genre_removed, cur);
					genres_played++;
					genres_on_list--;
					break;
				}
			}
		}
		fclose(fp);
	}
}
int get_songs_in_history(song** data)
{
	int i = 0;
	struct gen_node* cur = history_head;
	for(; cur; cur = cur->next)
	{
		(*data)[i].file = malloc(strlen(cur->name) + 1);
		memcpy((*data)[i].file, cur->name, strlen(cur->name) + 1);
		(*data)[i].weight = cur->weight;
		(*data)[i].sticky = cur->sticky;
		i++;
	}
	return i;
}
void update_history()
{
	if(hist_count < 1)
		return;
	song* data = malloc(sizeof(song));
	data->file = malloc(strlen(history_head->name) + 1);
	memcpy(data->file, history_head->name, strlen(history_head->name) + 1);
	data->weight = history_head->weight;
	data->sticky = history_head->sticky;
	send_song_data(data, 1, 'K');
	free(data->file);
	free(data);
}
//clears out history and skipped
void clear_history_files()
{
	FILE *fp;
	if ((fp = fopen(historyfile, "w")) == NULL)
		printf("can't open %s for reading\n", historyfile);
	else
		fclose(fp);
	if ((fp = fopen(skippedfile, "w")) == NULL)
		printf("can't open %s for reading\n", historyfile);
	else
		fclose(fp);
}
void make_push_song_node(char* file, int weight, int sticky)
{
	make_push_node(&head, file, weight, sticky);
}
// backlist functions--------------------------
void push_cursong_to_back()
{
	if(cursong == NULL)
		return;
	if(back_count == 0)
	{
		init_head(&back_head);
	}
	int a[2];
	retrieve_by_song(cursong, a);
	make_push_node(&back_head, cursong,a[0], a[1]);
	back_count++;
	int i = 0;
	printf("next next %s\n", history_head->next->next->name);
	struct gen_node* cur = history_head;
	while(i < back_count)
	{
		if(cur)
			cur = cur->next;
		i++;
	}
	printf("cur now %s\n", cur->name);
	printf("i = %d\n", i);
	if(cur->name != NULL)
	{
		printf("pushing %s to history\n", cur->name);
		make_push_node(&back_head, cur->name, cur->weight, cur->sticky);
		back_count++;
	}
	printf("BACK HEAD\n");
	print_list_from_head(back_head);
	printf("\n\n");
}
void set_cursong_from_back()
{
	set_cursong(back_head->name);
	pop_head(&back_head);
	back_count--;
}
int get_backcount()
{
	return back_count;
}
//network functions-----------------------------------------------------
void send_remaining()
{
	int played = 0;
	int on_list = 0;
	if(get_qcount() > 0)
	{
		played = get_qcount();
		on_list = get_qcount();;
	}
	else if (strcmp(val.playby, "song") == 0)
	{
		played = songs_played;
		on_list = songs_on_list;
	}
	else if (strcmp(val.playby, "album") == 0)
	{
		played = albums_played;
		on_list = albums_on_list;
	}
	else if (strcmp(val.playby, "artist") == 0)
	{
		played = artists_played;
		on_list = artists_on_list;
	}
	else if (strcmp(val.playby, "genre") == 0)
	{
		played = genres_played;
		on_list = genres_on_list;
	}
	char playing[16];
	memset(playing, 0, 16);
	playing[0] = 'R';
	char *p = &playing[1];
	char num[8];
	memset(num, 0, 8);
	//itoa(songs_played, played);
	snprintf(num, 8, "%d", played);
	char total[8];
	memset(total, 0, 8);
	//itoa(songs_on_playlist, total);
	snprintf(total, 8, "%d", on_list);
	memcpy(p, num, strlen(num));
	p += strlen(num);
	memcpy(p++, "/", 1);
	memcpy(p, total, strlen(total) + 1);
	printf("remaining command = %s\n", playing);
	send_command(playing, strlen(playing) + 1);
}
// generate playlist functions---------------------------------------------
void generate_playlist()
{
	if(strcmp(val.playby, "song") == 0)
	{
		if(get_playnow_left() == 0)
			generate_playlist_from_list();
	}
	else if (strcmp(val.playby, "album") == 0)
	{
		if(get_playnow_left() == 0)
			generate_playlist_from_albumlist();
	}
	else if (strcmp(val.playby, "artist") == 0)
	{
		if(get_playnow_left() == 0)
			generate_playlist_from_artistlist();
	}
	else if (strcmp(val.playby, "genre") == 0)
	{
		if(get_playnow_left() == 0)
			generate_playlist_from_genrelist();
	}
	else
		printf("we should never get here :(\n\n\n");
}
void generate_playlist_from_list()
{
	if(songs_on_list == 0)
	{
		head = removed;
		init_head(&removed);
		clear_history();
		clear_history_files();
		songs_on_list = songs_played;
		songs_played = 0;
	}
	get_random_node(&head, &removed, songs_on_list);
	//printf("playing song %s\t", removed->name);
	push_to_playnow_list(removed->name, removed->weight, removed->sticky);
	songs_played++;
	songs_on_list--;

	//printf("songs on list = %d\n", songs_on_list);
	//printf("songs played = %d\n", songs_played);
}
void generate_playlist_from_albumlist()
{
	do
	{
		if(albums_on_list == 0)
		{
			msalbum_head = msalbum_removed;
			init_head(&msalbum_removed);
			albums_on_list = albums_played;
			albums_played = 0;
			FILE *fp;
			if ((fp = fopen(albumhistory, "w")) == NULL)
				printf("can't open %s for reading\n", albumhistory);
			else
				fclose(fp);
		}
		get_random_node(&msalbum_head, &msalbum_removed, albums_on_list);
		add_to_album_history(msalbum_removed->name);
		albums_played++;
		albums_on_list--;
	} while(! should_play(msalbum_removed->weight));
	if(msalbum_removed->sticky == 0 && msalbum_removed->weight > 0)
	{
		msalbum_removed->weight--;
		set_album_weight(msalbum_removed->name, msalbum_removed->weight);
	}
	printf("playing album %s\n", msalbum_removed->name);
	int tot = add_to_playnow_list_by_field("TALB", msalbum_removed->name);
	printf("found %d tracks\n", tot);
	if(tot == 0)
		printf("\n\n\t!!!No tracks found for %s\n\n", msalbum_removed->name);

}
void generate_playlist_from_artistlist()
{
	do
	{
		if(artists_on_list == 0)
		{
			artist_head = artist_removed;
			init_head(&artist_removed);
			artists_on_list = artists_played;
			artists_played = 0;
			FILE *fp;
			if ((fp = fopen(artisthistory, "w")) == NULL)
				printf("can't open %s for reading\n", artisthistory);
			else
				fclose(fp);
		}
		get_random_node(&artist_head, &artist_removed, artists_on_list);
		add_to_artist_history(artist_removed->name);
		artists_played++;
		artists_on_list--;
	} while (! should_play(artist_removed->weight));
	if(artist_removed->sticky == 0 && artist_removed->weight > 0)
	{
		artist_removed->weight--;
		set_artist_weight(artist_removed->name, artist_removed->weight);
	}
	int tot = add_to_playnow_list_by_field("TPE1", artist_removed->name);
	printf("found %d tracks\n", tot);
	if(tot == 0)
		printf("\n\n\t!!!No tracks found for %s\n\n", artist_removed->name);
}
void generate_playlist_from_genrelist()
{
	do
	{
		if(genres_on_list == 0)
		{
			genre_head = genre_removed;
			init_head(&genre_removed);
			genres_on_list = genres_played;
			genres_played = 0;
			FILE *fp;
			if ((fp = fopen(genrehistory, "w")) == NULL)
				printf("can't open %s for reading\n", genrehistory);
			else
				fclose(fp);
		}
		get_random_node(&genre_head, &genre_removed, genres_on_list);
		add_to_genre_history(genre_removed->name);
		genres_played++;
		genres_on_list--;
	} while (! should_play(genre_removed->weight));
	if(genre_removed->sticky == 0 && genre_removed->weight > 0)
	{
		genre_removed->weight--;
		set_artist_weight(genre_removed->name, genre_removed->weight);
	}
	int tot = add_to_playnow_list_by_field("TCON", genre_removed->name);
	printf("found %d tracks\n", tot);
	if(tot == 0)
	{
		printf("\n\n\t!!!No tracks found for %s\n\n", genre_removed->name);
	}
}
