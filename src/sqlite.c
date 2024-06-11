/*
 * sqlite.c
 *
 *  Created on: Aug 7, 2008
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

#include "sqlite.h"

#include "btcomm.h"
#include "myflactag.h"
#include "myid3tag.h"
#include "mymp4tag.h"
#include "myoggtag.h"
#include "mytaglib.h"
#include "net_utils.h"
#include "playnow_list.h"
#include "songhash.h"
#include "three_lists.h"

sqlite3 *db;
struct counts stats;
int force_stay_connected = 0; //connected to database or not

static int set_file_data(const char*, int, int);
static int set_album_data(const char*, int, int);
static int set_artist_data(const char*, int, int);
static int set_genre_data(const char*, int, int);
static int send_tag_data(char**, int*, int);
static int check_tag(char*, char*);
static int check_tag_in_songs(char*, char*);
static int delete_album(char*);
static int delete_artist(char*);
static int delete_genre(char*);
static int search_by_field(char*, char*, song**);
static int search_all_tags(char*, song**);
static int search_full_path(char*, song**);
static void check_extension(char*);
static int cmpstringp(const void*, const void*);

/*
 * INITIALIZATION FUNCTIONS
 */

int create_database()
{
	sqlite3_stmt *stmt;
	char *pz_left;

	connect_to_database(0);
	char *sql = "CREATE TABLE files(id INTEGER PRIMARY KEY AUTOINCREMENT, file TEXT, weight INTEGER UNSIGNED, sticky INTEGER UNSIGNED, timeStamp INTEGER UNSIGNED)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE TABLE tags (id INTEGER UNSIGNED, field TEXT, tag TEXT)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE TABLE albums (album VARCHAR(249), weight INTEGER UNSIGNED, sticky INTEGER UNSIGNED, timeStamp INTEGER UNSIGNED)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE TABLE artists (artist VARCHAR(249), weight INTEGER UNSIGNED, sticky INTEGER UNSIGNED, timeStamp INTEGER UNSIGNED)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE TABLE genres (genre VARCHAR(249), weight INTEGER UNSIGNED, sticky INTEGER UNSIGNED, timeStamp INTEGER UNSIGNED)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE TABLE streams (usr TEXT, name TEXT, genre TEXT, website TEXT, bitrate TEXT)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE INDEX fileindex ON files (id, file, weight, timeStame)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sql = "CREATE INDEX tagindex ON tags (id, field)";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;

	sqlite3_finalize(stmt);
	//close_database(0);
	return 0;
}
//if stay_connected is nonzero then it skips any close_database() calls until close_database() is called with a nonzero arguement
int connect_to_database(int stay_connected)
{
	if (force_stay_connected == 1)
		return 1;
	if (stay_connected) {
		force_stay_connected = 1;
	}
	if (sqlite3_open(database, &db) != SQLITE_OK)
	{
		printf("FAILED TO OPEN DATABASE\n");
		printf("errmsg: %s\n", sqlite3_errmsg(db));
		return -1;
	}
	//sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	printf("\tOPENED DATABASE\n");
	return 0;
}
int close_database(int force_close)
{
	if (force_stay_connected == 1 && force_close == 0)
		return 1;
	//int ret = sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
	/*if (ret != SQLITE_OK) {
		printf("END TRANSACTION FAILED %d\n", ret);
		printf("%s\n", sqlite3_errmsg(db));
		return ret;
	}*/
	force_stay_connected = 0;
	printf("\tCLOSED DATABASE\n");
	return 0;
}
void begin_transaction()
{
	sqlite3_exec(db, "BEGIN_TRANSACTION", NULL, NULL, NULL);
}
void end_transaction()
{
	sqlite3_exec(db, "END_TRANSACTION", NULL, NULL, NULL);
}
/*
 * SET FUNCTIONS
 */
