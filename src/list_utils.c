/*
 * list_utils.c
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


#include "list_utils.h"

void init_head(struct gen_node** head)
{
	*head = malloc(sizeof(struct gen_node));
	(*head)->name = NULL;
	(*head)->next = NULL;
}
void clear_list(struct gen_node** head) {
	while(*head != NULL)
	{
		if((*head)->name != NULL)
			free((*head)->name);
		struct gen_node* temp = *head;
		*head = (*head)->next;
		free(temp);
	}
}
void make_push_node(struct gen_node** head, char* data, int weight, int sticky)
{
	if((*head)->name == NULL)
	{
		(*head)->name = malloc(strlen(data) + 1);
		memcpy((*head)->name, data, strlen(data) + 1);
		(*head)->weight = weight;
		(*head)->sticky = sticky;
		(*head)->next = NULL;
	}
	else
	{
		struct gen_node* node = malloc(sizeof(struct gen_node));
		node->name = malloc(strlen(data) + 1);
		memcpy(node->name, data, strlen(data) + 1);
		node->weight = weight;
		node->sticky = sticky;
		node->next = *head;
		*head = node;
	}
}
void make_add_node_to_end(struct gen_node** head, char* data, int weight, int sticky)
{
	if((*head)->name == NULL)
	{
		(*head)->name = malloc(strlen(data) + 1);
		memcpy((*head)->name, data, strlen(data) + 1);
		(*head)->weight = weight;
		(*head)->sticky = sticky;
		(*head)->next = NULL;
	}
	else
	{
		struct gen_node* cur = *head;
		while(cur->next != NULL)
			cur = cur->next;
		struct gen_node* node = malloc(sizeof(struct gen_node));
		node->name = malloc(strlen(data) + 1);
		memcpy(node->name, data, strlen(data) + 1);
		node->weight = weight;
		node->sticky = sticky;
		node->next = NULL;
		cur->next = node;
	}
}
void push_node(struct gen_node** head, struct gen_node* node)
{
	if((*head)->name != NULL)
		node->next = *head;
	else
		node->next = NULL;
	*head = node;
}
void pop_head(struct gen_node** head)
{
	struct gen_node* temp = *head;
	*head = (*head)->next;
	if(temp->name != NULL)
	{
		free(temp->name);
		free(temp);
	}
}
void move_node(struct gen_node** from_head, struct gen_node** to_head, struct gen_node* rem_node)
{
	if(*from_head == NULL)
	{
		return;
	}
	if(*from_head == rem_node)//removing head
	{
		struct gen_node* temp = (*from_head)->next;
		push_node(to_head, *from_head);
		*from_head = temp;
		return;
	}
	struct gen_node* cur = *from_head;
	for(; cur != NULL; cur = cur->next)
	{
		if(cur->next == rem_node)
			break;
	}
	if(cur == NULL)//not found
	{
		printf("NODE NOT FOUND\n");
	}
	else
	{
		struct gen_node* temp = cur->next;
		cur->next = cur->next->next;
		push_node(to_head, temp);
	}
}
void free_node(struct gen_node** head, struct gen_node* node)
{
	if(*head == node)
	{
		struct gen_node* temp = *head;
		*head = node->next;
		if(temp->name != NULL)
			free(temp->name);
		free(temp);
	}
	else
	{
		struct gen_node* cur = *head;
		for(; cur; cur = cur->next)
		{
			if(cur->next == node)
				break;
		}
		if(cur == NULL)
			printf("Can't free node, not found\n");
		else
		{
			struct gen_node* temp = cur->next;
			cur->next = cur->next->next;
			if(temp->name != NULL)
				free(temp->name);
			free(temp);
		}
	}
}
void rm_node_by_index(struct gen_node** head, int n)
{
	int i = 0;
	struct gen_node* cur = *head;
	while(i++ < (n - 1))//want to stop at the node *before* the node to remove
		cur = cur->next;
	struct gen_node* temp = cur->next;
	cur = cur->next;
	if(temp->name != NULL)
		free(temp->name);
	free(temp);
}
//node will be the head node of the to_head list
void get_random_node(struct gen_node** head, struct gen_node** to_head, int len)
{
	int n = 0;
	if(len > 0)
		n = random() % len;
	int i = 0;
	struct gen_node* cur = *head;
	while(i++ < n && cur->next != NULL)
	{
		cur = cur->next;
	}
	move_node(head, to_head, cur);
}
void print_list_from_head(struct gen_node* head)
{
	for(; head; head = head->next)
	{
		if(head->name != NULL)
			printf("%s\n", head->name);
		else
			printf("NULL\n");
	}

}
