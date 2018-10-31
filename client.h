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

#ifndef _CLIENT_GUARD
#define _CLIENT_GUARD

extern struct linkedList* clients;

struct client
{
	char* nickname;
	int socket;
};

/**
check whether or not the specified client has set his nickname
@param sender: the client whose nickname we wish to check
@returns: whether sender set his nickname (true) or not (false)
*/
bool checkNameSet(struct client* sender) {
	char* outMsg = "Invalid command, please identify yourself with USER.\n";
	if (sender->nickname == NULL) {
		send(sender->socket, outMsg, strlen(outMsg), 0);
		return false;
	}
	return true;
}

/**
finds the client with the specified username
@param name: the name of the client we wish to find
@returns: the client with the specified name if located, else NULL
*/ 
struct client* findClientWithName(char* name) {
	for (struct node* node = clients->head; node != NULL; node = node->next) {
		struct client* client = node->data;
		if (client->nickname != NULL && strcmp(client->nickname,name) == 0) {
			return client;
		}
	}
	return NULL;
}

/**
accept an incoming connection request from a client
@param servaddr: the server address socket
@param clients: the array of client structs
@param connection_socket: the socket on which we are ready to receive the incoming client connection
@returns: a pointer to the node in clients which contains the new client data 
*/
struct node* acceptClient(struct sockaddr_in* servaddr, int connection_socket) {
	unsigned int len = sizeof(struct sockaddr_in);
	struct client *newCli = NULL;
	newCli = malloc(sizeof(struct client));
	newCli->nickname = NULL;
	newCli->socket = accept(connection_socket, (struct sockaddr *) servaddr, &len);
	if (newCli->socket < 0) {
		printf("Error: accept() failed");
		exit(EXIT_FAILURE);
	}
	puts("sender connected");
	return ll_add(clients,newCli);
}

/**
remove a client, updating the linked list and closing his socket
@param cliNode: the node pointer containing the client to remove
*/
void removeClient(struct node* cliNode) {
	struct client* client = cliNode->data;
	close(client->socket);
	client->socket = -1;
	ll_remove(clients,cliNode);
	//free the client's nickname if it has been set
	if (client->nickname != NULL) {
		free(client->nickname);
	}
	free(client);
	puts("sender disconnected");
}
#endif