int add_new_file(const char *file)
{
	int rowid;
	int type = check_file(file);
	if (type >= 0)
	{
		printf("db_add %s\n", file);
		rowid = set_file_data(file, 100, 0);
		printf("%d %s\n", rowid, file);
		make_push_song_node((char*) file, 100, 0);
		if (rowid < 0)
		{
			printf("database error inserting %s\n", file);
			return -1;
		}
		switch(type)
		{
		case 0:
			get_ogg_tags(file, rowid);
			stats.ogg++;
			break;
		case 1:
			get_mp3_tags(file, rowid);
			stats.mp3++;
			break;
		case 2:
			get_flac_tags(file, rowid);
			stats.flac++;
			break;
		case 4:
			get_mp4_tags(file, rowid);
			stats.m4a++;
			break;
		default:
			get_generic_tags(file, rowid);
			stats.other++;
			break;
		}
		stats.total++;
		send_stats();
	}
	return 0;
}
int set_file_data(const char *file, int weight, int sticky)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "INSERT INTO files (file, weight, sticky, timeStamp) values(?, ?, ?, ?)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, file, strlen(file), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 2, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 3, sticky);
	if (bind != SQLITE_OK)
		return -2;
	int timeStamp = time(NULL);
	bind = sqlite3_bind_int(stmt, 4, timeStamp);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		printf("stepping\n");//don't know why it started spinning on some files
		break;
	}
	int rowid = sqlite3_last_insert_rowid(db);
	sqlite3_finalize(stmt);
	return rowid;
}
int set_tag_data(char *field, char *tag, unsigned int id)
{
	if ((tag == NULL) || (field == NULL))
		return -1;
	sqlite3_stmt *stmt;
	char *pz_left;

	char trfield[64];//the translated field to insert
	memset(trfield, 0, 64);
	if (strcasecmp(field, "artist") == 0)
		memcpy(trfield, "TPE1\0", 5);
	else if (strcasecmp(field, "album") == 0)
		memcpy(trfield, "TALB\0", 5);
	else if (strcasecmp(field, "genre") == 0)
		memcpy(trfield, "TCON\0", 5);
	else if (strcasecmp(field, "title") == 0)
		memcpy(trfield, "TIT2\0", 5);
	else if (strcasecmp(field, "comment") == 0 || strcasecmp(field, "comments") == 0)
		memcpy(trfield, "COMM\0", 5);
	else if (strcasecmp(field, "composer") == 0)
		memcpy(trfield, "TCOM\0", 5);
	else if (strcasecmp(field, "length") == 0)
		memcpy(trfield, "TLEN\0", 5);
	else if ((strcasecmp(field, "track number") == 0) || (strcasecmp(field, "tracknumber") == 0))
		memcpy(trfield, "TRCK\0", 5);
	else if (strcasecmp(field, "year") == 0)
		memcpy(trfield, "TYER\0", 5);
	else if (strcasecmp(field, "copyright") == 0)
		memcpy(trfield, "TCOP\0", 5);
	else if (strcasecmp(field, "Encoded by") == 0)
		memcpy(trfield, "TENC\0", 5);
	else
		memcpy(trfield, field, strlen(field) + 1);

//	printf("set tag data:%d %d %s %d %s\n", id, (int)strlen(trfield) ,trfield, (int)strlen(tag), tag);
	char *sql = "INSERT INTO tags (id, field, tag) values (?, ?, ?)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char**) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, id);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 2, trfield, strlen(trfield), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 3, tag, strlen(tag), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		printf("stepping\n");
		break;//why does it spin sometimes?
	}
	if (strcasecmp(trfield, "TALB") == 0)
	{
		if (check_tag("TALB", tag) == 0)
			set_album_data(tag, 100, 0);
	}
	else if (strcasecmp(trfield, "TPE1") == 0)
	{
		if (check_tag("TPE1", tag) == 0)
			set_artist_data(tag, 100, 0);
	}
	else if (strcasecmp(trfield, "TCON") == 0)
	{
		if (check_tag("TCON", tag) == 0)
			set_genre_data(tag, 100, 0);
	}
	sqlite3_finalize(stmt);
	return 0;
}
int delete_tag_data(const char *file, char *field)
{
	printf("DELETING TAG %s from %s\n", field, file);
	sqlite3_stmt *stmt;
	char *pz_left;

	char trfield[16];//the translated field to insert
	memset(trfield, 0, 16);
	if (strcasecmp(field, "artist") == 0)
		memcpy(trfield, "TPE1\0", 5);
	else if (strcasecmp(field, "album") == 0)
		memcpy(trfield, "TALB\0", 5);
	else if (strcasecmp(field, "genre") == 0)
		memcpy(trfield, "TCON\0", 5);
	else if (strcasecmp(field, "title") == 0)
		memcpy(trfield, "TIT2\0", 5);
	else if (strcasecmp(field, "comment") == 0 || strcasecmp(field, "comments") == 0)
		memcpy(trfield, "COMM\0", 5);
	else if (strcasecmp(field, "composer") == 0)
		memcpy(trfield, "TCOM\0", 5);
	else if (strcasecmp(field, "length") == 0)
		memcpy(trfield, "TLEN\0", 5);
	else if ((strcasecmp(field, "track number") == 0) || (strcasecmp(field, "tracknumber") == 0))
		memcpy(trfield, "TRCK\0", 5);
	else if ((strcasecmp(field, "year") == 0) || (strcasecmp(field, "date") == 0))
		memcpy(trfield, "TYER\0", 5);
	else if (strcasecmp(field, "copyright") == 0)
		memcpy(trfield, "TCOP\0", 5);
	else
	{
//		if (strlen(field)!= 4)
//			printf("can't translate %s\n", field);
		memcpy(trfield, field, strlen(field) + 1);
	}
	char *sql = "SELECT t.id FROM tags t INNER JOIN files f ON t.id = f.id WHERE f.file = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, file, strlen(file), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	int id;
	while (SQLITE_DONE != sqlite3_step(stmt))
		id = sqlite3_column_int(stmt, 0);

	printf("id %d %s\n", id, field);

	sql = "DELETE FROM tags WHERE id = ? AND field = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	bind = sqlite3_bind_int(stmt, 1, id);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 1, trfield, strlen(trfield), SQLITE_STATIC);
	if (bind != SQLITE_OK)
			return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	return 0;
}
int update_tag_data(const char *file, char *field, char *tag)
{
	printf("\tUPDATE TAG DATA %s %s %s\n", field, tag, file);
	sqlite3_stmt *stmt;
	char *pz_left;

	char *trfield = malloc(sizeof(char) * 5);
	back_translate(field, &trfield);
	//char *sql = "SELECT tags.field, tags.tag, files.id FROM files, tags WHERE files.file = ? AND files.id = tags.id";
	char *sql = "SELECT t.field, t.tag, f.id\
				FROM files f LEFT JOIN tags t ON f.id = t.id\
				WHERE f.file = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char**) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, file, strlen(file), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	char tag_field[64];
	char old_tag[1024];
	int tag_id = -1;
	unsigned int file_id = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		file_id = sqlite3_column_int(stmt, 2);
		const unsigned char* try_field = sqlite3_column_text(stmt, 0);
		if (try_field != NULL && !strcmp(trfield, (char*)try_field))
		{
			const unsigned char* t = sqlite3_column_text(stmt, 1);
			memcpy(old_tag, t, strlen((char*)t) + 1);
			memcpy(tag_field, try_field, strlen((char*)try_field) + 1);
			tag_id = file_id;
			break;
		}
	}
	printf("tag id = %d\n", tag_id);
	//song doesn't have the tag, have to add it to the tags database
	//also check if it needs to go in the relevant album/artist/genre database
	if (tag_id < 0 && tag != NULL)
	{
		printf("set tag data %s %s %d\n", trfield, tag, file_id);
		set_tag_data(trfield, tag, file_id);
		return 0;
	}
	//if the tag is deleted need to check if it's the last artist/album/genre left and update the other tables
	else if (tag_id > 0 && tag == NULL)
	{
		printf("DELETE tag %s\n", field);
		sql = "DELETE FROM tags WHERE field = ? AND id  = ?";
		prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char**) &pz_left);
		if (prepare != SQLITE_OK)
			return 1;
		bind = sqlite3_bind_text(stmt, 1, tag_field, strlen(tag_field), SQLITE_STATIC);
		if (bind != SQLITE_OK)
			return 2;
		bind = sqlite3_bind_int(stmt, 2, tag_id);
		if (bind != SQLITE_OK)
			return 2;
	}
	else if (tag_id > 0)
	{
		printf("UPDATE %s to %s\n", field, tag);
		sql = "UPDATE tags SET tag = ? WHERE field = ? AND id  = ?";
		prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char**) &pz_left);
		if (prepare != SQLITE_OK)
			return 1;
		bind = sqlite3_bind_text(stmt, 1, tag, strlen(tag), SQLITE_STATIC);
		if (bind != SQLITE_OK)
			return 2;
		bind = sqlite3_bind_text(stmt, 2, tag_field, strlen(tag_field), SQLITE_STATIC);
		if (bind != SQLITE_OK)
			return 2;
		bind = sqlite3_bind_int(stmt, 3, tag_id);
		if (bind != SQLITE_OK)
			return 2;
	}
	else//nothing need be done - tag didn't exist and still doesn't, weird
		return 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	//after deleting or changing the tag have to check if it's still in the database
	//if it was updated it might be new
	if (strcasecmp(tag_field, "TALB") == 0)
	{
		if (check_tag_in_songs("TALB", old_tag) == 0)
			delete_album(old_tag);
		if (tag != NULL && check_tag("TALB", tag) == 0)
			set_album_data(tag, 100, 0);
	}
	else if (strcasecmp(tag_field, "TPE1") == 0)
	{
		if (check_tag_in_songs("TPE1", old_tag) == 0)
			delete_artist(old_tag);
		if (tag != NULL && check_tag("TPE1", tag) == 0)
			set_artist_data(tag, 100, 0);
	}
	else if (strcasecmp(tag_field, "TCON") == 0)
	{
		if (check_tag_in_songs("TCON", old_tag) == 0)
		{
			delete_genre(old_tag);
			printf("delete genre %s\n", old_tag);
		}
		if (tag != NULL && check_tag("TCON", tag) == 0)
		{
			set_genre_data(tag, 100, 0);
			printf("add genre II %s\n", tag);
		}
	}
	sqlite3_finalize(stmt);
	return 0;
}
//database already opened in another function
int set_album_data(const char *album, int weight, int sticky)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "INSERT INTO albums (album, weight, sticky, timeStamp) values(?, ?, ?, ?)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, album, strlen(album), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 2, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 3, sticky);
	if (bind != SQLITE_OK)
		return -2;
	int timeStamp = time(NULL);
	bind = sqlite3_bind_int(stmt, 4, timeStamp);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	int rowid = sqlite3_last_insert_rowid(db);
	sqlite3_finalize(stmt);
	printf("added ALBUM %s\n", album);
	stats.albums++;
	send_stats();
	return rowid;
}
int set_artist_data(const char *artist, int weight, int sticky)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "INSERT INTO artists (artist, weight, sticky, timeStamp) values(?, ?, ?, ?)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, artist, strlen(artist), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 2, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 3, sticky);
	if (bind != SQLITE_OK)
		return -2;
	int timeStamp = time(NULL);
	bind = sqlite3_bind_int(stmt, 4, timeStamp);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	int rowid = sqlite3_last_insert_rowid(db);
	sqlite3_finalize(stmt);
	stats.artists++;
	send_stats();
	return rowid;
}
int set_genre_data(const char *genre, int weight, int sticky)
{
	if(genre == NULL || strlen(genre) == 0)
		return -1;
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "INSERT INTO genres (genre, weight, sticky, timeStamp) values(?, ?, ?, ?)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, genre, strlen(genre), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 2, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_int(stmt, 3, sticky);
	if (bind != SQLITE_OK)
		return -2;
	int timeStamp = time(NULL);
	bind = sqlite3_bind_int(stmt, 4, timeStamp);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	int rowid = sqlite3_last_insert_rowid(db);
	sqlite3_finalize(stmt);
	printf("ADDED GENRE %s\n", genre);
	stats.genres++;
	send_stats();
	return rowid;
}
int set_song_weight(char* song, int weight)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE files SET weight = ? WHERE file = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, song, strlen(song), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		printf("stepping\n");

	sqlite3_finalize(stmt);
	//change it in the struct too
	change_weight_hash(song, weight);
	return 0;
}
int set_album_weight(char* album, int weight)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE albums SET weight = ? WHERE album = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, album, strlen(album), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	return 0;
}
int set_artist_weight(char* artist, int weight)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	printf("setting %s to weight %d\n", artist, weight);
	char *sql = "UPDATE artists SET weight = ? WHERE artist = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_int(stmt, 1, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, artist, strlen(artist), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;

	sqlite3_finalize(stmt);
	return 0;
}
int set_genre_weight(char* genre, int weight)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE genres SET weight = ? WHERE genre = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, weight);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, genre, strlen(genre), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;

	sqlite3_finalize(stmt);
	return 0;
}
int set_song_sticky(char *song, int sticky)
{
	int prepare;
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE files SET sticky = ? WHERE file = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, sticky);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, song, strlen(song), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		printf("stepping\n");

	sqlite3_finalize(stmt);
	//change it in the struct too
	change_sticky_hash(song, sticky);
	return 0;
}
int set_album_sticky(char *album, int sticky)
{
	int prepare;
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE albums SET sticky = ? WHERE album = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, sticky);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, album, strlen(album), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		printf("stepping\n");

	sqlite3_clear_bindings(stmt);
	sqlite3_finalize(stmt);
	printf("\t%d set ALBUM = %s\n", sticky, album);
	return 0;
}
int set_artist_sticky(char *artist, int sticky)
{
	int prepare;
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE artists SET sticky = ? WHERE artist = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, sticky);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, artist, strlen(artist), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		printf("stepping\n");

	sqlite3_finalize(stmt);
	return 0;
}
int set_genre_sticky(char *genre, int sticky)
{
	int prepare;
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE genres SET sticky = ? WHERE genre = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, sticky);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, genre, strlen(genre), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		printf("stepping\n");

	sqlite3_finalize(stmt);
	return 0;
}
//get the id from the song first so all the tags can be deleted as well
//database opened and closed in songhash.c:check_all_songs_in_hash()
int delete_song_entry(const char *file)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT id FROM files WHERE file = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, file, strlen(file), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	int id;
	while (SQLITE_DONE != sqlite3_step(stmt))
		id = sqlite3_column_int(stmt, 0);

	sql = "DELETE FROM files WHERE id = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	bind = sqlite3_bind_int(stmt, 1, id);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;

	//check to see if any of the tags are the last one for album, artist or genre
	sql = "SELECT field, tag FROM tags WHERE id = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	bind = sqlite3_bind_int(stmt, 1, id);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char *field = sqlite3_column_text(stmt, 0);
		const unsigned char *tag = sqlite3_column_text(stmt, 1);
		if (strcasecmp((char*)field, "TALB") == 0)
		{
			if (check_tag("TALB", (char*)tag) == 1)
				delete_album((char*)tag);
		}
		else if (strcasecmp((char*)field, "TPE1") == 1)
		{
			if (check_tag("TPE1", (char*)tag) == 1)
				delete_artist((char*)tag);
		}
		else if (strcasecmp((char*)field, "TCON") == 0)
		{
			if (check_tag("TCON", (char*)tag) == 1)
				delete_genre((char*)tag);
		}
	}

	sql = "DELETE FROM tags WHERE id = ?";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	bind = sqlite3_bind_int(stmt, 1, id);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;

	sqlite3_finalize(stmt);
	stats.total--;
	send_stats();
	return 0;
}
//database opened and closed in songhash.c:check_all_songs_in_hash()
int update_song_entry(const char *oldfile, const char *newfile)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "UPDATE files SET file = ? WHERE file = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, newfile, strlen(newfile), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 2, oldfile, strlen(oldfile), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;

	sqlite3_finalize(stmt);
	return 0;
}
/*
 * GET FUNCTIONS
 */
