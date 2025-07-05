/*
 * current_song.c
 *
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


#include "current_song.h"

#include "myflactag.h"
#include "myid3tag.h"
#include "mymp4tag.h"
#include "myutils.h"
#include "songhash.h"
#include "sqlite.h"

char *cursong = NULL;
int weight, sticky;

void set_cursong(char* song) {
	if(song != NULL)
	{
		cursong = malloc(strlen(song) + 1);
		memcpy(cursong, song, strlen(song) + 1);
		int a[2];
		retrieve_by_song(song, a);
		weight = a[0];
		sticky = a[1];
	}
}
void set_cursong_sticky(int s) {
	if(cursong == NULL)
		return;
	sticky = s;
	set_song_sticky(cursong, s);
}
void free_cursong() {
	if(cursong != NULL) {
		free(cursong);
		cursong = NULL;
	}
}
void update_weight(int w) {
	if(cursong == NULL)
		return;
	weight += w;
	if (weight > 100)
		weight = 100;
	else if (weight < 0)
		weight = 0;
	set_song_weight(cursong, weight);
}
int update_image()
{
	//printf("update_image()\n");
	char *image_file = "/tmp/weighty-tag-album-art.jpg";
	unlink(image_file);
	switch(check_file(cursong))
	{
	case 1:
		check_for_mp3_image(cursong);
		break;
	case 2:
		check_for_flac_image(cursong);
		break;
	case 4:
		check_for_mp4_image(cursong);
		break;
	default:
		break;
	}
	return 0;
}
