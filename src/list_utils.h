/*
 * list_utils.h
 *
 *  Created on: Dec 26, 2016
 *      Author: bob
 */

#ifndef SRC_LIST_UTILS_H_
#define SRC_LIST_UTILS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct gen_node {
	char* name;
	int weight;
	int sticky;
	struct gen_node* next;
} agnode;

void init_head(struct gen_node**);
void clear_list(struct gen_node**);
void make_push_node(struct gen_node**, char*, int, int);
void make_add_node_to_end(struct gen_node**, char*, int, int);
void push_node(struct gen_node**, struct gen_node*);
void pop_head(struct gen_node**);
void move_node(struct gen_node**, struct gen_node**, struct gen_node*);
void free_node(struct gen_node**, struct gen_node*);
void rm_node_by_index(struct gen_node**, int);
void get_random_node(struct gen_node**, struct gen_node**, int);
void print_list_from_head(struct gen_node*);

#endif /* SRC_LIST_UTILS_H_ */