int send_tag_data(char **tag_array, int *tag_len, int num_tags)
{
	//printf("\n\tTAG DATA\n\n");
	int k;
	for (k = 0; k < num_tags; k++) {
		char total[4];
		memset(total, 0, 4);
		//itoa(num_tags - k - 1, total);
		int ret = snprintf(total, 4, "%d", num_tags - k - 1);
		if(ret >= 4)
			printf("truncated snprintf in send_tag_data");
		char pref_com[5];
		pref_com[0] = 'G';
		memcpy(&pref_com[1], total, strlen(total) + 1);
		//print_data(&pref_com[0], strlen(total) + 2);
		send_command(pref_com, strlen(total) + 2);
		//print_data(tag_array[k], tag_len[k]);
		send_command(tag_array[k], tag_len[k]);
		free(tag_array[k]);
	}
	return 0;
}
//fills up data with only albums having tags that occur more than once, have to add the weight and sticky later
int get_multisong_albums(song **data)
{
	sqlite3_stmt *stmt;
	char *pz_left;
	printf("get multisong_albums\n");

	char *sql = "SELECT t.tag, COUNT (t.tag), a.album, a.weight, a.sticky\
				FROM albums a INNER JOIN tags t\
				ON a.album = t.tag\
				WHERE t.field = ?\
				GROUP BY t.tag HAVING (COUNT (t.tag) > 1)\
				ORDER BY a.album COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, "TALB", 4, SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* album = sqlite3_column_text(stmt, 2);
		(*data)[i].file = malloc(strlen((char*)album) + 1);
		memcpy((*data)[i].file, (char*)album, strlen((char*)album) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 3);
		(*data)[i].sticky = sqlite3_column_int(stmt, 4);
		//printf("%d : %d %s\n", (*data)[i].sticky, (*data)[i].weight, (*data)[i].file);
		i++;
	}
	sqlite3_clear_bindings(stmt);
	sqlite3_finalize(stmt);
	printf("%d albums\n", i);

	return i;
}
int get_msalbums_for_ll(struct gen_node** head)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT t.tag, COUNT (t.tag), a.album, a.weight, a.sticky\
				FROM albums a INNER JOIN tags t\
				ON a.album = t.tag\
				WHERE t.field = ?\
				GROUP BY t.tag HAVING (COUNT (t.tag) > 1)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, "TALB", 4, SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* album = sqlite3_column_text(stmt, 2);
		make_push_node(head, (char*) album, sqlite3_column_int(stmt, 3), sqlite3_column_int(stmt, 4));
		i++;
		stats.albums++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int get_artists_for_ll(struct gen_node** head)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT ALL artist, weight, sticky FROM artists";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* artist = sqlite3_column_text(stmt, 0);
		make_push_node(head,(char*) artist, sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2));
