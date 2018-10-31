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
	struct linkedList* clients;
};

/**
check if the specified client is in the specified channel
@param channel: the channel to check in
@param inClient: the client to check for presence in the channel
@returns: whether client is found in inChannel
*/ 
bool clientInChannel(struct channel* channel, struct client* inClient) {
	struct linkedList* clients = channel->clients;
	for (struct node* node = clients->head; node != NULL; node = node->next) {
		struct client* client = node->data;
		if (client->nickname == inClient->nickname) {
			return true;
		}
	}
	return false;
}

/**
add the specified client node to the channel with the specific name
@param client: the client we wish to add to the channel
@param name: the name of the channel to which we wish to add the client
@returns: a node* containing the channel to which we added the client, or NULL if the client already existed in the channel
*/
struct node* joinChannel(struct client* client, char* name) {
	for (struct node* node = channels->head; node != NULL; node = node->next) {
		struct channel* channel = node->data;
		if (strcmp(channel->name,name) == 0) {
			//check if client is already in channel
			if (clientInChannel(channel,client)) {
				return NULL;
			}
			ll_add(channel->clients,client);
			return node;
		}
	}
	puts("channel not found; creating new channel");
	struct channel* newChannel = malloc(sizeof(struct channel));
	newChannel->name = malloc(strlen(name)+1);
	newChannel->clients = malloc(sizeof(struct linkedList));
	ll_init(newChannel->clients);
	strcpy(newChannel->name,name);
	ll_add(newChannel->clients,client);
	return ll_add(channels,newChannel);
}

#endif