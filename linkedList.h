#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdarg.h>

#ifndef _LINKEDLIST_GUARD
#define _LINKEDLIST_GUARD

struct linkedList
{
	struct node* head;
	struct node* tail;
	int numElements;
};

struct node
{
	struct node* next;
	struct node* prev;
	void* data;
};

void ll_init(struct linkedList* list) {
	list->numElements = 0;
	list->head = NULL;
	list->tail = NULL;
}

struct node* ll_add(struct linkedList* list, void* data) {
	struct node* node = malloc(sizeof(struct node));
	node->next = NULL;
	node->prev = NULL;
	(node->data) = data;
	if (list->numElements == 0) {
		list->head = node;
		list->tail = node;
	}
	else {
		list->tail->next = node;
		node->prev = list->tail;
		list->tail = node;
	}
	list->numElements++;
	return node;
}

void ll_remove(struct linkedList* list, struct node* node) {
	//standard case: node is somewhere in the middle, so update prev->next and next->prev and move on
	if (node != list->tail && node != list->head) {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	//special case: node is the only one, so head and tail should both be set to NULL
	else if (node == list->head && node == list->tail) {
		list->head = list->tail = NULL;
	}
	//special case: node is the head, buth others exist. update next->prev
	else if (node == list->head) {
		node->next->prev = NULL;
		list->head = node->next;
	}

	//special case: node is the tail, but others exist. update prev->next
	else if (node == list->tail) {
		node->prev->next = NULL;
		list->tail = node->prev;
	}
	printf("%p,%p\n",list->head,list->tail);
	free(node);
}

#endif