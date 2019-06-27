/*
 * songhash.c
 *
 *  Created on: Jul 4, 2011
 *      Author: bob
 */
/*
 * This file creates a hash for a quick weight/sticky lookup by file name.
 * A non-unique key between 0 and 9999 is generated for each file.
 * An array of 10,000 pointers to arrays of pointers to song structs is referenced by the keys.
 */

#include "songhash.h"

#include "myutils.h"
#include "net_utils.h"
#include "sqlite.h"

static hash_element *hash_table[HASH_SIZE];//this is a collection of all songpaths with weight and sticky
static hash_element *history_table[HASH_SIZE];
static added_element *added_table[SMALL_HASH];
static int added_n = -1;
static char *new_file[30000];

static int hash_song(const char*);
static int hash_added(const char*);
static void remove_from_hash(const char*);
static void set_flag(int, const char*);
static void clear_added(void);
static void store_added (const char*);
static void remove_added(const char*);
static int check_against_added(const char*);
static hash_element* get_element(const char*);

//takes a song and returns an int between 0 and 9999 with a simple function
int hash_song(const char *song)
{
	int total = 0;
	int i;
	for (i = 0; i < strlen(song); i++)
		total += i * ((int) song[i]);
	int key = total % HASH_SIZE;
//	printf("total = %d\tkey = %d\n", total, key);
	return key;
}
int hash_added(const char *song)
{
	int total = 0;
	int i;
	for (i = 0; i < strlen(song); i++)
		total += i * ((int) song[i]);
	return (total % SMALL_HASH);
}
int add_to_hash(const char *file, int weight, int sticky, int flag)
{
	if(file == NULL)
		return -1;
	//see if there is anything store there yet
	int key = hash_song(file);
	hash_element *chain;
	int cpos;
	if (hash_table[key] == NULL)
	{
		hash_table[key] = malloc(CHAIN_LENGTH*sizeof(hash_element));
		chain = hash_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
			chain[i].file = NULL;
		cpos = 0;
	}
	else
	{
		chain = hash_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (chain[i].file == NULL)
				break;
		}
		cpos = i;
	}
