/*
 * myxine.c
 *
 *  Created on: Dec 18, 2016
 *      Much copied from the basic xinimin.c example player
 *      http://xinehq.de/index.php/hackersguide
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


#include "myxine.h"

#include "current_song.h"
#include "list_manager.h"
#include "myutils.h"
#include "net_utils.h"
#include "playnow_list.h"
#include "queue.h"
#include "three_lists.h"
#include "sqlite.h"
#include "audioout.h"

static xine_t *xine;
static xine_stream_t *stream;
static xine_audio_port_t *ao_port;
static xine_event_queue_t *event_queue;
static stream_track *stream_history = NULL;
static int streamcount = 0;
static int streaming = 0;
static int running = 0;
static int xine_running = 0;

static void event_listener(void *user_data, const xine_event_t *event)
{
  switch(event->type)
  {
  case XINE_EVENT_UI_PLAYBACK_FINISHED:
	  //printf("playback finished\n");
	  free_cursong();
	  while(play(NULL))
		  ;
	  break;
  case XINE_EVENT_AUDIO_LEVEL://volume changed
	  send_volume();//function in network.c - gets the current volume and sends it to the client
	  break;
  case XINE_EVENT_UI_SET_TITLE:
	  printf("set title\n");
	  get_xine_meta_info();
	  break;
  /* you can handle a lot of other interesting events here */
  }
}
/* get functions
 * ______________________________________________
 */
