/*
 * mtputils.c
 *
*  Copyright: 2016-2017 David Morton
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
#include "mtputils.h"

#include "myutils.h"
#include "sqlite.h"

static int num_devices;

struct arg_struct {
	char dist[16];
	int thresh;
	int var;
	int time;
	char time_s[10];
	int data;
	char data_s[4];
};

static void clear_files(LIBMTP_mtpdevice_t*, uint32_t);
static int del_file(LIBMTP_mtpdevice_t*, uint32_t);
static int add_file(LIBMTP_mtpdevice_t*, uint32_t , char*, struct stat);
static void* get_songs_for_phone(void*);

LIBMTP_raw_device_t* connect_to_phone()
{
	LIBMTP_raw_device_t *rawdevices;
	int numrawdevices;
	LIBMTP_error_number_t errnum;

	LIBMTP_Init();

	errnum = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
	switch(errnum) {
	case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
		printf("No device attached\n");
		return NULL;
	case LIBMTP_ERROR_CONNECTING:
		printf("Error connecting\n");
		return NULL;
	case LIBMTP_ERROR_MEMORY_ALLOCATION:
		printf("Memory allocation error\n");
		return NULL;
	case LIBMTP_ERROR_NONE:	{
		printf("Found %d devices\n", numrawdevices);
		num_devices = numrawdevices;
		return rawdevices;
	}
	default:
		printf("Unknown error %d\n", errnum);
		return NULL;
	}
}
//deletes all the files from the folder with id = folder_id - should just be the Music folder
void clear_files(LIBMTP_mtpdevice_t *device, uint32_t folder_id) {
	LIBMTP_file_t *file;
	int count = 0;
	file = LIBMTP_Get_Filelisting_With_Callback(device, NULL, NULL);
	while(file != NULL) {
		if(file->parent_id == folder_id) {
			del_file(device, file->item_id);
			printf("Del %s\n", file->filename);
			count++;
		}
		file = file->next;
	}
	printf("Deleted %d files\n", count);
	LIBMTP_destroy_file_t(file);
}
int del_file(LIBMTP_mtpdevice_t *device, uint32_t id) {
	if(id == 0) {
		printf("id is zero, bailing\n");
		return -1;
	}
	int ret = LIBMTP_Delete_Object(device, id);
	if(ret != 0) {
		printf("Failed to deleted item %d\n", id);
		LIBMTP_Dump_Errorstack(device);
		LIBMTP_Clear_Errorstack(device);
		return ret;
	}
	printf(".");
	return 0;
}
int add_file(LIBMTP_mtpdevice_t *device, uint32_t folder_id, char *file, struct stat st) {
	off_t filesize = st.st_size;
	int type = check_file(file);
	int filetype;
	switch(type) {
	case 0://ogg
		filetype = LIBMTP_FILETYPE_OGG;
		break;
	case 1://mp3
		filetype = LIBMTP_FILETYPE_MP3;
		break;
	case 2://flac
		filetype = LIBMTP_FILETYPE_FLAC;
		break;
	default://skip everything else
		return -1;
	}
	char *album = malloc(TAG_SIZE * sizeof(char));
	memset(album, 0, TAG_SIZE);
	char *title = malloc(TAG_SIZE * sizeof(char));
	memset(title, 0, TAG_SIZE);
	char *artist = malloc(TAG_SIZE * sizeof(char));
	memset(artist, 0, TAG_SIZE);
	char *date = malloc(TAG_SIZE * sizeof(char));
	memset(date, 0, TAG_SIZE);
	char *genre = malloc(TAG_SIZE * sizeof(char));
	memset(date, 0, TAG_SIZE);

	LIBMTP_track_t *trackfile = LIBMTP_new_track_t();
	trackfile->filesize = filesize;
	trackfile->filetype = filetype;
	trackfile->parent_id = folder_id;
	if(get_tag_from_song(file, album, "TALB") > 0)
		trackfile->album = album;
	else
		trackfile->album = NULL;
	if(get_tag_from_song(file, title, "TIT2") > 0)
		trackfile->title = title;
	else
		trackfile->title = NULL;
	if(get_tag_from_song(file, artist, "TPE1") > 0)
		trackfile->artist = artist;
	else
		trackfile->artist = NULL;
	if(get_tag_from_song(file, date, "TYER") > 0)
		trackfile->date = date;
	else
		trackfile->date = NULL;
	if(get_tag_from_song(file, genre, "TCON") > 0)
		trackfile->genre = genre;
	else
		trackfile->genre = NULL;

	trackfile->tracknumber = 0;//FIXME could add a tracknumber if I cared, not really necessary at the moment
	char *base = strdup(file);
	trackfile->filename = basename(base);

	int ret = LIBMTP_Send_Track_From_File(device, file, trackfile, NULL, NULL);
	printf("adding track %s\n", file);
	printf("\tfilesize = %li\n", filesize);
	printf("\tfiletype = %d\n", filetype);
	printf("\talbum = %s\n", album);
	printf("\ttitle = %s\n", title);
	printf("\tartist = %s\n", artist);
	printf("\tdate = %s\n", trackfile->date);
	printf("\tgenre = %s\n", trackfile->genre);
	if(ret != 0) {
		printf("File transfer failed");
		LIBMTP_Dump_Errorstack(device);
		LIBMTP_Clear_Errorstack(device);
	}
	if(album != NULL)
		free(album);
	if(title != NULL)
		free(title);
	if(artist != NULL)
		free(artist);
	if(date != NULL)
		free(date);
	if(genre != NULL)
		free(genre);
	//LIBMTP_destroy_track_t(trackfile);
	if(base != NULL)
		free(base);
	return 0;
}
/*
 * This function looks at song and album weights, generates a random subset
 * of songs and copies them to a separate directory.
 */
