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

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

static int running = 0;

/* get functions
 * ______________________________________________
 */
int get_volume()
{
	//TODO: needs error checking
	long min, max, volume;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_get_playback_volume(elem, 0, &volume);

    snd_mixer_close(handle);
	int vol = 100 * (volume - min) / (max - min);
	return vol;
}
int set_volume(int volume)
{
	//TODO: needs error checking
	long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    long scaled_volume = volume * (max - min) / 100 + min;
    snd_mixer_selem_set_playback_volume_all(elem, scaled_volume);

    snd_mixer_close(handle);
}
void send_volume()
{
	int vol = get_volume();
	//TODO: seriously, abstract this
	char v[4];
	snprintf(v, 4, "%d", vol);
	char com[strlen(v) + 2];
	memset(com, 0, strlen(v) + 2);
	com[0] = 'V';
	strncat(com, v, strlen(v));
	send_command(com, strlen(v) + 2);
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
}
int update_gui()
{
	if (is_running())
	{
		get_tag_info('T', cursong);
		send_weight_sticky_data(weight, sticky);
		update_image();
		send_playing(cursong);
		send_time();
		send_remaining();
		highlight_playlist();
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

/* control state funcions
 * -----------------------------------------------------------------------------
 */
int play(char* song)
{
	if (is_running() == 0 && !(cursong == NULL))//pretty sure this never happens
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
	play_song();
	update_gui();
	return 0;
}
void play_song()
{
	//sndfile doesn't support mpc, m4a or mwa so skip them
	//play_file returns -1 if creating the thread fails
	if(check_file(cursong) == 3 || check_file(cursong) == 4 || check_file(cursong) == 6 || play_file(cursong)){
			next();	}
	else { 	running = 1; }
}
void play_cursong()
{
	cursong == NULL ? next() : play(cursong);
}
int stop()
{
	ao_stop();
	running = 0;

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
	return 0;
}
int jump_to_pos(int pos)
{
	return 0;
}
int is_running() {
	return running;
}