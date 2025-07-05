/*
 * myxine.h
 *
 *  Created on: Dec 23, 2016
 *      Author: bob
 */

typedef struct {
	int time;
	char *artist;
	char *title;
	char *stream;
} stream_track;

//static void event_listener(void*, const xine_event_t*);

int get_volume(void);
void send_volume(void);
int update_progressbar(void);

//update funcitons
void send_time(void);
int update_gui(void);
int update_progressbar(void);
int set_volume(int);
//control functions
int play(char*);
void play_cursong(void);
void play_song(void);
int stop(void);
int next(void);
int back(void);
int pausetoggle(void);
int jump_to_pos(int);
int is_running(void);