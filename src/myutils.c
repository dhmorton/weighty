/*
 * myutils.c
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

#include "myutils.h"

#include "net_utils.h"

char *homedir = NULL;
char *configfile = NULL;
char *alarmconfig = NULL;
char *sleepconfig = NULL;
char *streamconfig = NULL;
char *database = NULL;
char *errorlog = NULL;
char *discogs = NULL;

//struct config val;

static void reverse(char*);

void init_files()
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
	database = malloc(strlen(homedir) + 12);
	memset(database, 0, strlen(homedir) + 12);
	memcpy(database, homedir, strlen(homedir));
	strcat(database, "/weighty.db");
	errorlog = malloc(strlen(homedir) + 11);
	memset(errorlog, 0, strlen(homedir) + 11);
	memcpy(errorlog, homedir, strlen(homedir));
	strcat(errorlog, "/error.log");
	discogs = malloc(strlen(homedir) + 9);
	memset(discogs, 0, strlen(homedir) + 9);
	memcpy(discogs, homedir, strlen(homedir));
	strcat(discogs, "/discogs");
}

//just for positive integers
void itoa(int n, char s[])
{
    int i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    s[i] = '\0';
    reverse(s);
}
void reverse(char *p)
{
  char *q = p;
  while(q && *q) ++q;
  for(--q; p < q; ++p, --q)
    *p = *p ^ *q,
    *q = *p ^ *q,
    *p = *p ^ *q;
}
//checks to see if the extension matches standard song extensions
//returns a non-negative value depending on the extension
//negative values means it doesn't have an extension or it doesn't match
int check_file(const char *file)
{
	char *s = strrchr(file, '.');
	if (s == NULL)
		return -1;
	s++;//step past the .

	if (strcasecmp(s, "ogg") == 0)
		return 0;
	else if (strcasecmp(s, "mp3") == 0)
		return 1;
	else if (strcasecmp(s, "flac") == 0)
		return 2;
	else if (strcasecmp(s, "mpc") == 0)
		return 3;
	else if (strcasecmp(s, "m4a") == 0)
		return 4;
	else if (strcasecmp(s, "wav") == 0)
		return 5;
	else if (strcasecmp(s, "wma") == 0)
		return 6;
	else if (strcasecmp(s, "mp4") == 0)
		return 7;
	else if (strcasecmp(s, "asf") == 0)
		return 8;
	else if (strcasecmp(s, "ram") == 0)
		return 9;
	else if (strcasecmp(s, "rm") == 0)
		return 10;
	else if (strcasecmp(s, "ac3") == 0)
		return 11;
	else
		return -2;
	return -1;
}
//replace field with a translated version
//These are tuned to vorbiscomment field preferences but come from the standard id3 field names
void translate_field(char* field, char *trfield)
{
	if (strcmp(field, "TPE1") == 0)
		memcpy(trfield, "Artist\0", 7);
	else if (strcmp(field, "TPE2") == 0)
		memcpy(trfield, "Band/orchestra/accompaniment\0", 29);
	else if (strcmp(field, "TALB") == 0)
		memcpy(trfield, "Album\0", 6);
	else if (strcmp(field, "TCON") == 0)
		memcpy(trfield, "Genre\0", 6);
	else if (strcmp(field, "TIT2") == 0)
		memcpy(trfield, "Title\0", 6);
	else if (strcmp(field, "COMM") == 0)
		memcpy(trfield, "Description\0", 12);
	else if (strcmp(field, "TCOM") == 0)
		memcpy(trfield, "Composer\0", 9);
	else if (strcmp(field, "TLEN") == 0)
		memcpy(trfield, "Length\0", 7);
	else if (strcmp(field, "TRCK") == 0)
		memcpy(trfield, "Tracknumber\0", 12);
	else if (strcmp(field, "TYER") == 0)
		memcpy(trfield, "Date\0", 5);
	else if (strcmp(field, "TCOP") == 0)
		memcpy(trfield, "Copyright\0", 10);
	else if (strcmp(field, "TDRC") == 0)
		memcpy(trfield, "Recording time\0", 15);
	else if (strcmp(field, "TPUB") == 0)
		memcpy(trfield, "Publisher\0", 10);
	else if (strcmp(field, "TPOS") == 0)
		memcpy(trfield, "Part of a set\0", 14);
	else if (strcmp(field, "TFLT") == 0)
		memcpy(trfield, "File type\0", 10);
	else if (strcmp(field, "TENC") == 0)
		memcpy(trfield, "Encoded by\0", 11);
	else if (strcmp(field, "TPE3") == 0)
		memcpy(trfield, "Performer\0", 10);
	else
	{
//		printf("MISSING FIELD %s\n", field);
		memcpy(trfield, field, strlen(field) + 1);
	}
}
void back_translate(char *field, char **trfield)
{
	if (strcasecmp(field, "Artist") == 0)
		memcpy(*trfield, "TPE1\0", 5);
	else if (strcasecmp(field, "Band/orchestra/accompaniment") == 0)
		memcpy(*trfield, "TPE2\0", 5);
	else if (strcasecmp(field, "Album") == 0)
		memcpy(*trfield, "TALB\0", 5);
	else if (strcasecmp(field, "Genre") == 0)
		memcpy(*trfield, "TCON\0", 5);
	else if (strcasecmp(field, "Title") == 0)
		memcpy(*trfield, "TIT2\0", 5);
	else if (strcasecmp(field, "Comments") == 0)
		memcpy(*trfield, "COMM\0", 5);
	else if (strcasecmp(field, "Composer") == 0)
		memcpy(*trfield, "TCOM\0", 5);
	else if (strcasecmp(field, "Length") == 0)
		memcpy(*trfield, "TLEN\0", 5);
	else if ((strcasecmp(field, "Track #") == 0) || (strcmp(field, "Track Number") == 0))
		memcpy(*trfield, "TRCK\0", 5);
	else if ((strcasecmp(field, "Year") == 0)|| (strcmp(field, "DATE") == 0) || (strcmp(field, "Date") == 0))
		memcpy(*trfield, "TYER\0", 5);
	else if (strcasecmp(field, "Copyright") == 0)
		memcpy(*trfield, "TCOP\0", 5);
	else if ((strcasecmp(field, "Recording time") == 0))
		memcpy(*trfield, "TDRC\0", 5);
	else if (strcasecmp(field, "Publisher") == 0)
		memcpy(*trfield, "TPUB\0", 5);
	else if (strcasecmp(field, "Part of a set") == 0)
		memcpy(*trfield, "TPOS\0", 5);
	else if (strcasecmp(field, "File type") == 0)
		memcpy(*trfield, "TFLT\0", 5);
	else if (strcasecmp(field, "Encoded by") == 0)
		memcpy(*trfield, "TENC\0", 5);
	else if (strcasecmp(field, "Conductor/performer refinement") == 0)
		memcpy(*trfield, "TPE3\0", 5);
	else
		memcpy(*trfield, field, 5);
}
void print_data(char *s, int len)
{
	printf("PRINT DATA\n");
	int i;
	for (i = 0; i < len; i++)
	{
		if (s[i] == '\0')
			printf("|");
		else if (s[i] == '\r')
			printf("!");
		else
			printf("%c", s[i]);
	}
	printf("\n");
}
int error(char *s)
{
	perror(s);
	printf("\n");
//	exit(EXIT_FAILURE);
	return 0;
}

int read_config()
{
	printf("reading config...");
	FILE *fp;

	if ((fp = fopen(configfile, "r")) == NULL)
		printf("config file doesn't exist\n");
	else
	{
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		int counter = 0;
		char *fields[] = {
				"musicdir", "type", "threshhold", "var", "playby",
				"song_random", "song_skip", "album_random", "album_skip", "artist_random", "artist_skip", "genre_random", "genre_skip"};
		char *p;
		while ((read = getline(&line, &len, fp)) != -1)
		{
			char buf[16];
			for(p = line; *p != '='; p++)
				;
			strncpy(buf, line, p - line);
			if (*(p-1) == ' ')
				buf[p - line - 1] = '\0';
			else
				buf[p - line] = '\0';
			char value[256];
			if(*(++p) == ' ')
				p++;
			strncpy(value, p, 256);
			value[strlen(value) - 1] = '\0';
			int i;
			for(i = 0; i < 13; i++)
				if (strcmp(buf, fields[i]) == 0)
					switch (i)
					{
					case 0:
						strcpy(val.musicdir, value);
						break;
					case 1:
						strcpy(val.type, value);
						break;
					case 2:
						val.threshhold = atoi(value);
						break;
					case 3:
						val.var = atoi(value);
						break;
					case 4:
						strcpy(val.playby, value);
						break;
					case 5:
						val.song_rand = atoi(value);
						break;
					case 6:
						val.song_skip = atoi(value);
						break;
					case 7:
						val.album_rand = atoi(value);
						break;
					case 8:
						val.album_skip = atoi(value);
						break;
					case 9:
						val.artist_rand = atoi(value);
						break;
					case 10:
						val.artist_skip = atoi(value);
						break;
					case 11:
						val.genre_rand = atoi(value);
						break;
					case 12:
						val.genre_skip = atoi(value);
						break;
					}
			counter++;
		}
		if (line)
			free(line);
		fclose(fp);
	}
	return 0;
}
void write_config()
{
	char line[256];
	memset(line, 0, 256);
	FILE *fp;

	if ((fp = fopen(configfile, "w")) == NULL)
		printf("can't open config file for writing\n");
	memcpy(line, "musicdir = ", 11);
	strncat(line, val.musicdir, strlen(val.musicdir) + 1);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "type = ", 7);
	strncat(line, val.type, strlen(val.type) + 1);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "threshhold = ", 13);
	char t[4];
	memset(t, 0, 4);
	//itoa(val.threshhold, t);
	snprintf(t, 4, "%d", val.threshhold);
	strncat(line, t, strlen(t) + 1);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "var = ", 6);
	char v[4];
	memset(v, 0, 4);
	//itoa(val.var, v);
	snprintf(v, 3, "%d", val.var);
	strncat(line, v, strlen(v) + 1);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "playby = ", 9);
	strncat(line, val.playby, strlen(val.playby) + 1);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "song_random = ", 14);
	if (val.song_rand)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "song_skip = ", 12);
	if (val.song_skip)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);

	memset(line, 0, 256);
	memcpy(line, "album_random = ", 15);
	if (val.album_rand)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "album_skip = ", 13);
	if (val.album_skip)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);

	memset(line, 0, 256);
	memcpy(line, "artist_random = ", 16);
	if (val.artist_rand)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "artist_skip = ", 14);
	if (val.artist_skip)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);

	memset(line, 0, 256);
	memcpy(line, "genre_random = ", 15);
	if (val.genre_rand)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "genre_skip = ", 13);
	if (val.genre_skip)
		strncat(line, "1\0", 2);
	else
		strncat(line, "0\0", 2);
	fprintf(fp, "%s\n", line);

	fclose(fp);
}
//the next four functions take a weight, threshhold and var
//they each return a float between 0 and 1 inclusive
//for the linear function thresh is the weight at which the probality is 0.5, var is ignored
//for the exponential function thresh is where p = 0.5, var is ignored
//for the gaussian thresh is where the bell curve is centered and var is the variance
float linear(int weight, int thresh, int var)
{
	if (weight == 100)
		return 1.0;
	float numerator = weight + 100 - 2 * thresh;
	if (numerator < 0)
		return 0.0;
	float denominator = 2 * (100 - thresh);
	if (denominator == 0)//shouldn't ever happen
		return 1.0;
	float p = numerator / denominator;
	return p;
}
float exponential(int weight, int thresh, int var)
{
	float c = get_constant(thresh);
	return ((pow(2,(c * weight/100)) -1)/(pow(2, c) - 1) );
}
float gaussian(int weight, int thresh, int var)
{
	return (exp((-pow(weight - thresh, 2))/(2*pow(var, 2))));
}
float flat(int weight, int thresh, int var)
{
	return (weight > thresh ? 1 : 0);
}
//this is a constant used to calculate the exponential curve
//it calculates some value c that forces exponential() to return 0.5 at thresh
float get_constant(int thresh)
{
	float t = (float) thresh / 100;
	float c = 5.0;
	float delta = 1.0;

	float prob = (pow(2, c * t) - 1) / (pow(2, c) - 1);
	float diff = 0.5 - prob;

	while ((diff < -0.02) || (diff > 0.02))
	{
		if (diff < 0)
			c += delta;
		else if (diff > 0)
			c -= delta;
		prob = (pow(2, c * t) - 1) / (pow(2, c) - 1);
		diff = 0.5 - prob;
		if (prob < 1)
			delta += diff;
		else
			delta -= diff;
	}
	return c;
}

int should_play(int weight)
{
	float p, prob;
	float (*f)(int, int, int);
	if (strcmp(val.type, "linear") == 0)
		f = &linear;
	else if (strcmp(val.type, "exponential") == 0)
		f = &exponential;
	else if (strcmp(val.type, "gaussian") == 0)
		f = &gaussian;
	else if (strcmp(val.type, "flat") == 0)
		f = &flat;
	prob = f(weight, val.threshhold, val.var);
	p = ((double) random())/((double) RAND_MAX + 1);
	//printf("weight = %d p = %f prob = %f\n", weight, p, prob);
	return(prob >= p);
}
void send_discogs_key()
{
	char key[41];
	key[40] = 0;
	int fd;

	if ((fd = open(discogs, O_RDONLY)) == -1)
		printf("discogs file doesn't exist\n");
	else
	{
		read(fd, key, 40);
		char com[42];
		com[0] = 'Z';
		com[41] = 0;
		memcpy(&com[1], key, 40);
		send_command(com, strlen(com) + 1);
	}
}