int get_xine_meta_info()
{
	int tags = 0;
	const char *author = xine_get_meta_info(stream, XINE_META_INFO_AUTHOR);
	printf("\nauthor = %s\n", author);
	const char *performer = xine_get_meta_info(stream, XINE_META_INFO_PERFORMER);
	printf("performer = %s\n", performer);
	const char *loc = xine_get_meta_info(stream, XINE_META_INFO_LOCATION);
	printf("location = %s\n", loc);
	const char *opus = xine_get_meta_info(stream, XINE_META_INFO_OPUS);
	printf("opus = %s\n\n", opus);

	const char *codec = xine_get_meta_info(stream, XINE_META_INFO_AUDIOCODEC);
	printf("codec = %s\n", codec);
	const char *layer = xine_get_meta_info(stream, XINE_META_INFO_SYSTEMLAYER);
	printf("layer = %s\n", layer);
	const char *plugin = xine_get_meta_info(stream, XINE_META_INFO_INPUT_PLUGIN);
	printf("plugin = %s\n", plugin);
	const char *discid = xine_get_meta_info(stream, XINE_META_INFO_CDINDEX_DISCID);
	printf("discid = %s\n", discid);
	const char *track = xine_get_meta_info(stream, XINE_META_INFO_TRACK_NUMBER);
	printf("track = %s\n", track);
	const char *composer = xine_get_meta_info(stream, XINE_META_INFO_COMPOSER);
	printf("composer = %s\n\n", composer);

	const char *genre = xine_get_meta_info(stream, XINE_META_INFO_GENRE);
	if (genre != NULL)
		tags++;
	const char *tag = xine_get_meta_info(stream, XINE_META_INFO_TITLE);
	char *temp, *artist, *title;
	if (tag != NULL)
	{
		printf("tag = %s\n", tag);
		artist = malloc(strlen(tag) + 1);
		memcpy(artist, tag, strlen(tag) + 1);
		if ((temp = strchr(artist, '-')) != NULL)
		{
			temp--;
			if (*temp == ' ')
			{
				*temp = '\0';
				temp += 3;
			}
			else
			{
				temp++;
				*temp = '\0';
				temp++;
			}
			title = malloc(strlen(temp) + 1);
			memcpy(title, temp, strlen(temp) + 1);
			tags += 2;
		}
		else
		{
			title = NULL;
			tags++;
		}
	}
	const char *comment = xine_get_meta_info(stream, XINE_META_INFO_COMMENT);
	if (comment != NULL)
		tags++;
	uint32_t bitrate = 0;
	//while (bitrate == 0) FIXME: xine_get_stream_info() stopped returning for some reason - not my fault
	//	bitrate = xine_get_stream_info (stream, XINE_STREAM_INFO_AUDIO_BITRATE);
	bitrate /= 1000;
	printf("bitrate = %d\n", bitrate);
	const char *album = xine_get_meta_info(stream, XINE_META_INFO_ALBUM);//actually contains the stream name and description;
	if (album == NULL)
	{
		printf("NULL ALBUM\n\n");//don't expect this to happen
		return -1;
	}
	printf("album = %s\n", album);
	//parse the album string
	char name[strlen(album) + 1];
	memcpy(name, album, strlen(album) + 1);
	temp = NULL;
	char *desc = NULL;//stream description
	if ((temp = strchr(name, '-')) != NULL)
	{
		temp--;
		if (*temp == ' ')
		{
			*temp = '\0';
			temp += 3;
		}
		else
		{
			temp++;
			*temp = '\0';
			temp++;
		}
		desc = malloc(strlen(temp) + 1);
		memcpy(desc, temp, strlen(temp) + 1);
	}
	else if ((temp = strchr(name, ':')) != NULL)
	{
		*temp = '\0';
		temp++;
		if (*temp == ' ')
			temp++;
		desc = malloc(strlen(temp) + 1);
		memcpy(desc, temp, strlen(temp) + 1);
	}
	else if ((temp = strchr(name, '(')) != NULL)
	{
		//nothing interesting to break apart so just strip anything in parens off the end for the name and copy the entire thing into the description
		desc = malloc(strlen(album) + 1);
		memcpy(desc, album, strlen(album) + 1);
		temp--;
		*temp = '\0';
	}
	else//really nothing interesting in there
	{
		desc = malloc(strlen(album) + 1);
		memcpy(desc, album, strlen(album) + 1);
	}
	printf("ARTIST = %s\nTITLE = %s\nGENRE = %s\n", artist, title, genre);
	printf("NAME = %s\nDESC = %s\nCOMMENT = %s\nBITRATE = %d\n", name, desc, comment, bitrate);
	tags += 3;//name, desc and bitrate are assured to be non-null
	char com2[1024];
	memset(com2, 0, 1024);
	memcpy(com2, "T", 1);
	char tags_s[2];
	//itoa(tags, tags_s);
	snprintf(tags_s, 2, "%d", tags);
	tags_s[1] = '\0';
	memcpy(&com2[1], tags_s, 2);
	char *pcom2 = &com2[3];
	int len2 = 3;
	if (genre != NULL)
	{
		memcpy(pcom2, "genre\0", 6);
		pcom2 += 6;
		memcpy(pcom2, genre, strlen(genre) + 1);
		pcom2 += strlen(genre) + 1;
		len2 += strlen(genre) + 7;
	}

	if (artist != NULL)
	{
		memcpy(pcom2, "artist\0", 7);
		pcom2 += 7;
		memcpy(pcom2, artist, strlen(artist) + 1);
		pcom2 += strlen(artist) + 1;
		len2 += strlen(artist) + 8;
		stream_history[streamcount].artist = malloc(strlen(artist) + 1);
		memcpy(stream_history[streamcount].artist, artist, strlen(artist) + 1);
		free(artist);
	}
	else
		stream_history[streamcount].artist = NULL;
	if (title != NULL)
	{
		memcpy(pcom2, "title\0", 6);
		pcom2 += 6;
		memcpy(pcom2, title, strlen(title) + 1);
		pcom2 += strlen(title) + 1;
		len2 += strlen(title) + 7;
		stream_history[streamcount].title = malloc(strlen(title) + 1);
		memcpy(stream_history[streamcount].title, title, strlen(title) + 1);
		free(title);
	}
	else
		stream_history[streamcount].title = NULL;
	//add the name to com2
	memcpy(pcom2, "Stream name\0", 12);
	pcom2 += 12;
	memcpy(pcom2, name, strlen(name) + 1);
	pcom2 += strlen(name) + 1;
	len2 += strlen(name) + 13;
	stream_history[streamcount].stream = malloc(strlen(name) + 1);
	memcpy(stream_history[streamcount].stream, name, strlen(name) + 1);
	//add the desc to com2
	if(desc != NULL)
	{
		memcpy(pcom2, "Description\0", 12);
		pcom2 += 12;
		memcpy(pcom2, desc, strlen(desc) + 1);
		pcom2 += strlen(desc) + 1;
		len2 += strlen(desc) + 13;
	}
	//add the comment if it exists
	if (comment != NULL)
	{
		memcpy(pcom2, "Comment\0", 8);
		pcom2 += 8;
		memcpy(pcom2, comment, strlen(comment) + 1);
		pcom2 += strlen(comment) + 1;
		len2 += strlen(comment) + 9;
	}
	//add the bitrate as a string with units
	char bitrate_s[9];
	memset(bitrate_s, 0, 9);
	//itoa(bitrate, bitrate_s);
	snprintf(bitrate_s, 9, "%d", bitrate);
	strcat(bitrate_s, " kbps\0");
	memcpy(pcom2, "Bitrate\0", 8);
	pcom2 += 8;
	memcpy(pcom2, bitrate_s, strlen(bitrate_s) + 1);
	len2 += strlen(bitrate_s) + 9;
	send_command(com2, len2);
	//have to do this again because com2 wipes out all the entry fields
	char com[256];
	memset(com, 0, 256);
	if (genre != NULL)
	{
		memcpy(com, "NG", 2);
		memcpy(&com[2], genre, strlen(genre) + 1);
		send_command(com, strlen(genre) + 3);
	}
	else
		send_command("NG\0", 3);
	//same for bitrate
	memset(com, 0, 256);
	memcpy(com, "NB", 2);
	memcpy(&com[2], bitrate_s, strlen(bitrate_s) + 1);
	send_command(com, strlen(bitrate_s) + 3);
	//and name
	memset(com, 0, 256);
	memcpy(com, "NN", 2);
	memcpy(&com[2], name, strlen(name) + 1);
	send_command(com, strlen(name) + 3);

	stream_history[streamcount].time = time(NULL);
	streamcount++;
	send_stream_history(0);
	printf("streamcount = %d\n", streamcount);
	return 0;
}
void get_new_stream_data(char *url)
{
	stop();
	//check if it's a pls and deal with it
	char *mrl;
	mrl = malloc(65536);
	memset(mrl, 0, 65536);
	pls_check(url, mrl);
	printf("get new stream data\n");
	printf("MRL = %s\n", mrl);
	start_xine();
	if (!xine_open(stream, mrl))
	{
		printf("!!!FAIL opening %s\n", url);
		error("can't play");
	}
	else
	{
		if (!xine_play(stream, 0, 0))
		{
			printf("opened but !!!FAIL playing stream %s\n", url);
			error("can't play");
			return;
		}
		else
			printf("getting metadata\n");
		streaming = 1;
		const char *genre = xine_get_meta_info(stream, XINE_META_INFO_GENRE);
		const char *album = xine_get_meta_info(stream, XINE_META_INFO_ALBUM);//actually contains the stream name and description
		uint32_t bitrate = 0;
		int count = 0;
		while (bitrate == 0 && count < 10000) {
			bitrate = xine_get_stream_info (stream, XINE_STREAM_INFO_AUDIO_BITRATE);
			count++;
		}
		if(bitrate == 0)
			printf("Couldn't determine bitrate\n");
		bitrate /= 1000;
		if (album == NULL)
		{
			printf("NULL ALBUM\n\n");//don't expect this to happen
			return;//bail out - nothing there
		}
		//parse the album string
		char name[strlen(album) + 1];
		memcpy(name, album, strlen(album) + 1);
		char *temp;
		char *desc = NULL;//stream description
		if ((temp = strchr(name, '-')) != NULL)
		{
			temp--;
			if (*temp == ' ')
			{
				*temp = '\0';
				temp += 3;
			}
			else
			{
				temp++;
				*temp = '\0';
				temp++;
			}
			desc = malloc(strlen(temp) + 1);
			memcpy(desc, temp, strlen(temp) + 1);
		}
		else if ((temp = strchr(name, ':')) != NULL)
		{
			*temp = '\0';
			temp++;
			desc = malloc(strlen(temp) + 1);
			memcpy(desc, temp, strlen(temp) + 1);
		}
		else if ((temp = strchr(name, '(')) != NULL)
		{
			//nothing interesting to break apart so just strip anything in parens off the end for the name and copy the entire thing into the description
			desc = malloc(strlen(album) + 1);
			memcpy(desc, album, strlen(album) + 1);
			temp--;
			*temp = '\0';
		}
		else//really nothing interesting in there
		{
			desc = malloc(strlen(album) + 1);
			memcpy(desc, album, strlen(album) + 1);
		}
		char com[2048];
		memset(com, 0, 2048);
		memcpy(com, "O", 1);
		char *pcom = &com[1];
		int len = 1;
		memcpy(pcom, name, strlen(name) + 1);
		pcom += strlen(name) + 1;
		len += strlen(name) + 1;
		if (genre != NULL)
		{
			memcpy(pcom, genre, strlen(genre) + 1);
			pcom += strlen(genre) + 1;
			len += strlen(genre) + 1;
		}
		else
		{
			memcpy(pcom, "\0", 1);
			pcom += 1;
			len += 1;
		}
		memcpy(pcom, desc, strlen(desc) + 1);
		pcom += strlen(desc) + 1;
		len += strlen(desc) + 1;
		char bitrate_s[4];
		//itoa(bitrate, bitrate_s);
		int ret = snprintf(bitrate_s, 4, "%d", bitrate);
		if(ret >= 4)
			printf("truncated print in get_new_stream_data");
		memcpy(pcom, bitrate_s, strlen(bitrate_s) + 1);
		len += strlen(bitrate_s) + 1;
		send_command(com, len);
		if (desc != NULL)
			free(desc);
	}
}
void print_current_pos()
{
	int pos;
	xine_get_pos_length(stream, &pos, NULL, NULL);
	//printf("%d\n", pos);
}
int get_volume()
{
	if (xine_running == 0)
		start_xine();
	int vol = xine_get_param(stream, XINE_PARAM_AUDIO_VOLUME);
	//printf("VOL = %d\n", vol);
	return vol;
}
void send_volume()
{
	int vol = get_volume();
	char v[4];
	//itoa(vol, v);
	snprintf(v, 4, "%d", vol);
	char com[strlen(v) + 2];
	if(strlen(v) > 3)
		printf("LARGE V = %d\n", vol);
	memset(com, 0, strlen(v) + 2);
	com[0] = 'V';
	strncat(com, v, strlen(v));
	send_command(com, strlen(v) + 2);
}
void get_stream_history()
{
	//printf("stream history\n");
	int i;
	for (i = streamcount - 1; i >= 0; i--)
		send_stream_history(i);
}
void send_stream_history(int i)
{
	char com[2048];
	memset(com, 0, 2048);
	com[0] = 'J';
	char *pcom = &com[1];
	int len = 1;

	char num[6];
	memset(num, 0, 6);
	//itoa(i, num);
	snprintf(num, 6, "%d", i);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;

	i = streamcount - i - 1;

	char time[12];
	memset(time, 0, 12);
	//itoa(stream_history[i].time, time);
	snprintf(time, 12, "%d", stream_history[i].time);
	memcpy(pcom, time, strlen(time) + 1);
	pcom += strlen(time) + 1;
	len += strlen(time) + 1;

	if (stream_history[i].artist != NULL)
	{
		memcpy(pcom, stream_history[i].artist, strlen(stream_history[i].artist) + 1);
		pcom += strlen(stream_history[i].artist) + 1;
		len += strlen(stream_history[i].artist) + 1;
	}
	else
	{
		memcpy(pcom, "\0", 1);
		pcom++;
		len++;
	}
	if (stream_history[i].title != NULL)
	{
		memcpy(pcom, stream_history[i].title, strlen(stream_history[i].title) + 1);
		pcom += strlen(stream_history[i].title) + 1;
		len += strlen(stream_history[i].title) + 1;
	}
	else
	{
		memcpy(pcom, "\0", 1);
		pcom++;
		len++;
	}
	memcpy(pcom, stream_history[i].stream, strlen(stream_history[i].stream) + 1);
	len += strlen(stream_history[i].stream) + 1;
	print_data(com, len);
	send_command(com, len);
}
/* update functions
 * ----------------------------------------------------------------------------------------
 */