//		printf("artist = %s\n", artist);
		i++;
		stats.artists++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int get_artists(song **data)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT ALL artist, weight, sticky FROM artists ORDER BY artist COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* artist = sqlite3_column_text(stmt, 0);
		(*data)[i].file = malloc(strlen((char*)artist) + 1);
		memcpy((*data)[i].file, (char*)artist, strlen((char*)artist) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 1);
		(*data)[i].sticky = sqlite3_column_int(stmt, 2);
//		printf("artist = %s\n", artist);
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int get_genres_for_ll(struct gen_node** head)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT ALL genre, weight, sticky FROM genres";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* genre = sqlite3_column_text(stmt, 0);
		make_push_node(head,(char*) genre, sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2));
		if(sqlite3_column_int(stmt, 1) < 0)
		{
			printf("negative weight\n");
		}
		i++;
		stats.genres++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int get_genres(song **data)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT ALL genre, weight, sticky FROM genres ORDER BY genre COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* genre = sqlite3_column_text(stmt, 0);
		(*data)[i].file = malloc(strlen((char*)genre) + 1);
		memcpy((*data)[i].file, (char*)genre, strlen((char*)genre) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 1);
		(*data)[i].sticky = sqlite3_column_int(stmt, 2);
//		printf("genre = %s\n", genre);
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int check_tag(char *field, char *tag)
{
	if (tag == NULL)//why is this ever called when the tag is null?
		return 0;
	sqlite3_stmt *stmt;
	char *pz_left;

	printf("Looking for tag %s in %s\n", tag, field);
	char *sql = NULL;
	if(strcasecmp(field, "TALB") == 0)
	{
		sql = "SELECT album FROM albums WHERE album = ?";
	}
	else if(strcasecmp(field, "TPE1") == 0)
	{
		sql = "SELECT artist FROM artists WHERE artist = ?";
	}
	else if(strcasecmp(field, "TCON") == 0)
	{
		sql = "SELECT genre FROM genres WHERE genre = ?";
	}
	if(sql == NULL)
	{
		printf("NULL sql statement\n");
		return 0;
	}
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
	{
		printf("error preparing sql statement\n");
		printf("DBERROR %s\n", sqlite3_errmsg(db));
		return -1;
	}
	//int bind = sqlite3_bind_text(stmt, 1, field, strlen(field), SQLITE_STATIC);
	//if (bind != SQLITE_OK)
		//return -2;
	int bind = sqlite3_bind_text(stmt, 1, tag, strlen(tag), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		//const unsigned char* a = sqlite3_column_text(stmt, 0);
		i = 1;
	}
	sqlite3_finalize(stmt);
	return i;
}
int check_tag_in_songs(char *field, char *tag)
{
	if (tag == NULL)//why is this ever called when the tag is null?
		return 0;
	sqlite3_stmt *stmt;
	char *pz_left;

	printf("Looking for tag %s in %s\n", tag, field);
	char *sql = "SELECT tag FROM tags WHERE field = ? AND tag = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, field, strlen(field), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, tag, strlen(tag), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int delete_album(char *album)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "DELETE FROM albums WHERE album = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, album, strlen(album), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sqlite3_finalize(stmt);
	stats.albums--;
	send_stats();
	return 0;
}
int delete_artist(char *artist)
{
	printf("delete artist %s\n", artist);
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "DELETE FROM artists WHERE artist = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, artist, strlen(artist), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sqlite3_finalize(stmt);
	stats.artists--;
	send_stats();
	return 0;
}
int delete_genre(char *genre)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "DELETE FROM genres WHERE genre = ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, genre, strlen(genre), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	while (SQLITE_DONE != sqlite3_step(stmt))
		;
	sqlite3_finalize(stmt);
	stats.genres--;
	send_stats();
	return 0;
}
int get_songs_by_field(song **data, char *field, char *item)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT f.file, f.weight, f.sticky\
				FROM files f INNER JOIN tags t ON f.id = t.id\
				WHERE t.field = ? AND t.tag = ?\
				ORDER BY f.file COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, field, strlen(field), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 2, item, strlen(item), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* file = sqlite3_column_text(stmt, 0);
		(*data)[i].file = malloc(strlen((const char*)file) + 1);
		memcpy((*data)[i].file,(const char*) file, strlen((const char*)file) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 1);
		(*data)[i].sticky = sqlite3_column_int(stmt, 2);
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int add_to_playnow_list_by_field(char *field, char *item)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT f.file, f.weight, f.sticky\
				FROM files f INNER JOIN tags t ON f.id = t.id\
				WHERE t.field = ? AND t.tag = ?\
				ORDER BY f.file COLLATE NOCASE DESC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, field, strlen(field), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 2, item, strlen(item), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* file = sqlite3_column_text(stmt, 0);
		push_to_playnow_list((char*) file, sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2));
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
//gets a list of songs matching a field and tag and the tags that go along with the songs
int get_songs_and_tags_by_field(song **data, char *field, char *item)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	//char *sql = "SELECT files.file, files.weight, files.sticky FROM files, tags WHERE tags.field = ? AND tags.tag = ? AND files.id = tags.id";
	char *sql = "SELECT f.file, f.weight, f.sticky, f.id, t2.field, t2.tag\
				FROM files f INNER JOIN tags t ON f.id = t.id\
				INNER JOIN tags t2 ON f.id = t2.id\
				WHERE t.field = ? AND t.tag = ?\
				ORDER BY f.file COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK) {
		printf("DBERROR %s\n", sqlite3_errmsg(db));
		return 1;
	}
	int bind = sqlite3_bind_text(stmt, 1, field, strlen(field), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	bind = sqlite3_bind_text(stmt, 2, item, strlen(item), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;
	char *tag_array[4096];
	int tag_len[4096];//temporary
	char com[BUFF];
	memset(com, 0, 5);
	char *pcom = &com[5];//leave room for the number of fields at the beginning
	int len = 5;
	int cur_id = -1;
	int i = 0;
	int j = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		int id = sqlite3_column_int(stmt, 3);
		if (cur_id != id) {
			const unsigned char* file = sqlite3_column_text(stmt, 0);
			(*data)[i].file = malloc(strlen((const char*)file) + 1);
			memcpy((*data)[i].file,(const char*) file, strlen((const char*)file) + 1);
			(*data)[i].weight = sqlite3_column_int(stmt, 1);
			(*data)[i].sticky = sqlite3_column_int(stmt, 2);
			if (cur_id != -1) {
				char num[5];
				memset(num, 0, 5);
				//itoa(j, num);
				snprintf(num, 5, "%d", j);
				pcom = &com[5 - strlen(num) - 1];
				memcpy(pcom, num, strlen(num) + 1);
				len -= (5 - (strlen(num) + 1));
				tag_array[i-1] = malloc(len);
				tag_len[i-1] = len;
				memcpy(tag_array[i-1], pcom, len);
				memset(com, 0, BUFF);
				pcom = &com[5];
				len = 5;
			}
			cur_id = id;
			i++;
			j = 0;
		}
		const unsigned char* field = sqlite3_column_text(stmt, 4);
		const unsigned char* tag = sqlite3_column_text(stmt, 5);
		if (field != NULL)
		{
			memcpy(pcom, field, strlen((char*)field) + 1);
			pcom += strlen((char*)field) + 1;
			len += strlen((char*)field) + 1;
			memcpy(pcom, tag, strlen((char*)tag) + 1);
			pcom += strlen((char*)tag) + 1;
			len += strlen((char*)tag) + 1;
			j++;
		}
	}
	char num[5];
	memset(num, 0, 5);
	//itoa(j, num);
	snprintf(num, 5, "%d", j);
	pcom = &com[5 - strlen(num) - 1];
	memcpy(pcom, num, strlen(num) + 1);
	len -= (5 - (strlen(num) + 1));
	tag_array[i-1] = malloc(len);
	tag_len[i-1] = len;
	memcpy(tag_array[i-1], pcom, len);
	send_tag_data(tag_array, tag_len, i);
	sqlite3_finalize(stmt);
	return i;
}
//gets a complete list of unique fields, passes on the identity of the requesting tree view in com_char
int get_complete_field_list(char com_char)//FIXME - get rid of the sort and copy routine
{
	printf("get complete field list\n");
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT DISTINCT field FROM tags ORDER BY field COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int i = 0;
	int len = 2;//reserve 2 bytes for the command chars
	char **s;
	s = malloc(200*sizeof(char*));
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* field = sqlite3_column_text(stmt, 0);
		s[i] = malloc(strlen((const char*) field) + 1);
		memset(s[i], 0, strlen((const char*) field) + 1);
		memcpy(s[i], (const char*) field, strlen((const char*)field) + 1);
		len += strlen((char*)field) + 1;
//		printf("field = %s\n", field);
		i++;
	}
