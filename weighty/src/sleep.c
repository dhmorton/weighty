/*
 * sleep.c
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

#include "sleep.h"

#include "myutils.h"
#include "myxine.h"

static int sleeping = 0;
static int start_vol;
static int start_time;

struct sleep_time sleep_fade;
int check_sleep()
{
	if (sleeping == 0)
		return 1;
	time_t now = time(NULL);
	if (now >= sleeping)//out of time
	{
		sleeping = 0;
		stop();
	}
	else
	{
		if ((now - start_time) > (sleep_fade.play * 60))
		{
			int vol = (sleeping - now) * start_vol / (sleep_fade.fade * 60);
			set_volume(vol);
			send_volume();
		}
	}
	return 0;
}
void set_sleep(int play, int fade)
{
	sleep_fade.play = play;
	sleep_fade.fade = fade;
}
void start_sleep()
{
	int total = (sleep_fade.play + sleep_fade.fade) * 60;
	time_t now = time(NULL);
	sleeping = now + total;//epoch time at which to stop
	start_vol = get_volume();
	start_time = now;
}
void stop_sleep()
{
	sleeping = 0;
}
int read_sleep_config()
{
	FILE *fp;
	if ((fp = fopen(sleepconfig, "r")) == NULL)
		printf("config file doesn't exist\n");
	else
	{
		char *line = NULL;
		size_t len = 0;
		ssize_t read;

		if ((read = getline(&line, &len, fp)) != -1)
			sleep_fade.play = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
			sleep_fade.fade = atoi(line);
		if (line)
			free(line);
	}
	fclose(fp);
	return 0;
}