void myxine_init() {
	stream_history = malloc(sizeof(stream_track)*10000);
	queue_init();
}
//sends the length of the song in milliseconds
void send_time()
{
	long playing_seconds = (long) playing_time();
	char len[10];
	snprintf(len, 10, "%li", playing_seconds * 1000);
	char com[12];
	memset(com, 0, 12);
	com[0] = 'M';//time data
	memcpy(&com[1], len, strlen(len));
	com[strlen(len) + 2] = 0;
	send_command(com, strlen(len) + 2);
	print_data(com, strlen(len) + 2);
}
int update_gui()
{
	if (is_running())
	{
		//printf("update gui\n");
		get_tag_info('T', cursong);
		send_weight_sticky_data(weight, sticky);
		update_image();
		send_playing(cursong);
		send_time();
		send_remaining();
		highlight_playlist();
	}
	else if (streaming)
	{
		get_xine_meta_info();
	}
	else
		return 1;

	return 0;
}
int update_progressbar()
{
	if (! is_running())
		return 1;
	if(play_file_finished())
	{
		free_cursong();
		while(play(NULL))
			;
		return 0;
	}
	else {
		float percent = progress();
		int permil = percent * 1000;
		char p[5] = {0};
		snprintf(p, 4, "%d", permil);
		char com[6];
		com[0] = 'B';//update progressBar
		char *pcom = &com[1];
		memcpy(pcom, p, strlen(p));
		pcom += strlen(p);
		memcpy(pcom, "\0", 1);
		//printf("com = %s\tp = %s\n", com, p);
		send_command(com, strlen(p) + 2);
	}
	return 0;
}
int set_volume(int vol)
{
	if (xine_running == 0)
		start_xine();
	xine_set_param(stream, XINE_PARAM_AUDIO_VOLUME, vol);
	return 0;
}
/* control state funcions
 * -----------------------------------------------------------------------------
 */
