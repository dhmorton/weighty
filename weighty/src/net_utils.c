/*
 * net_utils.c
 *
 *  Created on: Dec 29, 2011
*  Copyright: 2011-2017 David Morton
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

#include "net_utils.h"

static fd_set socks_copy;
//struct counts stats;

void update_socks (int sock) {
	FD_SET(sock, &socks_copy);
}
void clear_sock(int sock) {
	FD_CLR(sock, &socks_copy);
}
void send_update(char adm, char* song)
{
	char com[BUFF];
	com[0] = 'U';//update
	com[1] = adm;//added, deleted or moved
	if (song != NULL)
	{
		char *pcom = &com[2];
		memcpy(pcom, song, strlen(song) + 1);
		send_command(com, strlen(song) + 3);
	}
	else
		send_command("UN", 2);
}
//sends the weight and sticky status of the currently loaded song
//FIXME this could be written so much better
int send_weight_sticky_data(int weight, int sticky)
{
	char w[4];
	char s[2];
	char com[8];
	memset(com, 0, 8);
	com[0] = 'W';
	char *pcom = &com[1];
	//itoa(weight, w);
	snprintf(w, 4, "%d", weight);
	int len = 1;
	if (weight == 100)
	{
		memcpy(pcom, w, 3);
		pcom += 3;
		len += 3;
	}
	else if (weight >= 10)
	{
		memcpy(pcom, w, 2);
		pcom += 2;
		len += 2;
	}
	else
	{
		memcpy(pcom, w, 1);
		pcom++;
		len++;
	}
	memcpy(pcom, "\0", 1);
	pcom++;
	len++;
	if(sticky == 0)
		s[0] = '0';
	else
		s[0] = '1';
	s[1] = '\0';
	memcpy(pcom++, s, 1);
	memcpy(pcom++, "\0", 1);
	len += 2;
	send_command(com, len);
	print_data(com, len);
	return 0;
}
int send_playing(char *song)
{
	char com[strlen(song) + 2];
	memset(com, 0, strlen(song) + 2);
	com[0] = 'Y';
	strncat(com, song, strlen(song) + 1);
	send_command(com, strlen(song) + 2);
	print_data(com, strlen(song) + 2);
	return 0;
}
int send_song_data(song* data, int songs, char pref)//pref is the prefix that tells the client what to do with the song list
{
	if(data == NULL)
		return -1;
	int i;
	for (i = 0; i < songs; i++)
	{
		char com[BUFF];
		memset(com, 0, BUFF);
		com[0] = pref;
		char *pcom = &com[1];
		char num[sizeof(int)*songs];
		//itoa(songs - i - 1, num);//send the number of songs left to parse with each song
		snprintf(num, sizeof(int)*songs, "%d", songs - i - 1);
		memcpy(pcom, num, strlen(num));
		pcom += strlen(num);
		memcpy(pcom++, "\0", 1);
		char sticky[3];
		snprintf(sticky, 2, "%d", data[i].sticky);
		//itoa(data[i].sticky, sticky);
		//snprintf(sticky, 1, "%d", data[i].sticky);
		memcpy(pcom++, sticky, 1);
		memcpy(pcom++, "\0", 1);
		char weight[4];
		//itoa(data[i].weight, weight);
		snprintf(weight, 4, "%d", data[i].weight);
		memcpy(pcom, weight, strlen(weight));
		pcom += strlen(weight);
		memcpy(pcom++, "\0", 1);
		memcpy(pcom, data[i].file, strlen(data[i].file));
		pcom += strlen(data[i].file);
		memcpy(pcom++, "\0", 1);
		int len = 1 + strlen(num) + 1 + 1 + 1 + strlen(weight) + 1 + strlen(data[i].file) + 1;
		send_command(com, len);
//		print_data(com, len);
//		free(data[i].file);//FIXME: who depends on this free here?
	}
	return 0;
}
void send_stats()
{
	char com[BUFF];
	memset(com, 0, BUFF);
	com[0] = 'S';
	int len = 1;
	char *pcom = &com[1];
	char num[10];
	memset(num, 0, 10);
	//itoa(stats.mp3, num);
	snprintf(num, 10, "%d", stats.mp3);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.ogg, num);
	snprintf(num, 10, "%d", stats.ogg);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.mpc, num);
	snprintf(num, 10, "%d", stats.mpc);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.wav, num);
	snprintf(num, 10, "%d", stats.wav);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.flac, num);
	snprintf(num, 10, "%d", stats.flac);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.m4a, num);
	snprintf(num, 10, "%d", stats.m4a);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.wma, num);
	snprintf(num, 10, "%d", stats.wma);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.other, num);
	snprintf(num, 10, "%d", stats.other);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.zero, num);
	snprintf(num, 10, "%d", stats.zero);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.hund, num);
	snprintf(num, 10, "%d", stats.hund);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.sticky, num);
	snprintf(num, 10, "%d", stats.sticky);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.total, num);
	snprintf(num, 10, "%d", stats.total);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.total, num);
	snprintf(num, 10, "%d", stats.albums);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.total, num);
	snprintf(num, 10, "%d", stats.artists);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	memset(num, 0, 10);
	//itoa(stats.total, num);
	snprintf(num, 10, "%d", stats.genres);
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	len += strlen(num) + 1;
	int i;
	for (i = 0; i <= 100; i++)
	{
		memset(num, 0, 10);
		//itoa(stats.count[i], num);
		snprintf(num, 10, "%d", stats.count[i]);
		memcpy(pcom, num, strlen(num) + 1);
		pcom += strlen(num) + 1;
		len += strlen(num) + 1;
	}
	send_command(com, len);
}
void send_command(char *com, int len)
{
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	/*if (com[0] != 'B')
		print_data(com, len);
		*/
	fd_set write_socks = socks_copy;
	select(FD_SETSIZE, NULL, &write_socks, NULL, &tv);
	int i;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (FD_ISSET(i, &write_socks))
		{
			//printf("Writing to socket %d\n", i);
			if (len > 49152)//3*2^14 is the max sent in one chunk apparantly
			{
				while (len > 49152)
				{
					send(i, com, 49152, 0);
					com += 49152;
					len -= 49152;
				}
			}
			if (len > 0)
			{
				//ssize_t ret = send(i, com, len, 0);
				ssize_t ret = write(i, com, len);
				//FIXME send should only run if there is a socket connection
				if(ret == -1) {
					error("write error");
					return;
				}
			}
		}
	}
}