//	printf("cpos = %d\tkey = %d\n", cpos, key);
	hash_element song_data;
	song_data.file = malloc(strlen(file) + 1);
	memcpy(song_data.file, file, strlen(file) + 1);
	song_data.weight = weight;
	song_data.sticky = sticky;
	song_data.flag = flag;
	chain[cpos] = song_data;

	return 0;
}
void remove_from_hash(const char *file)
{
	if(file == NULL)
		return;
	int key = hash_song(file);
	hash_element *chain = hash_table[key];
	if (chain == NULL)
		printf("null key in remove_from_hash %d %s\n", key, file);
	else
	{
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (strcmp(chain[i].file, file) == 0)
			{
				int j;
				for (j = i; j < CHAIN_LENGTH - 1; j++)
					chain[j] = chain[j + 1];
				break;
			}
		}
	}
}
void change_weight_hash(const char *file, int weight)
{
	if(file == NULL)
		return;
	int key = hash_song(file);
	hash_element *chain;
	if (hash_table[key] == NULL)
		printf("null key in change_weight_hash %d %s\n", key, file);
	else
	{
		chain = hash_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (strcmp(chain[i].file, file) == 0)
			{
				chain[i].weight = weight;
				break;
			}
		}
	}
}
void change_sticky_hash(const char *file, int sticky)
{
	if(file == NULL)
		return;
	int key = hash_song(file);
	hash_element *chain;
	if (hash_table[key] == NULL)
		printf("null key in change_weight_hash %d %s\n", key, file);
	else
	{
		chain = hash_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (strcmp(chain[i].file, file) == 0)
			{
				chain[i].sticky = sticky;
				break;
			}
		}
	}
}
//the history functions keep a hash of the play history
//it's used to exclude already played songs from the playlist
int add_to_history_hash(const char *file)
{
	if(file == NULL)
		return -1;
	int key = hash_song(file);
	hash_element *chain;
	int cpos;
	if (history_table[key] == NULL)
	{
		history_table[key] = malloc(CHAIN_LENGTH*sizeof(hash_element)*8);//FIXME
		chain = history_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
			chain[i].file = NULL;
		cpos = 0;
	}
	else
	{
		chain = history_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (chain[i].file == NULL)
				break;
		}
		cpos = i;
	}
	hash_element song_data;
	song_data.file = malloc(strlen(file) + 1);
	memcpy(song_data.file, file, strlen(file) + 1);
	chain[cpos] = song_data;

	return 0;
}
//returns 1 if the song is in the history otherwise returns zero
int check_song_in_history(const char *file)
{
	if(file == NULL)
		return -1;
	int key = hash_song(file);
	hash_element *chain = history_table[key];
	if (chain == NULL)
		return 0;
	else
	{
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (chain[i].file == NULL)
				return 0;
			else if (strcmp(chain[i].file, file) == 0)
				return 1;
		}
	}
	return 0;
}
void clear_history()
{
	hash_element *chain;
	int key;
	for (key = 0; key < HASH_SIZE; key++)
	{
		if (history_table[key] != NULL)
		{
			chain = history_table[key];
			int i;
			for (i = 0; i < CHAIN_LENGTH; i++)
			{
				if (chain[i].file != NULL)
					free(chain[i].file);
				else
					break;
			}
			history_table[key] = NULL;
		}
	}
}
//the _added functions keep track of newly found songs when updating
//they get checked for new/moved/deleted and then the hash is cleared
void init_added()
{
	int key;
	for (key = 0; key < SMALL_HASH; key++)
		added_table[key] = NULL;
	added_n = 0;
}
void clear_added()
{
	added_element *chain;
	int key;
	for (key = 0; key < SMALL_HASH; key++)
	{
		if (added_table[key] != NULL)
		{
			chain = added_table[key];
			int i;
			for (i = 0; i < CHAIN_LENGTH; i++)
			{
				if (chain[i].file != NULL)
					free(chain[i].file);
				else
					break;
				if (chain[i].path != NULL)
					free(chain[i].path);
			}
			added_table[key] = NULL;
		}
	}
	added_n = -1;
}
void store_added (const char *file)
{
	if(file == NULL)
		return;
	//see if there is anything store there yet
	int key = hash_added(file);
//	printf("added %d %s\n", key, file);
	added_element *chain;
	int cpos;
	if (added_table[key] == NULL)
	{
		added_table[key] = malloc(CHAIN_LENGTH*sizeof(added_element));
		chain = added_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			chain[i].path = NULL;
			chain[i].file = NULL;
		}
		cpos = 0;
	}
	else
	{
		chain = added_table[key];
		int i;
		for (i = 0; i < CHAIN_LENGTH; i++)
		{
			if (chain[i].path == NULL)
				break;
		}
		cpos = i;
	}
	added_element song_data;
	song_data.path = malloc(strlen(file) + 1);
	memset(song_data.path, 0, strlen(file) + 1);
	memcpy(song_data.path, file, strlen(file) + 1);
	const char *s = strrchr(file, '/');
	song_data.file = malloc(strlen(s) + 1);
	memcpy(song_data.file, s, strlen(s) + 1);
	chain[cpos] = song_data;
}
void remove_added(const char* file)
{
	int key = hash_added(file);
//	printf("remove %d %s\n", key, file);
	added_element *chain;
	chain = added_table[key];
	if (chain == NULL)
		printf("null chain shouldn't happen\n");
	int i;
	for (i = 0; i < CHAIN_LENGTH; i++)
	{
		if (chain[i].path == NULL)
			break;
		if (strcmp(chain[i].path, file) == 0)
			chain[i].file = NULL;
	}
}
void set_flag(int flag, const char *file)
{
	hash_element* entry = get_element(file);
	if (entry == NULL)
		printf("couldn't get file to set flag\n");
	else
		entry->flag = flag;
}
int retrieve_by_song(const char *file, int *a)
{
	hash_element *entry = get_element(file);
	if (entry == NULL)
		return -1;
	a[0] = entry->weight;
	a[1] = entry->sticky;

	return 0;
}
int check_song(const char *file)
{
	if (check_file(file) < 0)
		return -1;
	hash_element *entry = get_element(file);
	if (entry == NULL)//store it in the hash and in an array of new songs and an array of filenames for comparison
	{
		store_added(file);
		add_to_hash(file, 100, 0, 2);
//		printf("added_n = %d\n", added_n);
		new_file[added_n] = malloc(strlen(file) + 1);
		memcpy(new_file[added_n], file, strlen(file) + 1);
		added_n++;
	}
	else
		set_flag(1, file);//mark as seen

	return 0;
}
int check_all_songs_in_hash(void)
{
	int change = 0;//flag to see if anything has changed or not
	hash_element *chain;
	int hkey;
	//check all the songs to see if they have been seen or not
	//add the unseen ones either to moved or deleted and mark everything back to zero
	for (hkey = 0; hkey < HASH_SIZE; hkey++)
	{
		if ((chain = hash_table[hkey]) != NULL)
		{
			int i;
			for (i = 0; i < CHAIN_LENGTH; i++)
			{
				if (chain[i].file == NULL)
					break;
				else
				{
					if (chain[i].flag == 0)//hasn't been seen, add it to old
					{
						int pos = check_against_added(chain[i].file);
						if (pos >= 0)//exactly one match means it's been moved and pos is the place in the array the old file is
						{
							remove_added(new_file[pos]);
							printf("MOVED %s\n", chain[i].file);
							send_update('M', chain[i].file);
							update_song_entry(chain[i].file, new_file[pos]);
							remove_from_hash(chain[i].file);
							change++;
						}
						//more than one match, probably something like Track_03.mp3, don't know if it's moved, delete it
						//no matches and it's gone, delete it
						else
						{
							printf("DELETED %s\n", chain[i].file);
							send_update('D', chain[i].file);
							delete_song_entry(chain[i].file);
							remove_from_hash(chain[i].file);
							change++;
						}
					}
					else //it's been counted, mark it done and move on
						chain[i].flag = 0;
				}
			}
		}
	}
	//now run through all of the new songs and put the remaining ones in the added list
	printf("check added\n");
	added_element *achain;
	int akey;
	for (akey = 0; akey < SMALL_HASH; akey++)
	{
		achain = added_table[akey];
		if (achain != NULL)
		{
			int i;
			for (i = 0; i < CHAIN_LENGTH; i++)
			{
				if (achain[i].file == NULL)
					break;
				else if (achain[i].path == NULL)
					printf("achain]i].path == NULL\n");
				else
				{
					printf("ADDED %s\n", achain[i].path);
					send_update('A', achain[i].path);
					add_new_file(achain[i].path);
					change++;
				}
			}
		}
	}
	clear_added();
	if (change == 0)
		send_update('N', NULL);
	printf("DONE\n");
	return 0;
}
int check_against_added(const char *file)
{
	int count = 0;
	int pos;
	int i;
	for (i = 0; i < added_n; i++)
	{
		if (strcmp(strrchr(new_file[i], '/'), strrchr(file, '/')) == 0)//just compare the filenames not the full paths
//		if (revcmp(new_file[i], file) == 0)
		{
			count++;
			pos = i;
		}
	}
	if (count == 1)
		return pos;
	else
		return -1;
}
hash_element* get_element(const char* file)
{
	if(file == NULL)
		return NULL;
	int key = hash_song(file);
	hash_element *chain;
	chain = hash_table[key];
	if (chain == NULL)
		return NULL;
	int i;
	for (i = 0; i < CHAIN_LENGTH; i++)
	{
		if (chain[i].file == NULL)
			return NULL;
		else if (strcmp(file, chain[i].file) == 0)
			return &chain[i];
	}
	return NULL;//fail
}
