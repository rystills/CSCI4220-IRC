#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>
#include <math.h>
#include <ctype.h>

#include "linkedList.h"
//#include "commands.h"
#include "client.h"

#define BUFFSIZE 2048

int numChannels = 0;
struct linkedList* clients;
struct linkedList* channels;

/**
display an error message and exit the application
@param msg: the error message to display
*/
void errorFailure(const char* msg)
{
	printf("Error: %s\n", msg);
	exit(EXIT_FAILURE);
}

/**
initialize the socket on which we listen for new client connections
@param servaddr: the socket address of the server used to establish the connection socket
@returns: our newly created connectionSocket which is actively listen for new connections
*/
int initializeListenerSocket(struct sockaddr_in* servaddr)
{
	int connectionSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (connectionSocket < 0)
		errorFailure("Socket creation failed");
	
	socklen_t s = sizeof(*servaddr);
	memset(servaddr, 0, s);
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = INADDR_ANY;
	servaddr->sin_port = 0;
	if (bind(connectionSocket, (const struct sockaddr *) servaddr, s) < 0)
		errorFailure("Bind failed");
	
	//check/get sin_port
	getsockname(connectionSocket, (struct sockaddr *) servaddr,&s);
	printf("port is: %d\n", ntohs(servaddr->sin_port));
	fflush(stdout);
	
	//begin listening
	listen(connectionSocket, 1);

	return connectionSocket;
}

/**
send a message to the specified client
@param cli: the client to send the message to
@param outMsg: the message to send
*/
void sendMessage(struct client* cli, char* outMsg) {
	send(cli->socket, outMsg, strlen(outMsg), 0);
	return;
}

/**
handle a message received on a client socket
@param senderNode: the node containing a reference to the client from whom we received a message
*/
void handleClientMessage(struct node* senderNode) {
	//dereference the client pointer from our clients node first thing
	struct client* sender = (struct client*)(senderNode->data);
	char buff[BUFFSIZE];
	//remove client if we get a read value of 0
	ssize_t amntRead = read(sender->socket,buff,BUFFSIZE-1);
	buff[amntRead] = '\0';
	if (amntRead == 0) {
		puts("sender disconnected");
		//printf("%s (socket %d) has disconnected\n", sender->name, sender->socket);
		return removeClient(senderNode);
	}
	//strip trailing newline when present
	if (buff[amntRead-1] == '\n') {
		buff[amntRead-1] = '\0';
		--amntRead;
	}
	//handle USER command
	if (amntRead >= 5 && strncmp(buff,"USER ",5) == 0) {
		//check username is a valid length
		if (amntRead > 25) {
			return sendMessage(sender,"Error: username too long. Max username length = 20 chars\n");
		}
		//check username isn't already set
		if (sender->nickname != NULL) {
			return sendMessage(sender,"Error: username has already been set for this user\n");
		}
		//check username starts with an alpha char
		if (!isalpha(buff[5])) {
			return sendMessage(sender,"Error: username must start with an alphabetic character\n");
		}
		//check username contains only alpha, num, and space
		for (int i = 6; i < amntRead; ++i) {
			if (!(isalnum(buff[i]) || buff[i] == ' ')) {
				return sendMessage(sender,"Error: username may only contain alphanumeric characters and spaces\n");
			}
		}
		//username is legal! set it
		sender->nickname = malloc(amntRead-5);
		strcpy(sender->nickname,buff+5);
		//let the user know they're all good
		snprintf(buff,BUFFSIZE-1,"Welcome, %s\n",sender->nickname);
		return sendMessage(sender, buff);	
	}

	//handle LIST command
	if (amntRead >= 5 && strncmp(buff,"LIST ",5) == 0) {
		return;
	}

	//handle JOIN command
	if (amntRead >= 5 && strncmp(buff,"JOIN ",5) == 0) {
		return;
	}

	//handle PART command
	if (amntRead >= 5 && strncmp(buff,"PART ",5) == 0) {
		return;
	}

	//handle OPERATOR command
	if (amntRead >= 9 && strncmp(buff,"OPERATOR ",9) == 0) {
		return;
	}

	//handle KICK command
	if (amntRead >= 5 && strncmp(buff,"KICK ",5) == 0) {
		return;
	}

	//handle PRIVMSG command
	if (amntRead >= 8 && strncmp(buff,"PRIVMSG ",8) == 0) {
		return;
	}

	//handle QUIT command
	if (amntRead >= 4 && strncmp(buff,"QUIT",4) == 0) {
		removeClient(senderNode);
		return;
	}

	//unrecognized command
	sendMessage(sender, "Error: unrecognized command\n");

}

int main(int argc, char** argv)
{
	struct sockaddr_in servaddr;
	int connectionSocket = initializeListenerSocket(&servaddr);

	clients = malloc(sizeof(struct linkedList));
	ll_init(clients);
	channels = malloc(sizeof(struct linkedList));
	ll_init(channels);

	while (true)
	{
		// Select on client ports and the listener port
		fd_set rfds;
		FD_ZERO(&rfds);
		int maxPort = connectionSocket;
		for (struct node* node = clients->head; node != NULL; node = node->next)
		{
			struct client* client = (struct client*)(node->data);
			FD_SET(client -> socket, &rfds);
			maxPort = fmax(client -> socket, maxPort);
		}
		FD_SET(connectionSocket, &rfds);
		select(maxPort+1, &rfds, NULL, NULL, NULL);

		//check if the incoming connection socket received anything
		if (FD_ISSET(connectionSocket, &rfds)) {
			acceptClient(&servaddr, connectionSocket);
		}
		//check if any client sockets received anything
		for (struct node* node = clients->head; node != NULL;) {
			struct client* client = (struct client*)(node->data);
			printf("%d\n",client->socket);
			//store client's next now, because we won't be able to access it if client gets freed
			struct node* oldNext = node->next;
			if (client->socket != -1 && FD_ISSET(client->socket, &rfds))
				handleClientMessage(node);
			//iterate to old next after potential free
			node = oldNext;
		}
	}
	free(clients);
	free(channels);

	//close(connectionSocket);
}