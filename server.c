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
#include "channel.h"

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
check whether or not the specified string matches our requriements (20 characters, regexp [a-zA-Z][_0-9a-zA-Z]*)
@param sLoc: the starting location of the string in the specified buffer
@param buff: the buffer containing the string to check
@param amntRead: the total length of meaningful data in the buffer
@param sender: the client who messaged us this string
@returns: whether the string is valid (true) or not (false)
*/
bool checkValidString(int sLoc, char* buff, int amntRead, struct client* sender) {
	//check string is a valid length
	if (amntRead > 20+sLoc) {
		sendMessage(sender,"Error: provided string too long. Max length = 20 chars\n");
		return false;
	}
	//check string starts with an alpha char
	if (!isalpha(buff[sLoc])) {
		sendMessage(sender,"Error: provided string must start with an alphabetic character\n");
		return false;
	}
	//check string contains only alpha, num, and space
	for (int i = sLoc+1; i < amntRead; ++i) {
		if (!(isalnum(buff[i]) || buff[i] == ' ')) {
			sendMessage(sender,"Error: provided string must only contain alphanumeric characters and spaces\n");
			return false;
		}
	}
	return true;
}

/**
check whether or not the specified client has set his nickname
@param sender: the client whose nickname we wish to check
@returns: whether sender set his nickname (true) or not (false)
*/
bool checkNameSet(struct client* sender) {
	if (sender->nickname == NULL) {
		sendMessage(sender,"Invalid command, please identify yourself with USER.\n");
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
handle a message received on a client socket
@param senderNode: the node containing a reference to the client from whom we received a message
*/
void handleClientMessage(struct node* senderNode) {
	//dereference the client pointer from our clients node first thing
	struct client* sender = senderNode->data;
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
		//check username isn't already set
		if (sender->nickname != NULL) {
			return sendMessage(sender,"Error: username has already been set for this user\n");
		}
		//check that the username is a valid string
		if (!checkValidString(5,buff,amntRead,sender)) return;
		//check that username isn't in use by someone else
		if (findClientWithName(buff+5) != NULL) {
			return sendMessage(sender,"Error: username is already taken by someone else\n");
		}
		sender->nickname = malloc(amntRead-5);
		strcpy(sender->nickname,buff+5);
		//let the user know they're all good
		snprintf(buff,BUFFSIZE-1,"Welcome, %s\n",sender->nickname);
		return sendMessage(sender, buff);	
	}

	//make sure the username is set before continuing on
	if (!checkNameSet(sender)) return;

	//handle LIST command
	if (amntRead >= 4 && strncmp(buff,"LIST",4) == 0) {
		return;
	}

	//handle JOIN command
	if (amntRead >= 5 && strncmp(buff,"JOIN ",5) == 0) {
		//check that the channel name is a valid string starting with #
		if (buff[5] != '#') {
			return sendMessage(sender,"Error: channel name must begin with '#'\n");
		}
		if (!checkValidString(6,buff,amntRead,sender)) return;
		struct node* channelNode = joinChannel(sender,buff+6);
		//if we got a return value of NULL, that means we're already present in the channel
		if (channelNode == NULL) {
			return sendMessage(sender, "Error: user already present in specified channel\n");
		}
		//TODO: say hello to everyone in the channel (excluding ourself)
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
		//TODO: remove the user from all channels
		removeClient(senderNode);
		return;
	}

	//unrecognized command
	sendMessage(sender, "Invalid command.\n");

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
			struct client* client = node->data;
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
			struct client* client = node->data;
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