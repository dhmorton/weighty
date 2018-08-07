/*
 * weighty.c
 *
 *  Created on: Aug 6, 2008
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
#include "weighty.h"

#include "alarm.h"
#include "btcomm.h"
#include "firsttime.h"
#include "list_utils.h"
#include "myutils.h"
#include "network.h"
#include "player.h"
#include "sleep.h"
#include "sqlite.h"

char *homedir = NULL;
char *errorlog = NULL;
char *alarmconfig = NULL;
char *sleepconfig = NULL;
char *streamconfig = NULL;
char *configfile = NULL;
char *database = NULL;

int main(void)
{
	homedir = malloc(256);
	memset(homedir, 0, 256);
	char *home = getenv("HOME");
	memcpy(homedir, home, strlen(home));
	strcat(homedir, "/.weighty-new");
	configfile = malloc(strlen(homedir) + 8);
	memset(configfile, 0, strlen(homedir) + 8);
	memcpy(configfile, homedir, strlen(homedir));
	strcat(configfile, "/config");
	errorlog = malloc(strlen(homedir) + 11);
	memset(errorlog, 0, strlen(homedir) + 11);
	memcpy(errorlog, homedir, strlen(homedir));
	strcat(errorlog, "/error.log");
	database = malloc(strlen(homedir) + 12);
	memset(database, 0, strlen(homedir) + 12);
	memcpy(database, homedir, strlen(homedir));
	strcat(database, "/weighty.db");
	alarmconfig = malloc(strlen(homedir) + 14);
	memset(alarmconfig, 0, strlen(homedir) + 14);
	memcpy(alarmconfig, homedir, strlen(homedir));
	strcat(alarmconfig, "/alarm.config");
	sleepconfig = malloc(strlen(homedir) + 14);
	memset(sleepconfig, 0, strlen(homedir) + 14);
	memcpy(sleepconfig, homedir, strlen(homedir));
	strcat(sleepconfig, "/sleep.config");
	streamconfig = malloc(strlen(homedir) + 15);
	memset(streamconfig, 0, strlen(homedir) + 15);
	memcpy(streamconfig, homedir, strlen(homedir));
	strcat(streamconfig, "/stream.config");
	struct stat sbuf;
	//first check to see the home directory exists
	if (stat(homedir, &sbuf) == -1)
		printf("lstat failed on %s\n", homedir);
	if (! (S_ISDIR(sbuf.st_mode)))//the directory exists
	{
		if (mkdir(homedir, S_IRWXU))
			printf("mkdir failed\n");
		else
			get_musicdir();
	}
	connect_to_database(1);
	//read the config file and set the saved values
	read_config();
	read_alarm_config();
	read_sleep_config();
	//start up xine
	initialize();
	//if everything went ok then we can fork and let the parent die
	int sock = create_new_socket();
	//start the bluetooth thread
	bt_init();
	//daemonize();
	//hit the main server loop
	weighty(sock);
	return 0;
}
int daemonize()
{
	printf("daemonizing...");
	daemon(0, 1);
	printf("DONE\n");
	//open stderr to errorlog
	//open stdout to out.log
	//open stdin to /dev/null

	return 0;
}
