/*
 * audioout.h
 *
 *  Created on: Nov 15, 2018
 *      Author: bob
 */

#ifndef SRC_AUDIOOUT_H_
#define SRC_AUDIOOUT_H_

#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>

#include <ao/ao.h>
#include <FLAC/all.h>
#include <mpg123.h>
#include <vorbis/vorbisfile.h>
#include <sndfile.h>
#include <miniaudio/miniaudio.h>

#define VORBIS_TAG_LEN 30
#define VORBIS_YEAR_LEN 4
/* the main data structure of the FLAC portion */
typedef struct {
    FLAC__StreamDecoder *decoder;
    /* bits, rate, channels, byte_format */
    ao_sample_format sam_fmt; /* input sample's true format */
    ao_sample_format ao_fmt;  /* libao output format */
    unsigned long total_samples;
    unsigned long current_sample;
    float total_time;        /* seconds */
} file_info_struct;

int play_file(const char*);
void ao_stop(void);
void ao_next(void);
void ao_pause(void);
int play_file_finished(void);
float progress(void);
float mp3_progress(void);
float sndfile_progress(void);
float playing_time(void);
#endif /* SRC_AUDIOOUT_H_ */
