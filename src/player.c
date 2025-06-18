/*
 * player.c
 *
 *  Created on: Aug 12, 2008
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

#include "player.h"

#include "myutils.h"
#include "myxine.h"
#include "net_utils.h"
#include "songhash.h"
#include "sqlite.h"
#include "three_lists.h"

static int check_all_songs_on_disk(const char*);

int initialize()
{
	srandom(time(NULL));//seed random number generator
	three_lists_init();
	return 0;
}
void update_songs()
{
	printf("update songs\n");
	init_added();
	printf("check all songs on disk\n");
	int ret = check_all_songs_on_disk(val.musicdir);
	if (ret < 0)
		printf("musicdir = %s\n", val.musicdir);
	else
	{
		printf("check all songs in hash\n");
		check_all_songs_in_hash();
	}
}
int check_all_songs_on_disk(const char *file)
{
	begin_transaction();
	struct stat sbuf;
	if (stat(file, &sbuf) == -1)
	{
		printf("lstat failed on %s\n", file);
		return -1;
	}
	if (S_ISDIR(sbuf.st_mode))
	{
		DIR *ddir;
		struct dirent *pdir;
		ddir = opendir(file);
		if(ddir != NULL)
		{
			while ((pdir = readdir(ddir)) != NULL)
			{
				if (*(pdir->d_name) != '.')
				{
					char s[strlen(file) + strlen(pdir->d_name) + 2];//1 for the / and one for the \0
					strcpy(s, file);
					strcat(s, "/");
					strcat(s, pdir->d_name);
					s[strlen(file) + strlen(pdir->d_name) + 1] = '\0';
					check_all_songs_on_disk(s);
				}
			}
			closedir(ddir);
		}
	}
	else
	{
		check_song(file);//checks to see if it's in the hash and sets the appropriate flags
	}
	//close_database(1);
	//connect_to_database(1);
	end_transaction();
	close_database(0);
	connect_to_database(0);
	return 0;
}


