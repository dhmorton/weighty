/*
 * network.c
 *
 *  Created on: Aug 8, 2008
*  Copyright: 2008-2017 David Morton
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
#include "network.h"

#include "alarm.h"
#include "current_song.h"
#include "list_manager.h"
#include "mtputils.h"
#include "myflactag.h"
#include "myid3tag.h"
#include "mylyrics.h"
#include "mymp4tag.h"
#include "myoggtag.h"
#include "mytaglib.h"
#include "myutils.h"
#include "myxine.h"
#include "net_utils.h"
#include "player.h"
#include "playnow_list.h"
#include "queue.h"
#include "sleep.h"
#include "songhash.h"
#include "sqlite.h"
#include "three_lists.h"

static char data_buf[32*BUFF_SIZE];
static char *pbuf;
static int bytes_parsed = 0;
static int data_buf_len = 0;
static int mysock;
static fd_set socks;

struct config val;

static void setnonblocking(int*);
static int recv_data(int);
static int get_string_from_buf(char*);
static int get_num_from_buf(void);
static int buf_shift(void);
static void buf_step(void);
static void flush_buffer(void);
static void parse_command(void);
static int send_songs_in_dir(void);
static void get_track_list_by_playby(void);
static void get_list_of_all_fields(void);
static void get_field_list(void);
static void get_field_song_list(void);
static void get_recent(void);
static int get_history(void);
static void get_config(void);
static void get_query(void);
static void parse_lyrics_request(void);
static void play_album_now(void);
static void play_by_field(void);
static void net_enqueue(void);
static void parse_jump(void);
static void clear_n_queue(void);
static void update_database(void);
static void parse_config(void);
static void parse_alarm_config(void);
static void parse_sleep_config(void);
static void change_weight(void);
static void change_song_weight(void);
static void set_sticky(void);
static void set_current_sticky(void);
static void set_tag(void);
static void set_vol(void);
static void set_item_weight(void);
static void parse_phone_request(void);

/*
 * Socket and data functions
 */