void* get_songs_for_phone(void* data_args)
{
	struct stat st;
	long int size_of_list = 0;
	int i;

	struct arg_struct *args = (struct arg_struct *) data_args;
	int time_mult = 0;
	int k;
	for(k = 0; k < 10; k++)
		printf("%c\t", args->time_s[k]);
	printf("args->time_s %s \n", args->time_s);
	if(!strncmp(args->time_s, "days", 4))
		time_mult = args->time * 3600 * 24;
	else if (!strncmp(args->time_s, "months", 6))
		time_mult = args->time * 3600 * 24 * 30;
	else if (!strncmp(args->time_s, "years", 5))
		time_mult = args->time * 3600 * 24 * 365;
	else
		time_mult = 3600 * 24 * 365 * 20;//20 years - FIXME - should be earliest date in database

	//connect to the phone first
	LIBMTP_raw_device_t *rawdevices;
	rawdevices = connect_to_phone();
	if(rawdevices == NULL) {
		printf("Could not connect\n");
		return data_args;
	}
	printf("Connected to %ud\n", rawdevices->devnum);
	song *data = malloc(20000*sizeof(song));
	int num_recent_songs = get_recent_songs(&data, time_mult);//get all songs added in the last time_mult seconds
	int song_pool[num_recent_songs];//a list of indicies of songs in data that pass the weight calculation

	float (*f)(int, int, int);
	if (!strcmp(args->dist, "exponential"))
		f = &exponential;
	else if (!strcmp(args->dist, "gaussian"))
		f = &gaussian;
	else if (!strcmp(args->dist, "flat"))
		f = &flat;
	else
		f = &linear;
	int i_pool = 0;
	printf("checking %d songs\n", num_recent_songs);
	for(int i = 0; i < num_recent_songs; i++)
	{
		if(stat(data[i].file, &st) != -1)
		{
			float prob = f(data[i].weight, args->thresh, args->var);
			float dice_roll = (float)random() / (float)RAND_MAX;
			if(prob > dice_roll)
			{
				song_pool[i_pool++] = i;
				size_of_list += st.st_size;
			}
		}
		else
			printf("Couldn't stat song\n");
	}
	printf("Size = %ld b = %ld Mb\n", size_of_list, size_of_list / 1024 / 1024);

	//shuffle the list of songs
	for(int i = i_pool - 1; i > 0; i--)
	{
		int rand = (int)(random() % i);
		int temp = song_pool[i];
		song_pool[i] = song_pool[rand];
		song_pool[rand] = temp;
	}

	//FIXME: pick a particular device in the GUI
	for(i = 0; i < num_devices; i++) {
		LIBMTP_mtpdevice_t *device;
		LIBMTP_devicestorage_t *storage;
		int ret;

		printf("Opening device...");
		device = LIBMTP_Open_Raw_Device(&rawdevices[i]);
		if(device == NULL){
			printf("Unable to open device %d\n", i);
			return data_args;
		}
		printf("opened\n");

		printf("getting storage folders...");
		ret = LIBMTP_Get_Storage(device, LIBMTP_STORAGE_SORTBY_NOTSORTED);
		if(ret != 0)
		{
			printf("Failed to get folder listing\n");
			LIBMTP_Dump_Errorstack(device);
			LIBMTP_Clear_Errorstack(device);
			return data_args;
		}
		printf("done.\nLooking for music folder.\n");
		for(storage = device->storage; storage != 0; storage = storage->next) {
			LIBMTP_folder_t *folder;
			printf("Storage: %s\n", storage->StorageDescription);
			folder = LIBMTP_Get_Folder_List_For_Storage(device, storage->id);
			if(folder == NULL) {
				printf("No folders found\n");
				LIBMTP_Dump_Errorstack(device);
				LIBMTP_Clear_Errorstack(device);
			}
			else {
				long int total_bytes = 0;
				long int data_mult = 0;
				if(!strcmp(args->data_s, "Mb"))
					data_mult = 1024 * 1024;
				//FIXME data overflow
				//else if (!strcmp(args->data_s, "Tb"))
				//	data_mult = 1024 * 1024 * 1024 * 1024;
				else
					data_mult = 1024 * 1024 * 1024;
				long int max_bytes = data_mult * args->data;
				while(folder != NULL) {
					printf("%s\n", folder->name);
					//found the right folder, clean it out, dump in new stuff and bail
					if(!strcmp(folder->name, "Music")) {
						printf("deleting old files");
						clear_files(device, folder->folder_id);
						printf("\n");
						//add new files to the device
						i = 0;
						while(total_bytes < max_bytes && i < i_pool) {
							if(!stat(data[song_pool[i]].file, &st) && !add_file(device, folder->folder_id, data[song_pool[i]].file, st))
								total_bytes += st.st_size;
							i++;
						}
						if(folder != NULL) LIBMTP_destroy_folder_t(folder);
						break;
					}
					folder = folder->sibling;
				}
				printf("finished: total bytes = %ld\t, num songs = %d\n", total_bytes, i);
			}
		}
		printf("Releasing device\n");
		LIBMTP_Release_Device(device);
	}
	printf("freeing raw devices\n");
	free(rawdevices);
	free(data);
	return data_args;
}
int phone_thread_init(char dist[], int thresh, int var, int time, char time_s[], int data, char data_s[]) {
	struct arg_struct args;
	args.dist[15] = '\0';
	args.time_s[9] = '\0';
	args.data_s[3] = '\0';
	memcpy(args.dist, dist, strlen(dist));
	args.dist[strlen(dist)] = '\0';
	args.thresh = thresh;
	args.var = var;
	args.time = time;
	memcpy(args.time_s, time_s, strlen(time_s));
	args.time_s[strlen(time_s)] = '\0';
	args.data = data;
	memcpy(args.data_s, data_s, strlen(data_s));
	args.data_s[strlen(data_s)] = '\0';
	pthread_t phone_thread;
	printf("Starting phone thread\n");
	if(pthread_create(&phone_thread, NULL, get_songs_for_phone, (void *)&args))
	{
		fprintf(stderr, "Couldn't create thread\n");
		return 1;
	}
	return 0;
}

