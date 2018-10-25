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
	struct client newCli;
	newCli.socket = accept(connection_socket, (struct sockaddr *) servaddr, &len);
	if (newCli.socket < 0) {
		printf("Error: accept() failed");
		exit(EXIT_FAILURE);
	}
	if (numClients == 0) {
		cliHead = &newCli;
		cliTail = &newCli;
	}
	else {
		cliTail->next = &newCli;
		newCli.prev = cliTail;
		cliTail = &newCli;
	}
	++numClients;
};

void removeClient(struct client* client) {
	close(client->socket);
	client->socket = -1;
	--numClients;
	if (client != cliTail && client != cliHead) {
		client->prev->next = client->next;
		client->next->prev = client->prev;
		return;
	}
	if (client == cliHead && client == cliTail) {
		cliHead = NULL;
		cliTail = NULL;
		return;
	}
	if (client == cliHead) {
		client->next->prev = NULL;
		cliHead = client->next;
	}
	if (client == cliTail) {
		client->prev->next = NULL;
		cliTail = client->prev;
	}

};