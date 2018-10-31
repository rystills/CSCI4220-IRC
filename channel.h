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
#include "linkedList.h"

#ifndef _CHANNEL_GUARD
#define _CHANNEL_GUARD

extern struct linkedList* channels;

struct channel
{
	char* name;
};

/**
add the specified client node to the channel with the specific name
@param client: the client we wish to add to the channel
@param name: the name of the channel to which we wish to add the client
@returns: a node* containing the channel to which we added the client
*/
struct node* joinChannel(struct client* client, char* name) {
	for (struct node* node = channels->head; node != NULL; node = node->next) {
		struct channel* channel = node->data;
		if (strcmp(channel->name,name) == 0) {
			//TODO: add self to a new linkedlist of users in channel
			return node;
		}
	}
	puts("channel not found; creating channel");
	struct channel* newChannel = malloc(sizeof(struct channel));
	newChannel->name = malloc(strlen(name)+1);
	strcpy(newChannel->name,name);
	//TODO: add self to a new linkedlist of users in channel
	return ll_add(channels,newChannel);
}

#endif