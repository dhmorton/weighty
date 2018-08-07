/*
 * firsttime.h
 *
 *  Created on: Jul 4, 2011
 *      Author: bob
 */

#ifndef SRC_FIRSTTIME_H_
#define SRC_FIRSTTIME_H_

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/*
 * Opens a gtk dialogue window to get the root directory where music is stored.
 */
void get_musicdir(void);

#endif /* SRC_FIRSTTIME_H_ */
