#include <stdlib.h>

extern int numChannels;
extern int numClients;
extern struct client* cliHead;
extern struct client* cliTail;

struct client
{
	char* nickname;
	int socket;
	struct client* next;
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
		cliTail = &newCli;
	}
	++numClients;
};

void removeClient(struct client* clientList, struct client* client) {

};