int play(char* song)
{
	if(xine_running == 0)
	{
		start_xine();
	}

	if (is_running() == 0 && ! (cursong == NULL))//pretty sure this never happens
	{
		printf("play current song\n");
	}
	else if (song != NULL)
	{
		printf("play song %s\n", song);
		set_cursong(song);
		ao_next();
	}
	else if (get_backcount() > 0)
	{
		printf("play back\n");
		set_cursong_from_back();
	}
	else if (get_qcount() > 0)
	{
		highlight_playlist();
		set_cursong_from_qpointer();
	}
	else
	{
		generate_playlist();
		if(set_cursong_from_playnow())
			return 1;
	}
	running = 1;
	add_to_history(cursong);
	play_cursong();
//	get_xine_meta_info();
	update_gui();
	return 0;
}
void play_cursong()
{
	//sndfile doesn't support mpc, m4a or mwa so skip them
	//play_file returns -1 if creating the thread fails
	if(check_file(cursong) == 3 || check_file(cursong) == 4 || check_file(cursong) == 6 || play_file(cursong))
	{
		next();
	}
	else
	{
		running = 1;
	}
	/*else if (!xine_open(stream, cursong) || !xine_play(stream, 0, 0))
	{
		error("can't play song");
		running = 0;
		//try again
		next();
		return;
	}*/
}
int stop()
{
	ao_stop();
	running = 0;
	streaming = 0;

	return 0;
}
int next()
{
	running = 0;
	ao_next();
	free_cursong();
	while(play(NULL))
		;
	return 0;
}
int back()
{
	push_cursong_to_back();
	running = 1;
	ao_next();
	play(NULL);
	return 0;
}
int pausetoggle()
{
	if(! running)
		return 0;
	ao_pause();
	/*
	if (xine_get_param(stream, XINE_PARAM_SPEED) != XINE_SPEED_PAUSE)
		xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
	else
		xine_set_param(stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
	*/
	return 0;
}
int jump_to_pos(int pos)
{
	if (xine_running == 0)
		start_xine();
	if (!xine_play(stream, pos, 0))
	{
		error("can't play stream");
		return -1;
	}
	return 0;
}
int is_running() {
	return running;
}
void play_stream_now(char *url)
{
	stop();
	free_cursong();
	char *mrl;
	mrl = malloc(65536);
	memset(mrl, 0, 65536);
	pls_check(url, mrl);
	printf("MRL = %s\n", mrl);
	start_xine();
	if (!xine_open(stream, mrl) || !xine_play(stream, 0, 0))
	{
		printf("!!!FAIL opening %s\n", mrl);
		error("can't play");
	}
	else
	{
		streaming = 1;
		get_xine_meta_info();
	}
}
void start_xine()
{
	if (xine_running == 0)
	{
		char configfile[2048];
		char *ao_driver = "auto";

		printf("initializing xine...");
		xine = xine_new();
		snprintf(configfile, sizeof(configfile), "%s%s", xine_get_homedir(), "/.xine/config");
		xine_config_load(xine, configfile);
		xine_init(xine);
		ao_port = xine_open_audio_driver(xine , ao_driver, NULL);
		stream = xine_stream_new(xine, ao_port, NULL);
		event_queue = xine_event_new_queue(stream);
		xine_event_create_listener_thread(event_queue, event_listener, NULL);
		xine_running = 1;
		printf("DONE\n");
	}
}
void stop_xine()
{
	if (xine_running == 1)
	{
		printf("stopping xine...");
		xine_close(stream);
		xine_event_dispose_queue(event_queue);
		xine_dispose(stream);
		if(ao_port)
			xine_close_audio_driver(xine, ao_port);
		xine_exit(xine);
		xine_running = 0;
		printf("stopped\n");
	}
}
