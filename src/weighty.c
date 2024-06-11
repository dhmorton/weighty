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

int main(void)
{
	init_files();
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
	//initialize stuff from the config files
	connect_to_database(1);
	read_config();
	read_alarm_config();
	read_sleep_config();
	//start up xine
	initialize();
	struct sigaction sigact;
		int ret;
		memset(&sigact, 0, sizeof(sigact));
		sigact.sa_handler = SIG_IGN;
		sigact.sa_flags = SA_RESTART;
		ret = sigaction(SIGPIPE, &sigact, NULL);
		if(ret)
			error("SIGPIPE error");
	//if everything went ok then we can fork and let the parent die
	int sock = create_new_socket();
	//start the bluetooth thread
	bt_init();
	daemonize();
	//hit the main server loop
	weighty(sock);
	return 0;
}
int daemonize()
{
	printf("daemonizing\n");
	//daemon(0, 1);
	//TODO
	//open stderr to errorlog
	freopen(errorlog, "w", stderr);
	//open stdout to out.log
	//open stdin to /dev/null

	return 0;
}
