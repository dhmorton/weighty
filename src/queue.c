/*
 * queue.c
 *
 *  Created on: Jan 5, 2017
*  Copyright: 2017 David Morton
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

#include "queue.h"
#include "current_song.h"
#include "list_utils.h"
#include "net_utils.h"
#include "songhash.h"
#include "sqlite.h"

struct gen_node* queue_head = NULL;
struct gen_node* queue_removed = NULL;
static int qcount = 0;
static int qplayed = 0;

void queue_init()
{
	init_head(&queue_head);
	init_head(&queue_removed);
}
void enqueue(char *song)
{
	if(queue_head == NULL)
		init_head(&queue_head);
	int a[2];
	retrieve_by_song(song, a);
	make_add_node_to_end(&queue_head, song, a[0], a[1]);
	qcount++;
	printf("qcount = %d\n", qcount);
}
void get_queue()
{
	if(qcount == 0)
		return;
	song *data = malloc(qcount*sizeof(song));
	int i = 0;
	struct gen_node* cur = queue_head;
	for(; cur; cur = cur->next)
	{
		data[i].file = malloc(strlen(cur->name) + 1);
		memcpy(data[i].file, cur->name, strlen(cur->name) + 1);
		data[i].weight = cur->weight;
		data[i].sticky = cur->sticky;
		i++;
	}
	if(i != qcount)
		printf("QUEUE bad count\ti = %d\tqcount = %d\n", i, qcount);
	send_song_data(data, qcount, 'P');
	free(data);
}
void clear_queue()
{
	clear_list(&queue_head);
	qcount = 0;
	qplayed = 0;
}
void clear_queue_by_index(int index)
{
	if(index < qcount)
	{
		printf("QUEUE ERROR qcount > index : %d > %d\n", qcount, index);
		return;
	}
	rm_node_by_index(&queue_head, index);
	qcount--;
	get_queue();
}
void set_cursong_from_qpointer()
{
	set_cursong(queue_head->name);
	move_node(&queue_head, &queue_removed, queue_head);
	qcount--;
	qplayed++;
}
void highlight_playlist()
{
	if(qcount <= 0)
		return;
	char i[6];
	memset(i, 0, 6);
	snprintf(i, 6, "%d", qcount);
	char com[10];
	com[0] = 'L';//highlight song in playlist
	char *pcom = &com[1];
	memcpy(pcom, i, strlen(i) + 1);
	send_command(com, strlen(i) + 2);
}
int get_qcount()
{
	return qcount;
}
