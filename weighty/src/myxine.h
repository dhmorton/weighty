/*
 * myxine.h
 *
 *  Created on: Dec 23, 2016
 *      Author: bob
 */

#ifndef SRC_MYXINE_H_
#define SRC_MYXINE_H_

#define XINE_ENABLE_EXPERIMENTAL_FEATURES

#include <xine.h>
#include <xine/xineutils.h>
#include <xine/audio_out.h>

typedef struct {
	int time;
	char *artist;
	char *title;
	char *stream;
} stream_track;

//static void event_listener(void*, const xine_event_t*);

int get_xine_meta_info(void);
void get_new_stream_data(char*);
int get_volume(void);
void send_volume(void);
void get_stream_history(void);
void send_stream_history();
int update_progressbar(void);

//update funcitons
void myxine_init(void);
void send_time(void);
int update_gui(void);
int update_progressbar(void);
int set_volume(int);
//get methods
void get_stream_history(void);
void send_stream_history(int);
//control functions
int play(char*);
void play_cursong(void);
int stop(void);
int next(void);
int back(void);
int pausetoggle(void);
int jump_to_pos(int);
void start_xine(void);
void stop_xine(void);
int is_running(void);
void play_stream_now(char*);
void start_xine(void);
void stop_xine(void);

#endif /* SRC_MYXINE_H_ */