//	printf("sorting\n");
	qsort(s, i, sizeof(char*), cmpstringp);
	char num[6];
	memset(num, 0, 6);
	//itoa(i, num);
	int ret = snprintf(num, 6, "%d", i);
	if(ret >= 6)
		printf("truncated writing in get_complete_field_list");
	len += strlen(num) + 1;
	char com[len];
	com[0] = 'E';
	com[1] = com_char;
	char *pcom = &com[2];
	memcpy(pcom, num, strlen(num) + 1);
	pcom += strlen(num) + 1;
	int j;
	for (j = 0; j < i; j++)
	{
		memcpy(pcom, s[j], strlen(s[j]) + 1);
		pcom += strlen(s[j]) + 1;
		free(s[j]);
	}
	sqlite3_finalize(stmt);
	printf("num fields = %d\n", i);
	send_command(com, len);
	return i;
}
//gets all albums added since time
int get_recent_albums(song **data, int sec)
{
	sqlite3_stmt *stmt;
	char *pz_left;
	int epoch = time(NULL) - sec;

	//char *sql = "SELECT album, weight, sticky FROM albums WHERE timeStamp >= ?";
	char *sql = "SELECT t.tag, COUNT (t.tag), a.album, a.weight, a.sticky\
				FROM albums a INNER JOIN tags t ON a.album = t.tag\
				WHERE t.field = ? AND a.timeStamp >= ?\
				GROUP BY t.tag HAVING (COUNT(t.tag) > 1)";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK) {
		printf("prepare failed %d\n", prepare);
		printf("%s\n", sqlite3_errmsg(db));
		return 1;
	}
	int bind = sqlite3_bind_text(stmt, 1, "TALB", 4, SQLITE_STATIC);
	if (bind != SQLITE_OK) {
		printf("failed bind\n");
		return 2;
	}
	bind = sqlite3_bind_int(stmt, 2, epoch);
	if (bind != SQLITE_OK) {
		printf("failed bind\n");
		return 2;
	}
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* album = sqlite3_column_text(stmt, 2);
		(*data)[i].file = malloc(strlen((char*)album) + 1);
		memcpy((*data)[i].file, (char*)album, strlen((char*)album) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 3);
		(*data)[i].sticky = sqlite3_column_int(stmt, 4);
		i++;
	}
	sqlite3_clear_bindings(stmt);
	sqlite3_finalize(stmt);
	return i;
}
int get_recent_artists(song **data, int sec)
{
	sqlite3_stmt *stmt;
	char *pz_left;
	int epoch = time(NULL) - sec;

	char *sql = "SELECT artist, weight, sticky FROM artists WHERE timeStamp >= ?";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, epoch);
	if (bind != SQLITE_OK)
		return 2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* song = sqlite3_column_text(stmt, 0);
		(*data)[i].file = malloc(strlen((char*)song) + 1);
		memcpy((*data)[i].file, (char*)song, strlen((char*)song) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 1);
		(*data)[i].sticky = sqlite3_column_int(stmt, 2);
		printf("artist = %s\n", song);
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
int get_recent_songs(song **data, int sec)
{
	sqlite3_stmt *stmt;
	char *pz_left;
	int epoch = time(NULL) - sec;

	char *sql = "SELECT file, weight, sticky FROM files WHERE timeStamp >= ? ORDER BY timeStamp";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_int(stmt, 1, epoch);
	if (bind != SQLITE_OK)
		return 2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* song = sqlite3_column_text(stmt, 0);
		(*data)[i].file = malloc(strlen((char*)song) + 1);
		memcpy((*data)[i].file, (char*)song, strlen((char*)song) + 1);
		(*data)[i].weight = sqlite3_column_int(stmt, 1);
		(*data)[i].sticky = sqlite3_column_int(stmt, 2);
		//printf("song = %s\n", song);
		i++;
	}
	sqlite3_finalize(stmt);
	return i;
}
//gets the "field" listing for the given song if it has one
//calling function is responsible for allocating and freeing the tag parameter
//returns the number of tags found (should only be 0 or 1)
int get_tag_from_song(char *song, char *tag, char *field)
{
	//do we even have a current song
	if (song == NULL)
		return -3;
	sqlite3_stmt *stmt;
	char *pz_left;

	char *sql = "SELECT tags.tag FROM files, tags WHERE files.file = ? AND tags.field = ? AND files.id = tags.id";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, song, strlen(song), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, field, strlen(field), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char *temp = sqlite3_column_text(stmt, 0);
		memcpy(tag, (char*)temp, strlen((char*)temp) + 1);
		i++;
	}

	sqlite3_finalize(stmt);
	return i;
}
/*
 * Takes a file name and a null pointer for tag data.
 * Populates an array of structs with field and tag data for the song.
 * Returns the number of tags.
 */