void weighty(int sock)
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	struct sockaddr_in client_addr;

	//the main loop
	while (1)
	{
		fd_set read_socks = socks;
		if (select(FD_SETSIZE, &read_socks, NULL, NULL, &tv) < 0)
			printf("error on select\n");
		int i;
		for (i = 0; i < FD_SETSIZE; i++)
		{
			/* Only checking for reads since every read generates a write
			 * The networking write stuff happens in send_command() */
			if (FD_ISSET(i, &read_socks))
			{
				// new connection request
				if (i == sock)
				{
					printf("processing a new connection...");
					int new;
					socklen_t clilen = sizeof(client_addr);
					memset(&client_addr, 0, clilen);
					new = accept(sock, (struct sockaddr *) &client_addr, &clilen);
					if (new < 0)
						printf("error accepting new connection...\n");
					setnonblocking(&new);
					FD_SET(new, &socks);
					update_socks(new);
					printf("%d Done\n", new);
					update_gui();
				}
				else
					recv_data(i);
			}
		}
		check_alarm();
		check_sleep();
		usleep(TICK);
		update_progressbar();
	}
}
int create_new_socket()
{
	struct sockaddr_in addr;

	printf("creating new socket...");
	if ((mysock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		error("error opening socket");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(mysock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		error("error binding");

	if (listen(mysock, 1) < 0)
		error("listen failed");

	printf("\tlistening on %d to socket %d\n", PORT, mysock);
	setnonblocking(&mysock);
	FD_ZERO(&socks);
	FD_SET(mysock, &socks);
	return mysock;
}
void setnonblocking(int *sock)
{
	int opts;
	opts = fcntl(*sock,F_GETFL);
	if (opts < 0)
		error("fcntl");
	opts = (opts | O_NONBLOCK);
	if (fcntl(*sock,F_SETFL, opts) < 0)
		error("fnctl F_SETFL");
	return;
}
int recv_data(int sock)
{
	ssize_t bytes = -1;
	int recv = 0;
	char buf[BUFF];
	char string[sizeof(char)*BUFF*10];
	char *pstring = &string[0];
	//printf("\tSOCK = %d\n", sock);
	while ((bytes = read(sock, buf, BUFF)))
	{
		if (bytes > 0) {
			memcpy(pstring, buf, bytes);//keep copying stuff onto string until we're done reading or something
			pstring += bytes;
			recv += (int) bytes;
		}
		else
			break;
	}
	if (bytes == 0)
	{
		printf("socket closed: FD=%d\n", sock);
		close(sock);
		FD_CLR(sock, &socks);
		clear_sock(sock);
		return -1;
	}
	else if (recv > 0)
	{
		//shift all the read data off first
		if (data_buf[0] != '\0')
		{
			//printf("adding data to the end of data_buf %s\n", string);
			int i;
			for (i = 0; i < recv; i++)
				data_buf[data_buf_len+i] = string[i];
			data_buf_len += recv;
		}
		else//otherwise put it in the holding buffer because we don't know if it's complete or not
		{
			//printf("new data %s\n", string);
			memcpy(data_buf, string, recv);
			data_buf_len = recv;
			pbuf = &data_buf[0];
		}
	}
	while ((data_buf_len - bytes_parsed) > 1)
		parse_command();
	//somehow two clients messed up the counting. Should really figure out where that's happening.
	flush_buffer();
	return 0;
}
//parses data_buf (through the pointer pbuf) up to the next \0
//fills in the passed char array with that string
//returns the length of the string *including the nt*
int get_string_from_buf(char* s)
{
	int j = 0;
	if (*pbuf == '\0')
		return -1;
	while (*pbuf != '\0')
		s[j++] = *(pbuf++);
	s[j++] = *(pbuf++);
	bytes_parsed += j;
	return j;
}
//parses data_buf (through the pointer pbuf) up to the next \0
//returns that data as an integer
int get_num_from_buf()
{
	char s[10];
	memset(s, 0, 10);
	int j = 0;
	do
	{
		s[j] = *pbuf;
		pbuf++; j++;
	} while (*pbuf != '\0');
	pbuf++; j++;
	bytes_parsed += j;
	return atoi(s);
}
int buf_shift()
{
//	if (bytes_parsed < 1000)
//		return 1;
	if (data_buf_len == bytes_parsed)
	{
		memset(&data_buf[0], 0, BUFF_SIZE);
		pbuf = &data_buf[0];
		data_buf_len = bytes_parsed = 0;
	}
	else if ((data_buf_len - bytes_parsed) < 4096)
	{
		char *p = pbuf;
		int i;
		for (i = 0; i < (data_buf_len - bytes_parsed); i++)
			data_buf[i] = *(p++);
		data_buf_len -= bytes_parsed;
		bytes_parsed = 0;
		pbuf = &data_buf[0];
	}
	return (data_buf_len - bytes_parsed);
}
void buf_step()
{
	pbuf++;
	bytes_parsed++;
}
void flush_buffer()
{
	memset(&data_buf[0], 0, BUFF_SIZE);
	pbuf = &data_buf[0];
	data_buf_len = bytes_parsed = 0;
}
//parses commands from the client
void parse_command()
{
	//print_data(data_buf, data_buf_len);
	//playback commands
	if (data_buf_len - bytes_parsed < 2)
		printf("parse wait\n");//wait if there aren't at least two chars to parse
	//playback commands
	else if (*pbuf == 'P')
	{
		buf_step();
		if (*pbuf == 'L')
			net_play();
		else if (*pbuf == 'S')
		{
			buf_step();
			stop();
			buf_shift();
		}
		else if (*pbuf == 'N')
		{
			next();
			buf_step();
			buf_shift();
		}
		else if (*pbuf == 'A')
		{
			pausetoggle();
			buf_step();
			buf_shift();
		}
		else if (*pbuf == 'B')
		{
			back();
			buf_step();
			buf_shift();
		}
		else if (*pbuf == 'G')//generate new playlist and skip to next song
		{
			clear_playnow_lists();
			next();
			buf_step();
			buf_shift();
		}
		else if (*pbuf == 'F')//play by field from List tab
		{
			buf_step();
//			play_playlist();
			play_by_field();
			buf_shift();
		}
		else if (*pbuf == 'J')//jump to position
		{
			buf_step();
			parse_jump();
			buf_shift();
		}
		else if (*pbuf == 'P')//start to sleep
		{
			buf_step();
			start_sleep();
			check_sleep();
			buf_shift();
		}
		else if (*pbuf == 'O')//stop sleeping
		{
			buf_step();
			stop_sleep();
			buf_shift();
		}
		else if (*pbuf == 'C')//play full album from current song
		{
			buf_step();
			play_album_now(); //commented out just to try out the phone function
			buf_shift();
		}
		else
			printf("No such P command %s\n", pbuf);
	}
	//data commands
	else if (*pbuf == 'D')
	{
		buf_step();
		if (*pbuf == 'L')
			send_songs_in_dir();
		else if (*pbuf == 'C')
			get_config();
		else if (*pbuf == 'H')
			get_history();
		else if (*pbuf == 'S')
		{
			buf_step();
			send_stats();
			buf_shift();
		}
		else if (*pbuf == 'V')
		{
			buf_step();
			buf_shift();
			send_volume();
		}
		else if (*pbuf == 'U')
			update_database();
		else if (*pbuf == 'Q')//search
			get_query();
		else if (*pbuf == 'F')//get field data
			get_field_list();
		else if (*pbuf == 'T')//get field song data
			get_field_song_list();
		else if (*pbuf == 'R')//get recent data
			get_recent();
		else if (*pbuf == 'I')//get a track listing based on type of playby
			get_track_list_by_playby();
		else if (*pbuf == 'M')
			get_list_of_all_fields();
		else if (*pbuf == 'Y')//get lyrics
			parse_lyrics_request();
		else if (*pbuf == 'P')//transfer data to phone
			parse_phone_request();
		else if (*pbuf == 'D')//get discogs key
		{
			buf_step();
			send_discogs_key();
		}
		else
			printf("No such D command %s\n", pbuf);
	}
	//set property commands
	else if (*pbuf == 'S')
	{
		buf_step();
		if (*pbuf == 'W')
			change_weight();
		else if (*pbuf == 'S')
			change_song_weight();
		else if (*pbuf == 'P')
			set_sticky();
		else if (*pbuf == 'C')
			parse_config();
		else if (*pbuf == 'D')//write config
		{
			buf_step();
			write_config();
			buf_shift();
		}
		else if (*pbuf == 'V')
			set_vol();
		else if (*pbuf == 'I')//set item weights
			set_item_weight();
		else if (*pbuf == 'N')//set sticky of current song
			set_current_sticky();
		else if (*pbuf == 'T')//set tag
			set_tag();
		else if (*pbuf == 'A')//save alarm settings
			parse_alarm_config();
		else if (*pbuf == 'L')//sleep config settings
			parse_sleep_config();
		else
			printf("No such S command %s\n", pbuf);
	}
	//queue commands
	else if (*pbuf == 'Q')
	{
		buf_step();
		if (*pbuf == 'Q')
			net_enqueue();
		else if (*pbuf == 'G')
		{
			buf_step();
			get_queue();
			buf_shift();
		}
		else if (*pbuf == 'C')
		{
			buf_step();
			clear_queue();
			buf_shift();
		}
		else if (*pbuf == 'N')//clear n songs from queue
			clear_n_queue();
		else
			printf("No such Q command %s\n", pbuf);
	}
	else
	{
		//something went wrong somewhere, just flush everything on the buffer and reset
		printf("No such command %s\n", pbuf);
		flush_buffer();
	}
}
/*
 * Send data functions
 */
//get all the songs in *dir and send one char of sticky data, nt, weight, nt, song name (not full path), nt
int send_songs_in_dir()
{
	buf_step();
	char dir[BUFF];
	get_string_from_buf(dir);
	song *data;
	data = malloc(40000*sizeof(song));
	//printf("dir = %s\n", dir);
	int songs = get_songs_and_tags_in_dir(dir, &data);
	//printf("found %d songs\n", songs);
	send_song_data(data, songs, 'F');
	free(data);
	buf_shift();
	return 0;
}
/*
 * Get Functions
 */
void get_track_list_by_playby()
{
	buf_step();
	int tot;
	song *data;
	data = malloc(20000*sizeof(song));
	char *tag = malloc(1024);
	memset(tag, 0, 1024);
	if ((strcasecmp(val.playby, "song") == 0) || (strcasecmp(val.playby, "album") == 0))//get album listing
	{
		get_tag_from_song(cursong, tag, "TALB");
		printf("album = %s\n", tag);
		tot = get_songs_by_field(&data, "TALB", tag);
	}
	else if (! strcasecmp(val.playby, "artist"))
	{
		get_tag_from_song(cursong, tag, "TPE1");
		printf("artist = %s\n", tag);
		tot = get_songs_by_field(&data, "TPE1", tag);
	}
	else if (! strcasecmp(val.playby, "genre"))
	{
		get_tag_from_song(cursong, tag, "TCON");
		printf("genre = %s\n", tag);
		tot = get_songs_by_field(&data, "TCON", tag);
	}
	send_song_data(data, tot, 'I');
	free(data);
	buf_shift();
}
void get_query()
{
	buf_step();
	char field[64];
	get_string_from_buf(field);
	char query[1024];
	get_string_from_buf(query);
	song *data;
	data = malloc(20000*sizeof(song));
	int songs = search(field, query, &data);
	if (songs > 0)
		send_song_data(data, songs, 'Q');
	else
		;
	free(data);
	buf_shift();
}
void get_list_of_all_fields()
{
	buf_step();
	get_complete_field_list(pbuf[0]);
	buf_step();
	buf_shift();
}
void get_field_list()
{
	buf_step();
	song *data;
	int tot;
	data = malloc(20000*sizeof(song));
	if (*pbuf == 'T')
		tot = get_artists(&data);
	else if (*pbuf == 'G')
		tot = get_genres(&data);
	else if (*pbuf == 'A')
		tot = get_multisong_albums(&data);
	buf_step();
	printf("tot = %d\n", tot);
	send_song_data(data, tot, 'A');
	free(data);
	buf_shift();
}
void get_field_song_list()
{
	buf_step();
	char item[1024];
	song *data;
	data = malloc(20000*sizeof(song));
	int tot;
	if (*pbuf == 'A')
	{
		buf_step();
		get_string_from_buf(item);
		printf("TALB %s\n", item);
		tot = get_songs_and_tags_by_field(&data, "TALB", item);
	}
	else if (*pbuf == 'T')
	{
		buf_step();
		get_string_from_buf(item);
		tot = get_songs_and_tags_by_field(&data, "TPE1", item);
	}
	else if (*pbuf == 'G')
	{
		buf_step();
		get_string_from_buf(item);
		tot = get_songs_and_tags_by_field(&data, "TCON", item);
	}
	else
		printf("failed %s\n", pbuf);
	send_song_data(data, tot, 'D');
	free(data);
	buf_shift();
}
void get_recent()
{
	buf_step();
	int time;
	song *data;
	data = malloc(20000*sizeof(song));
	int tot;
	if (*pbuf == 'A')
	{
		buf_step();
		time = get_num_from_buf();
		tot = get_recent_albums(&data, time);
	}
	else if (*pbuf == 'T')
	{
		buf_step();
		time = get_num_from_buf();
		tot = get_recent_artists(&data, time);
	}
	else if (*pbuf == 'S')
	{
		buf_step();
		time = get_num_from_buf();
		tot = get_recent_songs(&data, time);
	}
	send_song_data(data, tot, 'A');
	free(data);
	buf_shift();
}
void get_config()
{
	buf_step();
	buf_shift();
	char com[BUFF];
	com[0] = 'C';
	char *p = &com[1];
	int len = 1;
	memcpy(p, val.musicdir, strlen(val.musicdir) + 1);
	p += strlen(val.musicdir) + 1;
	len += strlen(val.musicdir) + 1;

	char t[4];
	memset(t, 0, 4);
	//itoa(val.threshhold, t);
	snprintf(t, 4, "%d", val.threshhold);
	memcpy(p, t, strlen(t) + 1);
	p += strlen(t) + 1;
	len += strlen(t) + 1;
	memcpy(p, val.type, strlen(val.type) + 1);
	p += strlen(val.type) + 1;
	len += strlen(val.type) + 1;
	char v[3];
	memset(v, 0, 3);
	//itoa(val.var, v);
	snprintf(v, 3, "%d", val.var);
	memcpy(p, v, strlen(v) + 1);
	p += strlen(v) + 1;
	len += strlen(v) + 1;
	memcpy(p, val.playby, strlen(val.playby) + 1);
	p+= strlen(val.playby) + 1;
	len += strlen(val.playby) + 1;

	if (val.song_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;
	if (val.song_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;

	if (val.album_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;
	if (val.album_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;

	if (val.artist_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;
	if (val.artist_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;

	if (val.genre_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;
	if (val.genre_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	len++;
	memcpy(p++, "\0", 1);
	len++;

	send_command(com, len);
}
int get_history()
{
	buf_step();
	printf("get history\n");
	song *data;
	data = malloc(20000*sizeof(song));
	int songs = get_songs_in_history(&data);
	//printf("found %d songs\n", songs);
	send_song_data(data, songs, 'H');
	free(data);
	buf_shift();
	//print_data(pbuf, data_buf_len - bytes_parsed);
	return 0;
}
void parse_lyrics_request()
{
	buf_step();
	char artist[256];
	memset(artist, 0, 256);
	get_string_from_buf(artist);
	char title[256];
	memset(title, 0, 256);
	get_string_from_buf(title);
	//get_lyrics(artist, title);
	buf_shift();
}
/*
 * Set Functions
 */
//changes the weight of the current song by delta
void change_weight()
{
	buf_step();
	int delta = get_num_from_buf();
	update_weight(delta);
	buf_shift();
}
//changes the weight of a given song
void change_song_weight()
{
	buf_step();
	int weight = get_num_from_buf();
	char song[BUFF];
	get_string_from_buf(song);
	set_song_weight(song, weight);
	buf_shift();
}
//sets/unsets the sticky flag given a list or song
void set_sticky()
{
	buf_step();
	int flag = -1;
	if (*pbuf == 'S')
		flag = 0;
	else if (*pbuf == 'A')
		flag = 1;
	else if (*pbuf == 'T')
		flag = 2;
	else if (*pbuf == 'G')
		flag = 3;
	buf_step();
	int sticky = get_num_from_buf();
	char item[BUFF];
	get_string_from_buf(item);
	switch (flag)
	{
	case 0:
		set_song_sticky(item, sticky);
		break;
	case 1:
		set_album_sticky(item, sticky);
		break;
	case 2:
		set_artist_sticky(item, sticky);
		break;
	case 3:
		set_genre_sticky(item, sticky);
		break;
	default:
		printf("set_sticky case error\n");
	}
	buf_shift();
}
//sets/unsets the sticky flag on the current song
void set_current_sticky()
{
	buf_step();
	int sticky = get_num_from_buf();
	set_cursong_sticky(sticky);
}
void set_vol()
{
	buf_step();
	int vol = get_num_from_buf();
	set_volume(vol);
	buf_shift();
}
void set_item_weight()
{
	buf_step();
	char item[1024];
	if (*pbuf == 'A')
	{
		buf_step();
		int weight = get_num_from_buf();
		get_string_from_buf(item);
		int ret = set_album_weight(item, weight);
		printf("set album weight ret = %d\n", ret);
	}
	else if (*pbuf == 'T')
	{
		buf_step();
		int weight = get_num_from_buf();
		get_string_from_buf(item);
		set_artist_weight(item, weight);
	}
	else if (*pbuf == 'G')
	{
		buf_step();
		int weight = get_num_from_buf();
		get_string_from_buf(item);
		set_genre_weight(item, weight);
	}
	else
		printf("failed %s\n", pbuf);
	buf_shift();
}
void set_tag()
{
	buf_step();
	int num_tags = get_num_from_buf();
	char file[BUFF];
	get_string_from_buf(file);
	char field[64];
	char tag[BUFF];
	int i;
	char* fields[num_tags];
	char* tags[num_tags];
	for (i = 0; i < num_tags; i++)
	{
		memset(field, 0, 64);
		memset(tag, 0, BUFF);
		get_string_from_buf(field);
		if (get_string_from_buf(tag) < 0)
		{
			buf_step();
//		clear_mp3_tag(file, field);
		}
		fields[i] = malloc(strlen(field) + 1);
		memcpy(fields[i], field, strlen(field) + 1);
		if (strlen(tag) == 0) {
			printf("EMPTY TAG for %s\n", field);
			tags[i] = NULL;
		}
		else {
			tags[i] = malloc(strlen(tag) + 1);
			memcpy(tags[i], tag, strlen(tag) + 1);
		}
	}
	if (check_file(file) == 0)
		write_ogg_tags(file, num_tags, fields, tags);
	else if (check_file(file) == 1)
		write_mp3_tags(file, num_tags, fields, tags);
	else if (check_file(file) == 2)
		write_flac_tags(file, num_tags, fields, tags);
	else if (check_file(file) == 4)
		write_mp4_tags(file, num_tags, fields, tags);
	else
		write_taglib_tags(file, num_tags, fields, tags);
	buf_shift();
	if (num_tags > 0) {
		for (i = 0; i < num_tags; i++) {
			if (fields[i] != NULL)
				free(fields[i]);
			if (tags[i] != NULL)
				free(tags[i]);
		}

	}
}
void parse_config()
{
	buf_step();
	get_string_from_buf(val.musicdir);
	val.threshhold = get_num_from_buf();
	get_string_from_buf(val.type);
	val.var = get_num_from_buf();
	get_string_from_buf(val.playby);
	val.song_rand = get_num_from_buf();
	val.song_skip = get_num_from_buf();
	val.album_rand = get_num_from_buf();
	val.album_skip = get_num_from_buf();
	val.artist_rand = get_num_from_buf();
	val.artist_skip = get_num_from_buf();
	val.genre_rand = get_num_from_buf();
	val.genre_skip = get_num_from_buf();
	//generate_playlist();
	buf_shift();
}
void parse_alarm_config()
{
	buf_step();
	int i;
	for (i = 0; i < 7; i++)
	{
		int set = get_num_from_buf();
		int hr = get_num_from_buf();
		int min = get_num_from_buf();
		set_alarm(i, set, hr, min);
	}
	int start_vol = get_num_from_buf();
	int end_vol = get_num_from_buf();
	int fade = get_num_from_buf();
	char song[BUFF];
	int ret = get_string_from_buf(song);
	if (ret < 0)
	{
		buf_step();
		set_alarm_settings(start_vol, end_vol, fade, NULL);
	}
	else
		set_alarm_settings(start_vol, end_vol, fade, song);
	buf_shift();
}
void parse_sleep_config()
{
	buf_step();
	int play = get_num_from_buf();
	int fade = get_num_from_buf();
	set_sleep(play, fade);
	buf_shift();
}
/*
 * Action commands
 */
void update_database()
{
	buf_step();
	update_songs();
	buf_shift();
}
void net_play()
{
	buf_step();
	char song[BUFF];
	int len = get_string_from_buf(song);
	if (len < 0)
	{
		buf_step();
		next();
	}
	else
		play(song);
	buf_shift();
}
void play_album_now()
{
	printf("play full album\n");
	add_current_album_to_playlist();
	next();
}
void play_by_field()
{
	char field[5];//TALB, TPE1, TCON
	get_string_from_buf(field);
	char name[BUFF];
	get_string_from_buf(name);
	printf("play_by_field %s %s\n", field, name);
	add_field_to_playlist(field, name);
	next();
}
void parse_jump()
{
	int pos = get_num_from_buf();
	jump_to_pos(pos);
}
void net_enqueue()
{
	buf_step();
	char song[BUFF];
	get_string_from_buf(song);
	enqueue(song);
}
void clear_n_queue()
{
	buf_step();
	int count = get_num_from_buf();
	int i;
	for (i = 0; i < count; i++)
	{
		int index = get_num_from_buf();
		clear_queue_by_index(index);
	}
	buf_shift();
}
void parse_phone_request()
{
	buf_step();
	char dist[16];
	get_string_from_buf(dist);
	int thresh = get_num_from_buf();
	int var = get_num_from_buf();
	int time = get_num_from_buf();
	char time_s[16];
	get_string_from_buf(time_s);
	int data = get_num_from_buf();
	char data_s[4];
	get_string_from_buf(data_s);
	//printf("%s %d %d %d %s %d %s\n", dist, thresh, var, time, time_s, data, data_s);
	phone_thread_init(dist, thresh, var, time, time_s, data, data_s);
}

