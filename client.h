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

extern int numChannels;
extern int numClients;
extern struct client* cliHead;
extern struct client* cliTail;

struct client
{
	char* nickname;
	int socket;
	struct client* next;
	struct client* prev;
};

/**
accept an incoming connection request from a client
@param servaddr: the server address socket
@param clients: the array of client structs
@param connection_socket: the socket on which we are ready to receive the incoming client connection
*/
void acceptClient(struct sockaddr_in* servaddr, int connection_socket) {
	unsigned int len = sizeof(struct sockaddr_in);
	struct client *newCli = NULL;
	newCli = malloc(sizeof(struct client));

	newCli->socket = accept(connection_socket, (struct sockaddr *) servaddr, &len);
	if (newCli->socket < 0) {
		printf("Error: accept() failed");
		exit(EXIT_FAILURE);
	}
	puts("sender connected");
	if (numClients == 0) {
		cliHead = newCli;
		cliTail = newCli;
	}
	else {
		cliTail->next = newCli;
		newCli->prev = cliTail;
		cliTail = newCli;
	}
	++numClients;
};

/**
remove a client, updating the linked list and closing his socket
@param client: the client to remove
*/
void removeClient(struct client* client) {
	close(client->socket);
	client->socket = -1;
	--numClients;
	//standard case: client is somewhere in the middle, so update prev->next and next->prev and move on
	if (client != cliTail && client != cliHead) {
		client->prev->next = client->next;
		client->next->prev = client->prev;
		return;
	}
	//special case: client is the only one, so head and tail should both be set to NULL
	if (client == cliHead && client == cliTail) {
		cliHead = cliTail = NULL;
		return;
	}
	//special case: client is the head, buth others exist. update next->prev
	if (client == cliHead) {
		client->next->prev = NULL;
		cliHead = client->next;
	}

	//special case: client is the tail, but others exist. update prev->next
	if (client == cliTail) {
		client->prev->next = NULL;
		cliTail = client->prev;
	}

};