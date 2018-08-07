/*
 * mymp4tag.c
 *
*  Copyright: 2013-2017 David Morton
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

#include "mymp4tag.h"

#include "myutils.h"
#include "sqlite.h"

int get_mp4_tags(const char* file, int rowid)
{
	const MP4Tags *data = MP4TagsAlloc();
	MP4FileHandle mp4 = MP4Read(file);
	if (mp4 == MP4_INVALID_FILE_HANDLE)
		return -1;
	MP4TagsFetch(data, mp4);
	if (data->name != NULL) {
		printf("name = %s\n", data->name);
		set_tag_data("TIT2", (char*) data->name, rowid);
	}
	else if (data->artist != NULL) {
		printf("artist = %s\n", data->artist);
		set_tag_data("TPE1", (char*) data->artist, rowid);
	}
	else if (data->albumArtist != NULL) {
		printf("albumArtist = %s\n", data->albumArtist);
		set_tag_data("Album Artist", (char*) data->albumArtist, rowid);
	}
	else if (data->album != NULL) {
		printf("album = %s\n", data->album);
		set_tag_data("TALB", (char*) data->album, rowid);
	}
	else if (data->grouping != NULL) {
		printf("grouping = %s\n", data->grouping);
		set_tag_data("Grouping", (char*) data->grouping, rowid);
	}
	else if (data->composer != NULL) {
		printf("composer = %s\n", data->composer);
		set_tag_data("TCOM", (char*) data->composer, rowid);
	}
	else if (data->comments != NULL) {
		printf("comments = %s\n", data->comments);
		set_tag_data("COMM", (char*) data->comments, rowid);
	}
	else if (data->genre != NULL) {
		printf("genre = %s\n", data->genre);
		set_tag_data("TCON", (char*) data->genre, rowid);
	}
	else if (data->releaseDate != NULL) {
		printf("releaseDate = %s\n", data->releaseDate);
		set_tag_data("TYER", (char*) data->releaseDate, rowid);
	}
	else if (data->description != NULL) {
		printf("description = %s\n", data->description);
		set_tag_data("Description", (char*) data->description, rowid);
	}
	else if (data->longDescription != NULL) {
		printf("longDescription = %s\n", data->longDescription);
		set_tag_data("Long Description", (char*) data->longDescription, rowid);
	}
	else if (data->lyrics != NULL) {
		printf("lyrics = %s\n", data->lyrics);
		set_tag_data("Lyrics", (char*) data->lyrics, rowid);
	}
	else if (data->copyright != NULL) {
		printf("copyright = %s\n", data->copyright);
		set_tag_data("TCOP", (char*) data->copyright, rowid);
	}
	else if (data->encodingTool != NULL) {
		printf("encodingTool = %s\n", data->encodingTool);
		set_tag_data("Encoding Tool", (char*) data->encodingTool, rowid);
	}
	else if (data->encodedBy != NULL) {
		printf("encodedBy = %s\n", data->encodedBy);
		set_tag_data("TENC", (char*) data->encodedBy, rowid);
	}
	else if (data->purchaseDate != NULL) {
		printf("purchaseDate = %s\n", data->purchaseDate);
		set_tag_data("Purchase Date", (char*) data->purchaseDate, rowid);
	}
	else if (data->category != NULL) {
		printf("category = %s\n", data->category);
		set_tag_data("Category", (char*) data->category, rowid);
	}
	else if (data->iTunesAccount != NULL) {
		printf("iTunesAccount = %s\n", data->iTunesAccount);
		set_tag_data("iTunes Account", (char*) data->iTunesAccount, rowid);
	}
	else if (data->track != NULL) {
		const MP4TagTrack *track = data->track;
		printf("%d out of %d\n", track->index, track->total);
		char trackNum[4];
		itoa(track->index, trackNum);
		set_tag_data("TRCK", trackNum, rowid);
	}

	MP4Close(mp4, 0);
	MP4TagsFree(data);
	return 0;
}
int check_for_mp4_image(const char* file)
{
	const MP4Tags *data = MP4TagsAlloc();
	MP4FileHandle mp4 = MP4Read(file);
	if (mp4 == MP4_INVALID_FILE_HANDLE)
		return -1;
	MP4TagsFetch(data, mp4);
	if (data->artwork)
	{
		uint32_t count = data->artworkCount;
		printf("Found %d m4a images\n", count);
		uint32_t len = data->artwork->size;
		int type = data->artwork->type;
		printf("type = %d\n", type);

		FILE *fp;
		char *image_file = "/tmp/weighty-tag-album-art.jpg";
		if ((fp = fopen(image_file, "w")) != NULL) {
			fwrite(data->artwork->data, 1, len, fp);
			fclose(fp);
		}
	}
	else
		return -1;
	return 0;
}
int write_mp4_tag(const char* file, char* field, char* tag)
{
	const MP4Tags *data = MP4TagsAlloc();
	MP4FileHandle mp4 = MP4Read(file);
	if (mp4 == MP4_INVALID_FILE_HANDLE)
		return -1;
	if (strcmp(field, "TIT2")) {
		update_tag_data(file, field, tag);
		MP4TagsSetName(data, tag);
	}
	else if (strcmp(field, "TPE1")) {
		update_tag_data(file, field, tag);
		MP4TagsSetArtist(data, tag);
	}
	else if (strcasecmp(field, "albumArtist")) {
		update_tag_data(file, field, tag);
		MP4TagsSetAlbumArtist(data, tag);
	}
	else if (strcmp(field, "TALB")) {
		update_tag_data(file, field, tag);
		MP4TagsSetAlbum(data, tag);
	}
	else if (strcasecmp(field, "Grouping")) {
		update_tag_data(file, field, tag);
		MP4TagsSetGrouping(data, tag);
	}
	else if (strcmp(field, "TCOM")) {
		update_tag_data(file, field, tag);
		MP4TagsSetComposer(data, tag);
	}
	else if (strcmp(field, "COMM")) {
		update_tag_data(file, field, tag);
		MP4TagsSetComments(data, tag);
	}
	else if (strcmp(field, "TCON")) {
		update_tag_data(file, field, tag);
		MP4TagsSetGenre(data, tag);
	}
	else if (strcmp(field, "TYER")) {
		update_tag_data(file, field, tag);
		MP4TagsSetReleaseDate(data, tag);
	}
	else if (strcasecmp(field, "Description")) {
		update_tag_data(file, field, tag);
		MP4TagsSetDescription(data, tag);
	}
	else if (strcasecmp(field, "Long Description")) {
		update_tag_data(file, field, tag);
		MP4TagsSetLongDescription(data, tag);
	}
	else if (strcasecmp(field, "Lyrics")) {
		update_tag_data(file, field, tag);
		MP4TagsSetLyrics(data, tag);
	}
	else if (strcmp(field, "TCOP")) {
		update_tag_data(file, field, tag);
		MP4TagsSetCopyright(data, tag);
	}
	else if (strcasecmp(field, "Encoding Tool")) {
		update_tag_data(file, field, tag);
		MP4TagsSetEncodingTool(data, tag);
	}
	else if (strcmp(field, "TENC")) {
		update_tag_data(file, field, tag);
		MP4TagsSetEncodedBy(data, tag);
	}
	else if (strcasecmp(field, "Purchase Date")) {
		update_tag_data(file, field, tag);
		MP4TagsSetPurchaseDate(data, tag);
	}
	else if (strcasecmp(field, "Category")) {
		update_tag_data(file, field, tag);
		MP4TagsSetCategory(data, tag);
	}
	else if (strcmp(field, "TRCK")) {
		update_tag_data(file, field, tag);
		MP4TagTrack track;
		int t = 0;
		if (tag != NULL)
			t = atoi(tag);
		track.index = t;
		MP4TagsSetTrack(data, &track);
	}
	MP4Close(mp4, 0);
	MP4TagsFree(data);
	return 0;
}
int write_mp4_tags(const char* file, int num_tags, char** fields, char** tags)
{
	int i;
	for (i = 0; i < num_tags; i++) {
		write_mp4_tag(file, fields[i], tags[i]);
	}
	return 0;
}
