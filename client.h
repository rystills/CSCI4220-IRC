#include <stdlib.h>

struct client
{
	char* nickname;
	int socket;
};

struct clientList
{
	struct client* head;
};

void acceptClient(struct sockaddr_in* servaddr, struct client* clients, int connection_socket) {};

void removeClient(struct client* clientList, struct client* client) {};