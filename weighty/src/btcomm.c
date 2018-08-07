/*
 * btcomm.c
 *
 *  Copyright 2015-2017 David Morton
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

#include "btcomm.h"
#include "myxine.h"
#include "network.h"
#include "player.h"

int client, connected;

static void *bluetooth_thread_start(void*);

void *bluetooth_thread_start(void *param)
{
	struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
	char buf[256] = { 0 };
	int s, bytes;
	socklen_t opt = sizeof(rem_addr);

	// allocate socket
	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	// bind socket to port 1 of the first available
	// local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = (uint8_t) 1;
	bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

	while(1)
	{
		// put socket into listening mode
		connected = 0;
		printf("\nBluetooth listening\n");
		listen(s, 1);

		// accept one connection
		client = accept(s, (struct sockaddr *)&rem_addr, &opt);
		//bail if there's no bluetooth
		if(client < 0)
			break;
		connected = 1;

		ba2str( &rem_addr.rc_bdaddr, buf );
		fprintf(stdout, "accepted connection from %s %d\n", buf, client);
		update_gui();
		memset(buf, 0, sizeof(buf));

		// read data from the client
		int recv = 0;
		char buf[1024];
		while ((bytes = read(client, buf, sizeof(buf))))
		{
			printf("BLUETOOTH reading\n");
			if (bytes > 0) {
				recv += (int) bytes;
				if(buf[0] == 'P')
					net_play();
				else if (buf[0] == 'N')
					next();
				else if (buf[0] == 'S')
					stop();
				else if (buf[0] == 'U')
					pausetoggle();
				else
					printf("Unknown command %s\n", buf);
			}
			else
				break;
		}
		printf("BLUETOOTH DONE READING\n");
		if (bytes == 0)
		{
			printf("socket closed: FD=%d\n", client);
			close(client);
			//close(s);
		}
	}
	return NULL;
}
void send_command_bt(char *com, int len)
{
	if(connected)
		write(client, com, len);
}
int bt_init()
{
	int *param = 0;
	pthread_t bluetooth_thread;
	printf("Starting thread\n");
	if(pthread_create(&bluetooth_thread, NULL, bluetooth_thread_start, &param))
	{
		fprintf(stderr, "Couldn't create thread\n");
		return 1;
	}

	/*if(pthread_join(bluetooth_thread, NULL))
	{
		fprintf(stderr, "Couldn't join thread\n");
		return 2;
	}*/

	return 0;
}

