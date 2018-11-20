/*
 * flacplay.h
 *
 *  Created on: Nov 12, 2018
 *      Author: bob
 */

#ifndef SRC_FLACPLAY_H_
#define SRC_FLACPLAY_H_

#include <stdio.h>
#include <ao/ao.h>
#include <limits.h>
#include <FLAC/all.h>
#include <pthread.h>

#define VORBIS_TAG_LEN 30
#define VORBIS_YEAR_LEN 4
/* the main data structure of the program */
typedef struct {
    FLAC__StreamDecoder *decoder;
    /* bits, rate, channels, byte_format */
    ao_sample_format sam_fmt; /* input sample's true format */
    ao_sample_format ao_fmt;  /* libao output format */

    ao_device *ao_dev;
    char filename[1024];
    unsigned long total_samples;
    unsigned long current_sample;
    float total_time;        /* seconds */
    float elapsed_time;      /* seconds */
    FLAC__bool is_loaded;    /* loaded or not? */
    FLAC__bool is_playing;   /* playing or not? */
    char title[VORBIS_TAG_LEN+1];
    char artist[VORBIS_TAG_LEN+1];
    char album[VORBIS_TAG_LEN+1];    /* +1 for \0 */
    char genre[VORBIS_TAG_LEN+1];
    char comment[VORBIS_TAG_LEN+1];
    char year[VORBIS_YEAR_LEN+1];
} file_info_struct;

int flacplay(const char*, int);
void flacstop(void);
int poll_playing(void);
float progress(void);
#endif /* SRC_FLACPLAY_H_ */
