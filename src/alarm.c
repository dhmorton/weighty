/*
 * alarm.c
 *
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

#include "alarm.h"

#include "myutils.h"
#include "myxine.h"
#include "player.h"

static int waking = 0;
struct alarm_time wake;

static void stop_alarm(void);

void check_alarm()
{
	if (waking == 0)
	{
		//check the day of the week
		struct tm *now;
		time_t t = time(NULL);
		now = localtime(&t);
		char s[3];
		strftime(s, sizeof(s), "%w", now);
		int day = atoi(s);
		day = (day + 6) % 7;
		if (wake.set[day])
		{
			//check the hour
			char h[4];
			strftime(h, sizeof(h), "%H", now);
			int hour = atoi(h);
			if (wake.hr[day] == hour)
			{
				//check the minute
				char m[4];
				strftime(m, sizeof(m), "%M", now);
				int min = atoi(m);
				if (wake.min[day] == min)
				{
					waking = time(NULL);
					set_volume(wake.start_vol);
					play(wake.file);
					send_volume();
				}
			}
		}
	}
	else
	{
		time_t now = time(NULL);
		if ((now - waking) >= wake.fade)
		{
			set_volume(wake.end_vol);
			send_volume();
			waking = 0;
		}
		else
		{
			int vol;
			if (wake.fade != 0)
				vol = ((now - waking) * (wake.end_vol - wake.start_vol)) / wake.fade + wake.start_vol;
			else
				vol = wake.end_vol;
			set_volume(vol);
			send_volume();
			if (is_running() == 0)
				waking = 0;
		}
	}
}
void stop_alarm()
{
	waking = 0;
}
void set_alarm(int day, int set, int hr, int min)
{
	wake.set[day] = set;
	wake.hr[day] = hr;
	wake.min[day] = min;
}
void set_alarm_settings(int start_vol, int end_vol, int fade, char *song)
{
	if (wake.file != NULL)
		free(wake.file);
	wake.file = malloc(1024);
	memset(wake.file, 0, 1024);
	wake.start_vol = start_vol;
	wake.end_vol = end_vol;
	wake.fade = fade;
	if (song == NULL)
		wake.file = NULL;
	else
		memcpy(wake.file, song, strlen(song) + 1);
}
int read_alarm_config()
{
	FILE *fp;
	if ((fp = fopen(alarmconfig, "r")) == NULL)
		printf("config file doesn't exist\n");
	else
	{
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		int i;
		for(i = 0; i < 7; i++)
		{
			if ((read = getline(&line, &len, fp)) != -1)
				if (atoi(line) != i)
					printf("bad parsing!!!\n");
			if ((read = getline(&line, &len, fp)) != -1)
				wake.set[i] = atoi(line);
			if ((read = getline(&line, &len, fp)) != -1)
				wake.hr[i] = atoi(line);
			if ((read = getline(&line, &len, fp)) != -1)
				wake.min[i] = atoi(line);
		}
		if ((read = getline(&line, &len, fp)) != -1)
			wake.start_vol = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
			wake.end_vol = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
			wake.fade = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
		{
			wake.file = malloc(strlen(line) + 1);
			memcpy(wake.file, line, strlen(line) + 1);
		}
		if (line)
			free(line);
		fclose(fp);
	}
	return 0;
}
