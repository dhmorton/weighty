/*
 * mp3play.c
 *
 *  Created on: Nov 12, 2018
 *      Author: bob
 *
 *  Copyright: 2016-2018 David Morton
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
 *   This code adopted from http://hzqtc.github.io/2012/05/play-mp3-with-libmpg123-and-libao.html
 */

#include "mp3play.h"

#define BITS 8
struct arg_struct {
	char filename[1024];
};
pthread_t mp3_thread;
int finished;
void* play_mp3file(void*);
off_t playing_time;
off_t total_time;
int driver = -1;
int interrupt;

int mp3play(char* filename)
{
	finished = 0;
	struct arg_struct args;
	memcpy(args.filename, filename, strlen(filename));
	if(pthread_create(&mp3_thread, NULL, play_mp3file, (void *)&args))
	{
		fprintf(stderr, "Couldn't create thread\n");
		return 1;
	}

    return 0;
}
void mp3stop()
{
	interrupt = 1;
	//pthread_cancel(mp3_thread);
}
float mp3_progress()
{
	return (total_time != 0) ? (float) playing_time / total_time : 0;
}
void* play_mp3file(void* data_args)
{
	mpg123_handle *mh;
	unsigned char *buffer;
	size_t buffer_size;
	size_t done;
	int err;

	ao_device *dev;

	ao_sample_format format;
	int channels, encoding;
	long rate;

	struct arg_struct *args = (struct arg_struct *) data_args;

	 /* initializations */
	interrupt = 0;
	if(driver < 0)
		driver = ao_driver_id("alsa");
	printf("mp3driver = %d\n", driver);
	mpg123_init();
	mh = mpg123_new(NULL, &err);
	buffer_size = mpg123_outblock(mh);
	buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
	mpg123_open(mh, args->filename);
	mpg123_getformat(mh, &rate, &channels, &encoding);

	/* set the output format and open the output device */
	format.bits = mpg123_encsize(encoding) * BITS;
	format.rate = rate;
	format.channels = channels;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;
	dev = ao_open_live(driver, &format, NULL);
	if(dev == NULL)
	{
		printf("ao_open_live error %s\n", strerror(errno));
	}
	total_time = mpg123_length(mh);
	printf("mp3 samples = %li\n", total_time);
	/* decode and play */
	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK && interrupt == 0)
	{
		ao_play(dev, (char *) buffer, done);
		playing_time = mpg123_tell(mh);
		printf("%d %li\n", interrupt, playing_time);
	}
	/* clean up */
	free(buffer);
	ao_close(dev);
	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();
	//ao_shutdown();
	printf("MP3 thread finished\n");
	finished = 1;
	return data_args;
}
int mp3_finished()
{
	return finished;
}
