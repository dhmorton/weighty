/*
 * audioout.c
 *
 *  Created on: Nov 15, 2018
 *      Author: bob
 */

#include "myutils.h"
#include "audioout.h"

//FLAC stuff
static file_info_struct file_info = { NULL, {0,0,0,0}, {0,0,0,0}, 0, 0, 0};
struct arg_struct {
	char filename[1024];
};
static float FLAC_total_time = 0;
static float FLAC_playing_time = 0;

//mp3 stuff
#define BITS 8
float mp3_total_time;
off_t mp3_playing_time;

//libsndfile stuff
int items_read = 0;
int total_items = 0;
float total_time = 0.0;

//pthread stuff
int interrupt = 0;
static int finished = 0;
static int paused = 0;
pthread_t playing_thread = 0;

//libao stuff
static int ao_driver = -1;
static ao_device *ao_dev = NULL;
static int playing = 0;

int extension = -1;

static void* play_FLAC(void *);
static void* play_mp3(void*);
static void* play_ogg(void*);
static void* play_sndfile(void*);
static float flac_progress(void);
static void data_callback(ma_device*, void*, const void*, ma_uint32);

FLAC__bool decoder_constructor(const char *, int);
void flac_error_hdl(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
void flac_metadata_hdl(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *, void *);
FLAC__StreamDecoderWriteStatus flac_write_hdl(const FLAC__StreamDecoder *, const FLAC__Frame *, const FLAC__int32 * const buf[], void *);

int play_file(const char *filename)
{
	if(!playing) {
		ao_initialize();
		playing = 1;
	}
	extension = check_file(filename);
	void* (*play_func)(void*);
	switch(extension) {
	case 0:
		play_func = &play_ogg;
		break;
	case 1:
		play_func = &play_mp3;
		break;
	case 2:
		play_func = &play_FLAC;
		break;
	default:
		play_func = &play_sndfile;
	}
	if(playing_thread != 0) {
		if(!pthread_join(playing_thread, NULL)) {
			//printf("pthread_join error %s\n", strerror(errno));
		}
	}
	struct arg_struct args;
	memcpy(args.filename, filename, strlen(filename));
	if(ao_driver < 0)
			ao_driver = ao_driver_id("alsa");
	finished = 0;
	if(pthread_create(&playing_thread, NULL, play_func, (void *)&args))
	{
		printf("Couldn't create thread %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
void ao_pause() {
	paused = !paused;
}
void ao_next() {
	interrupt = 1;
}
void ao_stop() {
	interrupt = 1;
	playing = 0;
}
float mp3_progress() {
	return (mp3_total_time != 0) ? (float) mp3_playing_time / mp3_total_time : 0;
}
float playing_time() {
	switch(extension) {
	case 1:
		return mp3_total_time;
	case 2:
		return FLAC_total_time;
	default:
		return total_time;
	}
}
float progress() {
	switch(extension) {
	case 1:
		return mp3_progress();
	case 2:
		return flac_progress();
	default:
		return sndfile_progress();
	}
	//return (FLAC_total_time != 0) ? FLAC_playing_time / FLAC_total_time : 0;
}
float sndfile_progress() {
	return (total_items != 0) ? (float) items_read / total_items : 0.0;
}
float flac_progress() {
	return (FLAC_total_time != 0) ? FLAC_playing_time / FLAC_total_time : 0.0;
}
int play_file_finished() {
	return finished;
}
void* play_sndfile(void* data_args)
{
	printf("\tSNDFILE playing\n");
	struct arg_struct *args = (struct arg_struct *) data_args;
	ao_sample_format format;
	SF_INFO sfinfo;
	SNDFILE *file = sf_open(args->filename, SFM_READ, &sfinfo);
	int buf_size = 8192;
	short* buffer = calloc(buf_size, sizeof(short));
	ao_initialize();
	ao_driver = ao_driver_id("alsa");
	format.rate = 44100;
	format.channels = sfinfo.channels;
	format.bits = 16;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = NULL;
	ao_dev = ao_open_live(ao_driver, &format, NULL);
	if(ao_dev == NULL) {
		printf("unable to open device\n");
		return 0;
	}
	int read;
	items_read = 0;
	total_items = sfinfo.frames * sfinfo.channels;
	total_time = (float) sfinfo.frames / sfinfo.samplerate;
	while((read = sf_read_short(file, buffer, buf_size)) != 0 && !interrupt) {
		items_read += read;
		ao_play(ao_dev, (char*) buffer, (int) (read * sizeof(short)));
		while(paused) {
			usleep(20);
		}
	}
	interrupt = 0;
	if(buffer != NULL)
		free(buffer);
	sf_close(file);
	ao_close(ao_dev);
	finished = 1;
	return data_args;
}
void* play_ogg(void* data_args)
{
	struct arg_struct *args = (struct arg_struct *) data_args;
	printf("\tOGG %s\n", args->filename);
	ao_driver = ao_driver_id("alsa");
	ao_sample_format format;
	format.rate = 44100;
	format.channels = 2;
	format.bits = 16;
	format.byte_format = AO_FMT_LITTLE;
	format.matrix = NULL;
	ao_dev = ao_open_live(ao_driver, &format, NULL);
	if(ao_dev == NULL) {
		printf("unable to open device\n");
		return 0;
	}

	OggVorbis_File vf;
	int bitstream;
	int open;
	int buf_size = 4096;
	char* buffer = malloc(buf_size);
	if((open = ov_fopen(args->filename, &vf)) != 0) {
		printf("fopen failed %s\n", strerror(errno));
	}
	interrupt = 0;
	long read;
	while((read = ov_read(&vf, buffer, buf_size, 0, 2, 1, &bitstream)) != 0 && !interrupt)
	{
		ao_play(ao_dev, buffer, (int) read);
		while(paused) {
			usleep(20);
		}
	}
	interrupt = 0;
	finished = 1;
	if(buffer != NULL)
		free(buffer);
	ov_clear(&vf);
	ao_close(ao_dev);
	return data_args;
}
void* play_mp3(void* data_args)
{
	mpg123_handle *mh;
	unsigned char *buffer;
	size_t buffer_size;
	size_t done;
	int err;

	ao_sample_format format;
	int channels, encoding;
	long rate;

	struct arg_struct *args = (struct arg_struct *) data_args;

	 /* initializations */
	interrupt = 0;
	mpg123_init();
	mh = mpg123_new(NULL, &err);
	buffer_size = mpg123_outblock(mh);
	buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
	mpg123_open(mh, args->filename);
	mpg123_getformat(mh, &rate, &channels, &encoding);
	printf("encoding = %d\n", encoding * BITS);


	/* set the output format and open the output device */
	format.bits = mpg123_encsize(encoding) * BITS;
	format.rate = rate;
	format.channels = channels;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;
	printf("rate = %d\n", format.rate);
	int count = 0;
	while(count < 3) {
		ao_dev = ao_open_live(ao_driver, &format, NULL);
		if(ao_dev == NULL)	{
			printf("ao_open_live error %s\n", strerror(errno));
			count++;
		}
		else
			count = 3;
	}
	struct mpg123_frameinfo mi;
	mpg123_info(mh, &mi);
	mp3_total_time = (float) mpg123_length(mh) / mi.rate;
	printf("mp3 seconds = %f\n", mp3_total_time);
	/* decode and play */
	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK && !interrupt)
	{
		while(paused) {
			usleep(20);
		}
		ao_play(ao_dev, (char *) buffer, done);
		mp3_playing_time = mpg123_tell(mh) / rate;
	}
	interrupt = 0;
	/* clean up */
	free(buffer);
	ao_close(ao_dev);
	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();
	printf("MP3 thread finished\n\n");
	finished = 1;
	return data_args;
}
void* play_FLAC(void* data_args)
{
	FLAC_total_time = 0;
	FLAC_playing_time = 0;
	interrupt = 0;
	struct arg_struct *args = (struct arg_struct *) data_args;
    if (!decoder_constructor(args->filename, ao_driver))
    {
    	fprintf(stderr, "Error opening %s\n", args->filename);
    	return data_args;
    }

    while (FLAC__stream_decoder_process_single(file_info.decoder) == true &&
	   FLAC__stream_decoder_get_state(file_info.decoder) <
	   FLAC__STREAM_DECODER_END_OF_STREAM && !interrupt)
    {
    	while(paused) {
    		usleep(20);
    	}
    }
    interrupt = 0; /* more accurate feedback if placed after loop */

    //clean up
    FLAC__stream_decoder_finish(file_info.decoder);
    FLAC__stream_decoder_delete(file_info.decoder);
    ao_close(ao_dev);
    finished = 1;
    return data_args;
}
FLAC__bool decoder_constructor(const char *filename, int ao_driver)
{
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
	if (!(ao_dev = ao_open_live(ao_driver, &(file_info.ao_fmt), NULL)))
	{
		fprintf(stderr, "Error opening ao device %d\n", ao_driver);
		FLAC__stream_decoder_delete(file_info.decoder);
		return false;
	}
    return true;
}
void flac_error_hdl(const FLAC__StreamDecoder *dec,
		    FLAC__StreamDecoderErrorStatus status, void *data)
{
    printf("error handler called!\n");
}
void flac_metadata_hdl(const FLAC__StreamDecoder *dec,
		       const FLAC__StreamMetadata *meta, void *data)
{
    file_info_struct *p = (file_info_struct *) data;

    if(meta->type == FLAC__METADATA_TYPE_STREAMINFO) {
		p->sam_fmt.bits = p->ao_fmt.bits = meta->data.stream_info.bits_per_sample;
		if(p->ao_fmt.bits == 24) {
		    p->ao_fmt.bits = 16;
		    printf("Downgrading from 24 to 16 bit\n");
		}
		p->ao_fmt.rate = meta->data.stream_info.sample_rate;
		p->ao_fmt.channels = meta->data.stream_info.channels;
		p->ao_fmt.byte_format = AO_FMT_NATIVE;
		//FLAC__ASSERT(meta->data.stream_info.total_samples < 0x100000000); /* we can handle < 4 gigasamples */
		p->total_samples = (unsigned)
			(meta->data.stream_info.total_samples & 0xffffffff);
		p->current_sample = 0;
		p->total_time = (((float) p->total_samples) / p->ao_fmt.rate);
	    FLAC_total_time = p->total_time;
    }
}
FLAC__StreamDecoderWriteStatus flac_write_hdl(const FLAC__StreamDecoder *dec, const FLAC__Frame *frame, const FLAC__int32 * const buf[], void *data)
{
    int sample, channel, i;
    uint_32 samples = frame->header.blocksize;
    file_info_struct *p = (file_info_struct *) data;
    uint_32 decoded_size = frame->header.blocksize * frame->header.channels	* (p->ao_fmt.bits / 8);
    //from flac-1.3.2 decode.c
	static union
	{	/* The arrays defined within this union are all the same size. */
		FLAC__uint8  u8buffer	[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int32)];
		FLAC__uint16 u16buffer	[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(FLAC__int16)];
		FLAC__uint32 u32buffer	[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS];
	} ubuf;

    float elapsed, remaining_time;

    if (p->sam_fmt.bits == 8) {
        for (sample = i = 0; sample < samples; sample++) {
			for(channel = 0; channel < frame->header.channels; channel++,i++) {
			   ubuf.u8buffer[i] = buf[channel][sample];
			}
        }
    }
    else if (p->sam_fmt.bits == 16) {
        for (sample = i = 0; sample < samples; sample++) {
			for(channel = 0; channel < frame->header.channels; channel++,i++) {
				ubuf.u16buffer[i] = (uint_16)(buf[channel][sample]);
			}
        }
        ao_play(ao_dev, (char*) ubuf.u8buffer, decoded_size);
    }
    else if(p->sam_fmt.bits == 24) {
    	for(sample = i = 0; sample < samples; sample++) {
    		for(channel = 0; channel < frame->header.channels; channel++, i++)
    			//take off the lowest 8 bits to squeze it into a 16 bit output
    			ubuf.u16buffer[i] = (uint_16)(buf[channel][sample] >> 8);
    	}
        ao_play(ao_dev, (char*) ubuf.u8buffer, decoded_size);
    }
    else {
    	printf("%d bit FLAC\n", p->sam_fmt.bits);
    }

    p->current_sample += samples;
    elapsed = ((float) samples) / frame->header.sample_rate;
    FLAC_playing_time += elapsed;
    if ((remaining_time = p->total_time - FLAC_playing_time) < 0)
    	remaining_time = 0;
    FLAC_total_time = p->total_time;
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}
