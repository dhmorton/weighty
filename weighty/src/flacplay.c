/*
 * flacplay.c
 *
 *  Created on: Nov 12, 2018
 *      Author: bob
 *  Copyright: 2011-2018 David Morton
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
 *
 *   Most of this file is extracted from the flac123 code
 */

#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include "flacplay.h"

file_info_struct file_info = { NULL, {0,0,0,0}, {0,0,0,0}, NULL, "", 0,0,0,0, false };
struct arg_struct {
	char filename[1024];
	int ao_driver;
};

void* play_file(void *);
void flac_error_hdl(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
void flac_metadata_hdl(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *, void *);
FLAC__StreamDecoderWriteStatus flac_write_hdl(const FLAC__StreamDecoder *, const FLAC__Frame *, const FLAC__int32 * const buf[], void *);

static int interrupted = 0;
int finished = 0;
pthread_t flac_thread;
float total_time;
float playing_time;

ao_option **ao_options = NULL;

int flacplay(const char *filename, int ao_driver)
{
    ao_options = malloc(1024);
    *ao_options = NULL;

	struct arg_struct args;
	memcpy(args.filename, filename, strlen(filename));
	args.ao_driver = ao_driver;
	finished = 0;
	printf("forking thread...\n");
    if(pthread_create(&flac_thread, NULL, play_file, (void *)&args))
    {
    	fprintf(stderr, "Couldn't create thread\n");
    	return 1;
    }

    return 0;
}
void flacstop()
{
	interrupted = 1;
	//pthread_cancel(flac_thread);
}
float progress()
{
	return (total_time != 0) ? playing_time / total_time : 0;
}
int poll_playing()
{
	return finished;
}
FLAC__bool decoder_constructor(const char *filename, int ao_driver)
{
    int len = strlen(filename);
    static ao_sample_format previous_bitrate;

    file_info.filename[len] = '\0';
    strncpy(file_info.filename, filename, len);

    memset(file_info.title, ' ', VORBIS_TAG_LEN);
    file_info.title[VORBIS_TAG_LEN] = '\0';
    memset(file_info.artist, ' ', VORBIS_TAG_LEN);
    file_info.artist[VORBIS_TAG_LEN] = '\0';
    memset(file_info.album, ' ', VORBIS_TAG_LEN);
    file_info.album[VORBIS_TAG_LEN] = '\0';
    memset(file_info.genre, ' ', VORBIS_TAG_LEN);
    file_info.genre[VORBIS_TAG_LEN] = '\0';
    memset(file_info.comment, ' ', VORBIS_TAG_LEN);
    file_info.comment[VORBIS_TAG_LEN] = '\0';
    memset(file_info.year, ' ', VORBIS_YEAR_LEN);
    file_info.year[VORBIS_YEAR_LEN] = '\0';

    /* create and initialize flac decoder object */
    file_info.decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(file_info.decoder, true);

    /* read metadata */
    if ((FLAC__stream_decoder_init_file(file_info.decoder, filename, flac_write_hdl, flac_metadata_hdl, flac_error_hdl, (void *)&file_info) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	|| (!FLAC__stream_decoder_process_until_end_of_metadata(file_info.decoder)))
    {
    	FLAC__stream_decoder_delete(file_info.decoder);
    	return false;
    }

	//ao_close(file_info.ao_dev);
	if (!(file_info.ao_dev = ao_open_live(ao_driver, &(file_info.ao_fmt), *ao_options)))
	{
		fprintf(stderr, "Error opening ao device %d\n", ao_driver);
		FLAC__stream_decoder_delete(file_info.decoder);
		return false;
	}

    previous_bitrate.bits = file_info.ao_fmt.bits;
    previous_bitrate.rate = file_info.ao_fmt.rate;
    previous_bitrate.channels = file_info.ao_fmt.channels;

    file_info.is_loaded  = true;
    file_info.is_playing = true;

    return true;
}

void decoder_destructor(void)
{
    FLAC__stream_decoder_finish(file_info.decoder);
    FLAC__stream_decoder_delete(file_info.decoder);
    file_info.is_loaded  = false;
    file_info.is_playing = false;
    file_info.filename[0] = '\0';
    ao_close(file_info.ao_dev);
}

void* play_file(void* data_args)
{
	struct arg_struct *args = (struct arg_struct *) data_args;
    if (!decoder_constructor(args->filename, args->ao_driver))
    {
    	fprintf(stderr, "Error opening %s\n", args->filename);
    	return data_args;
    }

    while (FLAC__stream_decoder_process_single(file_info.decoder) == true &&
	   FLAC__stream_decoder_get_state(file_info.decoder) <
	   FLAC__STREAM_DECODER_END_OF_STREAM && !interrupted)
    {
    }
    interrupted = 0; /* more accurate feedback if placed after loop */

    decoder_destructor();
	//ao_shutdown();
    finished = 1;
    printf("FLAC thread finished\n");
    return data_args;
}

void flac_error_hdl(const FLAC__StreamDecoder *dec,
		    FLAC__StreamDecoderErrorStatus status, void *data)
{
    fprintf(stderr, "error handler called!\n");
}

void flac_metadata_hdl(const FLAC__StreamDecoder *dec,
		       const FLAC__StreamMetadata *meta, void *data)
{
    file_info_struct *p = (file_info_struct *) data;

    if(meta->type == FLAC__METADATA_TYPE_STREAMINFO) {
	p->sam_fmt.bits = p->ao_fmt.bits = meta->data.stream_info.bits_per_sample;
	p->ao_fmt.rate = meta->data.stream_info.sample_rate;
	p->ao_fmt.channels = meta->data.stream_info.channels;
	p->ao_fmt.byte_format = AO_FMT_NATIVE;
	//FLAC__ASSERT(meta->data.stream_info.total_samples < 0x100000000); /* we can handle < 4 gigasamples */
	p->total_samples = (unsigned)
	    (meta->data.stream_info.total_samples & 0xffffffff);
	p->current_sample = 0;
	p->total_time = (((float) p->total_samples) / p->ao_fmt.rate);
	p->elapsed_time = 0;
    }
}

FLAC__StreamDecoderWriteStatus flac_write_hdl(const FLAC__StreamDecoder *dec,
					      const FLAC__Frame *frame,
					      const FLAC__int32 * const buf[],
					      void *data)
{
    int sample, channel, i;
    uint_32 samples = frame->header.blocksize;
    file_info_struct *p = (file_info_struct *) data;
    uint_32 decoded_size = frame->header.blocksize * frame->header.channels
	* (p->ao_fmt.bits / 8);
    static uint_8 aobuf[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS *
			sizeof(uint_32) / 4]; /*oink!*/
    uint_16 *u16aobuf = (uint_16 *) aobuf;
    uint_8   *u8aobuf = (uint_8  *) aobuf;
    float elapsed, remaining_time;

    if (p->sam_fmt.bits == 8) {
        for (sample = i = 0; sample < samples; sample++) {
			for(channel = 0; channel < frame->header.channels; channel++,i++) {
			   u8aobuf[i] = buf[channel][sample];
			}
        }
    }
    else if (p->sam_fmt.bits == 16) {
        for (sample = i = 0; sample < samples; sample++) {
			for(channel = 0; channel < frame->header.channels; channel++,i++) {
			u16aobuf[i] = (uint_16)(buf[channel][sample]);
			}
        }
    }
    ao_play(p->ao_dev, (char*) aobuf, decoded_size);

    p->current_sample += samples;
    elapsed = ((float) samples) / frame->header.sample_rate;
    p->elapsed_time += elapsed;
    if ((remaining_time = p->total_time - p->elapsed_time) < 0)
    	remaining_time = 0;
    total_time = p->total_time;
    playing_time = p->elapsed_time;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}
