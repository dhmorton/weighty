/*
 * firsttime.c
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

#include "firsttime.h"
#include "myutils.h"
#include "sqlite.h"
#include "three_lists.h"

static GtkWidget *win;

static void set_musicdir(GtkWidget*);
static void create_config(const char*);
static void create_alarm_config(void);
static void create_sleep_config(void);
static void create_stream_config(void);
static int find_songfiles(const char*);
static void quit(void);


void get_musicdir()
{
	printf("get musicdir\n");
	gtk_init(NULL, NULL);
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (win), "Music directory");
	gtk_window_set_default_size(GTK_WINDOW (win), 300, 100);
	g_signal_connect(G_OBJECT (win), "destroy", G_CALLBACK (quit), NULL);

	GtkWidget *frame = gtk_frame_new("Enter your music directory.");
	GtkWidget *entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame), entry);

	gtk_container_add(GTK_CONTAINER(win), frame);

	g_signal_connect(entry, "activate", G_CALLBACK(set_musicdir), NULL);

	gtk_widget_show_all(GTK_WIDGET(win));
	gtk_main();
}
void set_musicdir(GtkWidget *entry)
{
	const gchar *musicdir;
	musicdir = gtk_entry_get_text(GTK_ENTRY(entry));
	printf("musicdir = %s\n", musicdir);
	//make sure it is a directory
	struct stat sbuf;
	if (stat(musicdir, &sbuf) == -1)
		printf("lstat failed on %s\n", musicdir);
	if (S_ISDIR(sbuf.st_mode))//the directory exists
	{
		//set up the config files
		create_config(musicdir);
		create_alarm_config();
		create_sleep_config();
		create_stream_config();
		//create the database
		create_database();
		//read what's there
		three_lists_init();
		find_songfiles(musicdir);
	}
	else
	{
		printf("%s is not a directory\n", musicdir);
		exit(0);
	}
	gtk_widget_destroy(win);
}
void create_config(const char *musicdir)
{
	char line[256];
	memset(line, 0, 256);
	FILE *fp;
	if ((fp = fopen(configfile, "w")) == NULL)
		printf("can't open config file for writing\n");
	memcpy(line, "musicdir = ", 11);
	strncat(line, musicdir, strlen(musicdir) + 1);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "type = linear\0", 14);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "threshhold = 50\0", 16);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "var = 5\0", 8);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "playby = song\0", 14);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "random = 1\0", 11);
	fprintf(fp, "%s\n", line);
	memset(line, 0, 256);
	memcpy(line, "noskip = 0\0", 11);
	fprintf(fp, "%s\n", line);
	fclose(fp);
}
void create_alarm_config()
{
	FILE *fp;
	if ((fp = fopen(alarmconfig, "w")) == NULL)
		printf("can't open config file for writing\n");
	int i;
	for (i = 0; i < 7; i++)
	{
		fprintf(fp, "%d\n", i);
		fprintf(fp, "0\n");
		fprintf(fp, "6\n");
		fprintf(fp, "30\n");
	}
	fprintf(fp, "0\n");
	fprintf(fp, "50\n");
	fprintf(fp, "529\n");
	fclose(fp);
}
void create_sleep_config()
{
	FILE *fp;
	if ((fp = fopen(sleepconfig, "w")) == NULL)
		printf("can't open config file for writing\n");
	else
	{
		fprintf(fp, "10\n");
		fprintf(fp, "5\n");
		fclose(fp);
	}
}
void create_stream_config()
{
	FILE *fp;
	if ((fp = fopen(streamconfig, "w")) == NULL)
		printf("can't open config file for writing\n");
	else
	{
		fprintf(fp, "name=KDVS Freeform Radio\n");
		fprintf(fp, "genre=Freeform\n");
		fprintf(fp, "bitrate=320\n");
		fprintf(fp, "description=KDVS Freeform Radio (320kbps)\n");
		fprintf(fp, "url=http://api.kdvs.org:8000/kdvs320\n");
		fclose(fp);
	}
}
/*takes a base directory and traverses recursively in search of music files
 * that match ogg, asf, wma, mp3, wav, rm, ram, mp4, flac, m4a, mpc
 * the tags are extracted and stored in the database table "tags"
*/
int find_songfiles(const char *file)
{
	struct stat sbuf;
	if (stat(file, &sbuf) == -1)
		printf("lstat failed on %s\n", file);
	if (S_ISDIR(sbuf.st_mode))
	{
		DIR *ddir;
		struct dirent *pdir;
		ddir = opendir(file);

		while ((pdir = readdir(ddir)) != NULL)
		{
			if (*(pdir->d_name) != '.')
			{
				char s[strlen(file) + strlen(pdir->d_name) + 2];//1 for the / and one for the \0
				strcpy(s, file);
				strcat(s, "/");
				strcat(s, pdir->d_name);
				s[strlen(file) + strlen(pdir->d_name) + 1] = '\0';
				find_songfiles(s);
			}
		}
		closedir(ddir);
	}
	else
		add_new_file(file);
	return 0;
}
void quit()
{
	exit(0);
}