int get_tag_info(char pref, char* song)
{
	sqlite3_stmt *stmt;
	char *pz_left;
	char *sql = "SELECT field, tag FROM files, tags WHERE file = ? AND files.id = tags.id";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	int bind = sqlite3_bind_text(stmt, 1, song, strlen(song), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return 2;

	char com[1<<16];
	memset(com, 0, 5);
	char *pcom = &com[5];//leave room for the number of fields at the beginning
	int len = 5;
	int i = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* field = sqlite3_column_text(stmt, 0);
		const unsigned char* tag = sqlite3_column_text(stmt, 1);
		if (field != NULL)
		{
			memcpy(pcom, field, strlen((char*)field) + 1);
			pcom += strlen((char*)field) + 1;
			len += strlen((char*)field) + 1;
			memcpy(pcom, tag, strlen((char*)tag) + 1);
			pcom += strlen((char*)tag) + 1;
			len += strlen((char*)tag) + 1;
			i++;
		}
	}
	sqlite3_finalize(stmt);
	//copy the number of fields
	char num[5];
	memset(num, 0, 5);
	//itoa(i, num);
	int ret = snprintf(num, 5, "%d", i);
	if(ret >= 5)
		printf("truncated print in get_tag_info");
	pcom = &com[5 - strlen(num) - 1];
	memcpy(pcom, num, strlen(num) + 1);
	len -= (5 - (strlen(num) + 1));
	if (pref != 'X')
	{
		com[5-strlen(num) - 2] = pref;
		pcom = &com[5-strlen(num) - 2];
		len++;
	}
	send_command(pcom, len);
	send_command_bt(pcom, len);
	return i;
}
/*
 * Takes a directory and empty pointer to an array of song structs
 * and populates the struct with the weight, sticky and filename data
 * and sends the tags to the client
 * returns the number of songs found
 */
