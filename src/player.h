/*
 * player.h
 *
 *  Created on: Aug 12, 2008
 *      Author: bob
 */

#ifndef SRC_PLAYER_H_
#define SRC_PLAYER_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stddef.h>

int initialize(void);

/*
 * Updates the database to find new, deleted or moved songs.
 * The method is to read the directory recursively checking every song against the current hash
 * If the song is there mark the flag as unchanged
 * If the song isn't there then add it and flag it as changed, also record the path and the song name in arrays
 * Read through the entire hash table
 * If a song is not marked as seen or new then check to see if it is in the list of new songs
 * If it matches songs in the list exactly once mark it as moved
 * If it matches songs in the list more than once mark it as new (can't verify which track it was i.e. Track_12.mp3)
 * If it doesn't match a song in the new list then it's gone, delete it from the hash and database.
 */
void update_songs(void);
void generate_playlist(void);

#endif /* SRC_PLAYER_H_ */
