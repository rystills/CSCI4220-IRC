#include <stdlib.h>

struct client
{
	char* nickname;
	int port;
};

struct clientList
{
	struct client* head;
};

void addClient(struct client* clientList, struct client* client)

void removeClient(struct client* clientList, struct client* client)