int get_songs_and_tags_in_dir(char *dir, song** data)
{
	sqlite3_stmt *stmt;
	char *pz_left;
	//char *sql = "SELECT file FROM files WHERE file GLOB ? AND file NOT GLOB ?";
	char *sql = "SELECT f.file, f.weight, f.sticky, f.id, t.field, t.tag\
				FROM files f LEFT JOIN tags t ON f.id = t.id\
				WHERE f.file GLOB ? AND f.file NOT GLOB ?\
				ORDER BY f.file COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK) {
		printf("%s\n", sqlite3_errmsg(db));
		return -1;
	}
	char bdir[strlen(dir) + 3];
	memset(bdir, 0, strlen(dir) + 3);
	bzero(bdir, strlen(bdir));
	memcpy(bdir, dir, strlen(dir) + 1);
	strcat (bdir, "/*");
	char pdir[strlen(dir) + 1024];
	memset(pdir, 0, strlen(dir) + 1024);
	bzero(pdir, strlen(pdir));
	memcpy(pdir, dir, strlen(dir));
	strcat(pdir, "/*/*\0");
	int bind = sqlite3_bind_text(stmt, 1, bdir, strlen(bdir), SQLITE_STATIC);
	if (bind != SQLITE_OK)
	{
		printf("bind fail %s\n", sqlite3_errmsg(db));
		return -2;
	}
	bind = sqlite3_bind_text(stmt, 2, pdir, strlen(pdir), SQLITE_STATIC);
	if (bind != SQLITE_OK)
	{
		printf("bind fail %s\n", sqlite3_errmsg(db));
		return -2;
	}
	//char tag_array[4096][BUFF];//array of 4096 tags lists each of size BUFF
	char *tag_array[4096];
	int tag_len[4096];//temporary
	char com[BUFF];
	memset(com, 0, 5);
	char *pcom = &com[5];//leave room for the number of fields at the beginning
	int len = 5;
	int cur_id = -1;
	int i = 0;
	int j = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		int id = sqlite3_column_int(stmt, 3);
		if (cur_id != id) {
			const unsigned char* file = sqlite3_column_text(stmt, 0);
			(*data)[i].file = malloc(strlen((char*)file) + 1);
			memcpy((*data)[i].file,(const char*) file, strlen((const char*)file) + 1);
			(*data)[i].weight = sqlite3_column_int(stmt, 1);
			(*data)[i].sticky = sqlite3_column_int(stmt, 2);
			if (cur_id != -1) {
				char num[5];
				memset(num, 0, 5);
				//itoa(j, num);
				snprintf(num, 5, "%d", j);
				pcom = &com[5 - strlen(num) - 1];
				memcpy(pcom, num, strlen(num) + 1);
				len -= (5 - (strlen(num) + 1));
				tag_array[i-1] = malloc(len);
				tag_len[i-1] = len;
				memcpy(tag_array[i-1], pcom, len);
				memset(com, 0, BUFF);
				pcom = &com[5];
				len = 5;
			}
			cur_id = id;
			i++;
			j = 0;
		}
		const unsigned char* field = sqlite3_column_text(stmt, 4);
		const unsigned char* tag = sqlite3_column_text(stmt, 5);
		if (field != NULL)
		{
			memcpy(pcom, field, strlen((char*)field) + 1);
			pcom += strlen((char*)field) + 1;
			len += strlen((char*)field) + 1;
			memcpy(pcom, tag, strlen((char*)tag) + 1);
			pcom += strlen((char*)tag) + 1;
			len += strlen((char*)tag) + 1;
			j++;
		}
	}
	char num[5];
	memset(num, 0, 5);
	//itoa(j, num);
	snprintf(num, 5, "%d", j);
	pcom = &com[5 - strlen(num) - 1];
	memcpy(pcom, num, strlen(num) + 1);
	len -= (5 - (strlen(num) + 1));
	tag_array[i-1] = malloc(len);
	tag_len[i-1] = len;
	memcpy(tag_array[i-1], pcom, len);
	send_tag_data(tag_array, tag_len, i);
	sqlite3_finalize(stmt);
	return i;
}
int search(char *field, char *query, song **data)
{
	int num_ret;

	if (strcmp(field, "All tags") == 0)
		num_ret = search_all_tags(query, data);
	else if (strcmp(field, "Full path") == 0)
		num_ret = search_full_path(query, data);
	else
		num_ret = search_by_field(field, query, data);
	printf("got %d results\n", num_ret);

	if (num_ret < 0)
	{
		printf("search error\n");
		return 0;
	}
	return num_ret;
}
int search_by_field(char *field, char *query, song** data)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	printf("search by %s for %s\n", field, query);
	char trfield[16];
	memset(trfield, 0, 16);
	if (strcmp(field, "Album") == 0)
		memcpy(trfield, "TALB\0", 5);
	else if (strcmp(field, "Artist") == 0)
		memcpy(trfield, "TPE1\0", 5);
	else if (strcmp(field, "Genre") == 0)
		memcpy(trfield, "TCON\0", 5);
	else if (strcmp(field, "Title") == 0)
		memcpy(trfield, "TIT2\0", 5);
	else if (strcmp(field, "Year") == 0)
		memcpy(trfield, "TYER\0", 5);

	char trquery[1026];
	memset(trquery, 0, 1026);
	trquery[0] = '%';
	strncat(trquery, query, strlen(query));
	strncat(trquery, "%\0", 2);

	printf("trfield = %s\n", trfield);
	//char *sql = "SELECT files.file FROM files, tags WHERE tags.field = ? AND tags.tag LIKE ? AND files.id = tags.id";
	char *sql = "SELECT f.file, f.weight, f.sticky, f.id, t2.field, t2.tag\
				FROM files f INNER JOIN tags t ON f.id = t.id\
				INNER JOIN tags t2 ON f.id = t2.id\
				WHERE t.field = ? AND t.tag LIKE ?\
				ORDER BY f.file COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, trfield, strlen(trfield), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	bind = sqlite3_bind_text(stmt, 2, trquery, strlen(trquery), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	char *tag_array[4096];
	int tag_len[4096];//temporary
	char com[BUFF];
	memset(com, 0, 5);
	char *pcom = &com[5];//leave room for the number of fields at the beginning
	int len = 5;
	int cur_id = -1;
	int i = 0;
	int j = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		int id = sqlite3_column_int(stmt, 3);
		if (cur_id != id) {
			const unsigned char* file = sqlite3_column_text(stmt, 0);
			(*data)[i].file = malloc(strlen((const char*)file) + 1);
			memcpy((*data)[i].file,(const char*) file, strlen((const char*)file) + 1);
			(*data)[i].weight = sqlite3_column_int(stmt, 1);
			(*data)[i].sticky = sqlite3_column_int(stmt, 2);
			if (cur_id != -1) {
				char num[5];
				memset(num, 0, 5);
				//itoa(j, num);
				snprintf(num, 5, "%d", j);
				pcom = &com[5 - strlen(num) - 1];
				memcpy(pcom, num, strlen(num) + 1);
				len -= (5 - (strlen(num) + 1));
				tag_array[i-1] = malloc(len);
				tag_len[i-1] = len;
				memcpy(tag_array[i-1], pcom, len);
				memset(com, 0, BUFF);
				pcom = &com[5];
				len = 5;
			}
			cur_id = id;
			i++;
			j = 0;
		}
		const unsigned char* field = sqlite3_column_text(stmt, 4);
		const unsigned char* tag = sqlite3_column_text(stmt, 5);
		if (field != NULL)
		{
			memcpy(pcom, field, strlen((char*)field) + 1);
			pcom += strlen((char*)field) + 1;
			len += strlen((char*)field) + 1;
			memcpy(pcom, tag, strlen((char*)tag) + 1);
			pcom += strlen((char*)tag) + 1;
			len += strlen((char*)tag) + 1;
			j++;
		}
	}
	char num[5];
	memset(num, 0, 5);
	//itoa(j, num);
	snprintf(num, 5, "%d", j);
	pcom = &com[5 - strlen(num) - 1];
	memcpy(pcom, num, strlen(num) + 1);
	len -= (5 - (strlen(num) + 1));
	tag_array[i-1] = malloc(len);
	tag_len[i-1] = len;
	memcpy(tag_array[i-1], pcom, len);
	send_tag_data(tag_array, tag_len, i);
	sqlite3_finalize(stmt);
	return i;
}
int search_all_tags(char *query, song** data)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char trquery[1026];
	memset(trquery, 0, 1026);
	trquery[0] = '%';
	strncat(trquery, query, strlen(query));
	strncat(trquery, "%\0", 2);

	printf("search all tags %s\n", trquery);
	char *sql = "SELECT f.file, f.weight, f.sticky, f.id, t2.field, t2.tag\
				FROM files f INNER JOIN tags t ON f.id = t.id\
				INNER JOIN tags t2 ON f.id = t2.id\
				WHERE t.tag LIKE ?\
				ORDER BY f.file COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, trquery, strlen(trquery), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	char *tag_array[4096];
	int tag_len[4096];//temporary
	char com[BUFF];
	memset(com, 0, 5);
	char *pcom = &com[5];//leave room for the number of fields at the beginning
	int len = 5;
	int cur_id = -1;
	int i = 0;
	int j = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		int id = sqlite3_column_int(stmt, 3);
		if (cur_id != id) {
			const unsigned char* file = sqlite3_column_text(stmt, 0);
			(*data)[i].file = malloc(strlen((const char*)file) + 1);
			memcpy((*data)[i].file,(const char*) file, strlen((const char*)file) + 1);
			(*data)[i].weight = sqlite3_column_int(stmt, 1);
			(*data)[i].sticky = sqlite3_column_int(stmt, 2);
			if (cur_id != -1) {
				char num[5];
				memset(num, 0, 5);
				//itoa(j, num);
				snprintf(num, 5, "%d", j);
				pcom = &com[5 - strlen(num) - 1];
				memcpy(pcom, num, strlen(num) + 1);
				len -= (5 - (strlen(num) + 1));
				tag_array[i-1] = malloc(len);
				tag_len[i-1] = len;
				memcpy(tag_array[i-1], pcom, len);
				memset(com, 0, BUFF);
				pcom = &com[5];
				len = 5;
			}
			cur_id = id;
			i++;
			j = 0;
		}
		const unsigned char* field = sqlite3_column_text(stmt, 4);
		const unsigned char* tag = sqlite3_column_text(stmt, 5);
		if (field != NULL)
		{
			memcpy(pcom, field, strlen((char*)field) + 1);
			pcom += strlen((char*)field) + 1;
			len += strlen((char*)field) + 1;
			memcpy(pcom, tag, strlen((char*)tag) + 1);
			pcom += strlen((char*)tag) + 1;
			len += strlen((char*)tag) + 1;
			j++;
		}
	}
	char num[5];
	memset(num, 0, 5);
	//itoa(j, num);
	snprintf(num, 5, "%d", j);
	pcom = &com[5 - strlen(num) - 1];
	memcpy(pcom, num, strlen(num) + 1);
	len -= (5 - (strlen(num) + 1));
	tag_array[i-1] = malloc(len);
	tag_len[i-1] = len;
	memcpy(tag_array[i-1], pcom, len);
	send_tag_data(tag_array, tag_len, i);
	sqlite3_finalize(stmt);
	return i;
}
int search_full_path(char *query, song **data)
{
	sqlite3_stmt *stmt;
	char *pz_left;

	char trquery[1026];
	memset(trquery, 0, 1026);
	trquery[0] = '%';
	strncat(trquery, query, strlen(query));
	strncat(trquery, "%\0", 2);

	printf("search full path %s\n", trquery);
	//char *sql = "SELECT file FROM files WHERE file LIKE ?";
	char *sql = "SELECT f.file, f.weight, f.sticky, f.id, t.field, t.tag\
				FROM files f INNER JOIN tags t ON f.id = t.id\
				WHERE f.file LIKE ?\
				ORDER BY f.file COLLATE NOCASE ASC";
	int prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return -1;
	int bind = sqlite3_bind_text(stmt, 1, trquery, strlen(trquery), SQLITE_STATIC);
	if (bind != SQLITE_OK)
		return -2;
	char *tag_array[4096];
	int tag_len[4096];//temporary
	char com[BUFF];
	memset(com, 0, 5);
	char *pcom = &com[5];//leave room for the number of fields at the beginning
	int len = 5;
	int cur_id = -1;
	int i = 0;
	int j = 0;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		int id = sqlite3_column_int(stmt, 3);
		if (cur_id != id) {
			const unsigned char* file = sqlite3_column_text(stmt, 0);
			(*data)[i].file = malloc(strlen((const char*)file) + 1);
			memcpy((*data)[i].file,(const char*) file, strlen((const char*)file) + 1);
			(*data)[i].weight = sqlite3_column_int(stmt, 1);
			(*data)[i].sticky = sqlite3_column_int(stmt, 2);
			if (cur_id != -1) {
				char num[5];
				memset(num, 0, 5);
				//itoa(j, num);
				snprintf(num, 5, "%d", j);
				pcom = &com[5 - strlen(num) - 1];
				memcpy(pcom, num, strlen(num) + 1);
				len -= (5 - (strlen(num) + 1));
				tag_array[i-1] = malloc(len);
				tag_len[i-1] = len;
				memcpy(tag_array[i-1], pcom, len);
				memset(com, 0, BUFF);
				pcom = &com[5];
				len = 5;
			}
			cur_id = id;
			i++;
			j = 0;
		}
		const unsigned char* field = sqlite3_column_text(stmt, 4);
		const unsigned char* tag = sqlite3_column_text(stmt, 5);
		if (field != NULL)
		{
			memcpy(pcom, field, strlen((char*)field) + 1);
			pcom += strlen((char*)field) + 1;
			len += strlen((char*)field) + 1;
			memcpy(pcom, tag, strlen((char*)tag) + 1);
			pcom += strlen((char*)tag) + 1;
			len += strlen((char*)tag) + 1;
			j++;
		}
	}
	char num[5];
	memset(num, 0, 5);
	//itoa(j, num);
	snprintf(num, 5, "%d", j);
	pcom = &com[5 - strlen(num) - 1];
	memcpy(pcom, num, strlen(num) + 1);
	len -= (5 - (strlen(num) + 1));
	tag_array[i-1] = malloc(len);
	tag_len[i-1] = len;
	memcpy(tag_array[i-1], pcom, len);
	send_tag_data(tag_array, tag_len, i);
	sqlite3_finalize(stmt);
	return i;
}
/*
 * Takes a pointer to a pointer to an array of song structs
 * and populates the array with all the filenames, weights and sticky flag from the database
 * Returns the total number of songs.
 */
