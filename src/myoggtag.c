/*
 * myoggtag.c
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

#include "myoggtag.h"

#include "mytaglib.h"
#include "sqlite.h"

int get_ogg_tags(const char *file, int rowid)
{
	OggVorbis_File vf;
	vorbis_comment *vc;
	FILE *stream;

	if ((stream = fopen(file, "r")) == NULL)
		return -1;
	if (ov_open(stream, &vf, NULL, 0) < 0)
	{
		fclose(stream);
		return -1;
	}
	vc = ov_comment(&vf, -1);
	int num_com = vc->comments;
	int i;
	for (i = 0; i < num_com; i++)
	{
		char *s;
		s = vc->user_comments[i];
		char field[256];//yeah it could be 2^32-1 but will it really?
		int j = 0;
		while (*s != '=')
			field[j++] = *s++;
		field[j] = '\0';
		s++;
		printf("\t%s\t%s\n", field, s);
		set_tag_data(field, (char*) s, rowid);
	}
	vorbis_comment_clear(vc);
	ov_clear(&vf);

	return 0;
}
int write_ogg_tags(char *file, int num_tags, char **fields, char **tags)
{
	//FIXME: cheated with the taglib library. Either use the ogg library or dump it and use taglib
	printf("write ogg tags\n");
	int i;
	for (i = 0; i < num_tags; i++)
	{
		printf("%s\t=>%s\n", fields[i], tags[i]);
		write_taglib_tag(file, fields[i], tags[i]);
	}
	return 0;
}
