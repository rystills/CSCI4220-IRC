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
@returns: the client that is found in inChannel, or NULL if the client is not found
*/ 
struct node* clientInChannel(struct channel* channel, struct client* inClient) {
	struct linkedList* clients = channel->clients;
	for (struct node* node = clients->head; node != NULL; node = node->next) {
		struct client* client = node->data;
		if (client == inClient) {
			return node;
		}
	}
	return NULL;
}

/**
check if the client with the specified name is in the specified channel
@param channel: the channel to check in
@param inClient: the client to check for presence in the channel
@returns: the client that is found in inChannel, or NULL if the client is not found
*/ 
struct node* clientNameInChannel(struct channel* channel, char* clientName) {
	struct linkedList* clients = channel->clients;
	for (struct node* node = clients->head; node != NULL; node = node->next) {
		struct client* client = node->data;
		if (strcmp(client->nickname,clientName) == 0) {
			return node;
		}
	}
	return NULL;
}

/**
find the channel with the specified name, if it exists
@param name: the name of the channel we wish to find
@returns: the channel with the specified name, or NULL if no such channel is found
*/
struct channel* findChannel(char* name) {
	for (struct node* node = channels->head; node != NULL; node = node->next) {
		struct channel* channel = node->data;
		if (strcmp(channel->name,name) == 0) {
			return channel;
		}
	}
	return NULL;
}

/**
send a message to all members of a channel, optionally ignoring one member
@param msg: message to send
@param channel: channel on which to send the message
@param ignoreMe: client who should not receive the message (typically the sender)
*/
void sendToChannelMembers(char* msg, struct channel* channel, struct client* ignoreMe) {
	for (struct node* node = channel->clients->head; node != NULL; node = node->next) {
		struct client* client = node->data;
		if (client == ignoreMe) continue;
		send(client->socket, msg, strlen(msg), 0);
	}
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
			if (clientInChannel(channel,client) != NULL) {
				return NULL;
			}
			ll_add(channel->clients,client);
			return node;
		}
	}
	//puts("channel not found; creating new channel");
	struct channel* newChannel = malloc(sizeof(struct channel));
	newChannel->name = malloc(strlen(name)+1);
	newChannel->clients = malloc(sizeof(struct linkedList));
	ll_init(newChannel->clients);
	strcpy(newChannel->name,name);
	ll_add(newChannel->clients,client);
	return ll_add(channels,newChannel);
}
#endif