int get_all_songs(struct gen_node** head)
{
	int prepare;
	sqlite3_stmt *stmt;
	char *pz_left;
	int i = 0;

	char *sql = "SELECT file, weight, sticky FROM files ORDER BY file COLLATE NOCASE ASC";
	prepare = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, (const char **) &pz_left);
	if (prepare != SQLITE_OK)
		return 1;
	while (SQLITE_DONE != sqlite3_step(stmt))
	{
		const unsigned char* temp = sqlite3_column_text(stmt, 0);
		int weight = sqlite3_column_int(stmt, 1);
		int sticky = sqlite3_column_int(stmt, 2);
		check_extension((char*)temp);
		if (weight == 0)
			stats.zero++;
		else if (weight == 100)
			stats.hund++;
		if (sticky == 1)
			stats.sticky++;
		stats.count[weight]++;
		stats.total++;
		add_to_hash((char*)temp, weight, sticky, 0);
		make_push_node(head, (char*)temp, weight,sticky);

		i++;
	}
	printf("mp3 = %d\n", stats.mp3);
	printf("ogg = %d\n", stats.ogg);
	printf("mpc = %d\n", stats.mpc);
	printf("wav = %d\n", stats.wav);
	printf("flac = %d\n", stats.flac);
	printf("m4a = %d\n", stats.m4a);
	printf("wma = %d\n", stats.wma);
	printf("other = %d\n", stats.other);
	sqlite3_reset(stmt);

	sqlite3_finalize(stmt);
	return i;
}
void check_extension(char *s)
{
	s += strlen(s);
	while (*(--s) != '.')
		;
	s++;
	if ((strcmp(s, "mp3") == 0) || (strcmp(s, "Mp3") == 0) || (strcmp(s, "MP3") == 0))
		stats.mp3++;
	else if (strcmp(s, "ogg") == 0)
		stats.ogg++;
	else if (strcmp(s, "mpc") == 0)
		stats.mpc++;
	else if (strcmp(s, "wav") == 0)
		stats.wav++;
	else if (strcmp(s, "flac") == 0)
		stats.flac++;
	else if (strcmp(s, "m4a") == 0)
		stats.m4a++;
	else if (strcmp(s, "wma") == 0)
		stats.wma++;
	else
	{
		stats.other++;
		printf("other = %s\n", s);
	}
}
int cmpstringp(const void *p1, const void *p2)
{
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}
