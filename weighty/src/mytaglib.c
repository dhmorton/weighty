/*
 * mytaglib.c
 *
 *  Created on: Jul 6, 2011
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

#include "mytaglib.h"
#include "myutils.h"
#include "sqlite.h"

int get_generic_tags(const char *file, int rowid)
{
	TagLib_File *tlfile;
	TagLib_Tag *tltag;

	if (check_file(file) == 6)
		tlfile = taglib_file_new_type(file, TagLib_File_ASF);
	else if (check_file(file) == 3)
		tlfile = taglib_file_new_type(file, TagLib_File_MPC);
	else
		tlfile = taglib_file_new(file);

	if (tlfile == NULL)
	{
		printf("failed to open %s\n", file);
		return -1;
	}
	if (!taglib_file_is_valid(tlfile))
	{
		printf("taglib file not valid\n");
		taglib_file_free(tlfile);
		return -1;
	}
	tltag = taglib_file_tag(tlfile);
	if (tltag == NULL)
	{
		printf("null taglib\n");
		taglib_file_free(tlfile);
		return -1;
	}

//	printf("getting title...\n");
	char *title = taglib_tag_title(tltag);
//	printf("\ttitle = %s\n", title);
	if (title != NULL)
		set_tag_data("TIT2", title, rowid);
	char *artist = taglib_tag_artist(tltag);
	if (artist != NULL)
		set_tag_data("TPE1", artist, rowid);
	char *album = taglib_tag_album(tltag);
	if (album != NULL)
		set_tag_data("TALB", album, rowid);
	char *comment = taglib_tag_comment(tltag);
	if (comment != NULL)
		set_tag_data("COMM", comment, rowid);
	char *genre = taglib_tag_genre(tltag);
	if (genre != NULL)
		set_tag_data("TCON", genre, rowid);
	unsigned int year = taglib_tag_year(tltag);
	if (year != 0)
	{
		char y[6];
		memset(y, 0, 6);
		//itoa(year, y);
		snprintf(y, 6, "%d", year);
		set_tag_data("TYER", y, rowid);
	}
	unsigned int track = taglib_tag_track(tltag);
	if (track != 0)
	{
		char t[6];
		memset(t, 0, 6);
		//itoa(track, t);
		snprintf(t, 6, "%d", track);
		set_tag_data("TRCK", t, rowid);
	}

	taglib_tag_free_strings();
	taglib_file_free(tlfile);

	return 0;
}
int write_taglib_tag(const char *file, char *field, char *tag)
{
	TagLib_File *tlfile;
	TagLib_Tag *tltag;

	tlfile = taglib_file_new(file);
	printf("%s\n", file);
	if (tlfile == NULL)
	{
		printf("failed to open %s\n", file);
		return -1;
	}
	if (!taglib_file_is_valid(tlfile))
	{
		printf("taglib file not valid\n");
		taglib_file_free(tlfile);
		return -1;
	}
	tltag = taglib_file_tag(tlfile);
	if (tltag == NULL)
	{
		printf("null taglib\n");
		taglib_file_free(tlfile);
		return -1;
	}

	if (strcmp(field, "TPE1") == 0)
	{
		update_tag_data(file, field, tag);
		if (tag != NULL)
			taglib_tag_set_artist(tltag, tag);
		else
			taglib_tag_set_artist(tltag, "");
	}
	else if (strcmp(field, "TALB") == 0)
	{
		update_tag_data(file, field, tag);
		if (tag != NULL)
			taglib_tag_set_album(tltag, tag);
		else
			taglib_tag_set_album(tltag, "");
	}
	else if (strcmp(field, "TCON") == 0)
	{
		update_tag_data(file, field, tag);
		if (tag != NULL)
			taglib_tag_set_genre(tltag, tag);
		else
			taglib_tag_set_genre(tltag, "");
	}
	else if (strcmp(field, "TIT2") == 0)
	{
		update_tag_data(file, field, tag);
		if (tag != NULL)
			taglib_tag_set_title(tltag, tag);
		else
			taglib_tag_set_title(tltag, "");
	}
	else if (strcmp(field, "COMM") == 0)
	{
		update_tag_data(file, field, tag);
		if (tag != NULL)
			taglib_tag_set_comment(tltag, tag);
		else
			taglib_tag_set_comment(tltag, "");
	}
	else if (strcmp(field, "TYER") == 0)
	{
		update_tag_data(file, field, tag);
		int year = 0;
		if (tag != NULL)
			year = atoi(tag);
		taglib_tag_set_year(tltag, year);
	}
	else if (strcmp(field, "TRCK") == 0)
	{
		update_tag_data(file, field, tag);
		int track = 0;
		if (tag != NULL)
			track = atoi(tag);
		taglib_tag_set_track(tltag, track);
	}
	taglib_tag_free_strings();
	taglib_file_free(tlfile);
	return 0;
}
int write_taglib_tags(const char* file, int num_tags, char** fields, char** tags)
{
	int i;
	for (i = 0; i < num_tags; i++) {
		write_taglib_tag(file, fields[i], tags[i]);
	}
	return 0;
}
