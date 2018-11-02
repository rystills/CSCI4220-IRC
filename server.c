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
#include "commands.h"
#include "client.h"
#include "channel.h"

#define BUFFSIZE 2048

struct linkedList* clients;
struct linkedList* channels;
char* password;

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
@param shouldNotifySender: whether to alert the sender if an invalid string was provided (true) or not (false)
@param stopAtSpace: wether we should stop upon reaching a space (true) or keep reading (false)
@returns: whether the string is valid (true) or not (false)
*/
bool checkValidString(int sLoc, char* buff, int amntRead, struct client* sender, bool shouldNotifySender, bool stopAtSpace) {
	//check string starts with an alpha char
	if (!isalpha(buff[sLoc])) {
		if (shouldNotifySender)
			sendMessage(sender,"Error: provided string must start with an alphabetic character\n");
		return false;
	}
	//check string contains only alpha, num, and space
	for (int i = sLoc+1; i < amntRead; ++i) {
		if (!isalnum(buff[i]) && buff[i] != '_') {
			if (stopAtSpace && (buff[i] == ' '))
				return true;
			if (shouldNotifySender)
				sendMessage(sender,"Error: provided string must only contain alphanumeric characters and underscores\n");
			return false;
		}
		//check string is a valid length
		if (i == sLoc+20) {
			if (shouldNotifySender)
				sendMessage(sender,"Error: provided string too long. Max length = 20 chars\n");
			return false;
		}
	}
	return true;
}

bool leaveChannel(struct channel* channel, struct client* sender, bool shouldPrintError)
{
	struct node* channelCli = clientInChannel(channel,sender);
	if (channelCli == NULL)
	{
		if (shouldPrintError)
			sendMessage(sender,"Error: invalid channel specified.\n");
		return false;
	}
	
	char outBuff[BUFFSIZE];
	sprintf(outBuff,"#%s> %s has left the channel.\n",channel->name,sender->nickname);
	sendToChannelMembers(outBuff,channel,NULL);
	ll_remove(channel->clients,channelCli);
	return true;
}

bool handleJOIN(char* buff, int amntRead, struct client* sender)
{
	//check that the channel name is a valid string starting with #
	if (buff[5] != '#') {
		sendMessage(sender,"Error: channel name must begin with '#'.\n");
		return false;
	}
	if (!checkValidString(6,buff,amntRead,sender,true,false))
		return false;
	struct node* channelNode = joinChannel(sender,buff+6);
	//if we got a return value of NULL, that means we're already present in the channel
	if (channelNode == NULL) {
		sendMessage(sender, "Error: user already present in specified channel.\n");
		return false;
	}

	struct channel* channel = channelNode -> data;
	//say hello to everyone in the channel
	char outBuff[BUFFSIZE];
	sprintf(outBuff,"#%s> %s joined the channel.\n",channel->name,sender->nickname);
	sendToChannelMembers(outBuff,channel,sender);
	//let the connected client know that he joined successfully
	sprintf(outBuff,"Joined channel #%s\n",channel->name);
	sendMessage(sender,outBuff);
	return true;
}

bool handlePART(char* buff, int amntRead, struct client* sender)
{
	if (amntRead > 4 && buff[4] != ' ') {
		sendMessage(sender,"Error: command not recognized. Did you mean PART?\n");
		return false;
	}
	char outBuff[BUFFSIZE];
	//check if a valid channel was specified
	if (amntRead >= 6 && buff[5] == '#' && checkValidString(6,buff,amntRead,sender,false,false)) {
		//valid channel name was specified; check if the channel with that name exists
		struct channel* channel = findChannel(buff+6);
		if (channel == NULL) {
			sprintf(outBuff,"Channel #%s does not exist.\n",buff+6);
			sendMessage(sender,outBuff);
			return false;
		}
		return leaveChannel(channel, sender, true);
	}

	//no argument was specified, so remove user from all channels
	for (struct node* node = channels->head; node != NULL; node = node->next)
		leaveChannel(node -> data, sender, false);
	return true;
}

bool handleOPERATOR(char* buff, int amntRead, struct client* sender)
{
	//check that the password is a valid string
	if (!checkValidString(9,buff,amntRead,sender,true,false))
		return false;
	//check if there is a password and the password is correct
	if (password[0] != '\0' && strcmp(buff+9,password) == 0) {
		sender->isOperator = true;
		sendMessage(sender,"OPERATOR status bestowed.\n");
		return true;
	}
	sendMessage(sender,"Invalid OPERATOR command.\n");
	return false;
}

bool handleKICK(char* buff, int amntRead, struct client* sender)
{
	//must be an operator to kick
	if (!sender->isOperator) {
		sendMessage(sender,"Error: cannot kick if not OPERATOR\n");
		return false;
	}
	//verify channel name
	if (buff[5] != '#') {
		sendMessage(sender,"Error: channel name must start with a '#'\n");
		return false;
	}
	if (!checkValidString(6,buff,amntRead,sender,true,true))
		return false;
	char channelName[21];
	//set channel name
	int spaceInd = 5;
	while (buff[spaceInd] != ' ' && spaceInd < amntRead) {
		++spaceInd;
	}
	if (spaceInd == amntRead) {
		sendMessage(sender,"Error: no user to kick specified\n");
		return false;
	}
	strncpy(channelName,buff+6,spaceInd-6);
	channelName[spaceInd-6] = '\0';
	
	//verify user name
	if (!checkValidString(spaceInd+1,buff,amntRead,sender,true,false))
		return false;
	//set user name
	char userName[21];
	memset(userName,'\0',sizeof(userName));
	strcpy(userName,buff+spaceInd+1);
	
	//check if channel exists
	struct channel* channel = findChannel(channelName);
	if (channel == NULL) {
		sendMessage(sender,"Error: channel not found\n");
		return false;
	}
	//channel matching specified name detected; check if kick user is present in it
	struct node* cliNode = clientNameInChannel(channel,userName);
	if (cliNode == NULL) {
		sendMessage(sender,"Error: client not found in channel\n");
		return false;
	}
	//client found in channel; kick him
	char outBuff[BUFFSIZE];
	sprintf(outBuff,"#%s> %s has been kicked from the channel.\n",channel->name,userName);
	sendToChannelMembers(outBuff,channel,NULL);
	ll_remove(channel->clients,cliNode);
	return true;
}

bool handlePRVMSG(char* buff, ssize_t amntRead, struct client* sender)
{
	//verify destination name
	int isChannel = buff[8] == '#' ? 1 : 0;
	if (!checkValidString(8+isChannel,buff,amntRead,sender,true,true))
		return false;
	char destName[21];
	//set destination name
	int spaceInd = 8;
	while (buff[spaceInd] != ' ' && spaceInd < amntRead) {
		++spaceInd;
	}
	if (spaceInd == amntRead) {
		sendMessage(sender,"Error: no channel to message specified\n");
		return false;
	}
	strncpy(destName,buff+8+isChannel,spaceInd-(8+isChannel));
	destName[spaceInd-(8+isChannel)] = '\0';
	
	//verify message length
	if (amntRead - spaceInd - 1 > 512) {
		sendMessage(sender,"Error: message length may not exceed 512 chars\n");
		return false;
	}
	if (isChannel) {
		//check if channel exists
		struct channel* channel = findChannel(destName);
		if (channel == NULL) {
			sendMessage(sender,"Error: channel not found\n");
			return false;
		}

		//channel matching specified name detected; send message to all channel members
		char outBuff[BUFFSIZE];
		sprintf(outBuff,"#%s> %s: %s\n",channel->name,sender->nickname,buff+spaceInd+1);
		sendToChannelMembers(outBuff,channel,NULL);
	}
	else {
		//check if user exists
		struct client* user = findClientWithName(destName);
		if (user == NULL) {
			sendMessage(sender,"Error: user not found\n");
			return false;
		}
		//user matching specified name detected; send message to him
		char outBuff[BUFFSIZE];
		//TODO: should there be a newline after PRIVMSG to user? hw pdf seems unclear
		sprintf(outBuff,"<<%s %s\n",sender->nickname,buff+spaceInd+1);
		sendMessage(user,outBuff);
		sprintf(outBuff,"%s>> %s\n",user->nickname,buff+spaceInd+1);
		sendMessage(sender,outBuff);
	}
	return true;
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
	memset(buff,'\0',sizeof(buff));
	ssize_t amntRead = read(sender->socket,buff,BUFFSIZE-1);
	buff[amntRead] = '\0';
	if (amntRead == 0) {
		amntRead = 4;
		sprintf(buff,"QUIT");
	}
	//strip trailing newline when present
	if (buff[amntRead-1] == '\n') {
		buff[amntRead-1] = '\0';
		--amntRead;
	}

	//handle QUIT command
	if (amntRead == 4 && strncmp(buff,"QUIT",4) == 0) {
		//remove user from all channels
		for (struct node* node = channels->head; node != NULL; node = node->next) {
			struct channel* channel = node->data;
			struct node* channelCli = clientInChannel(channel,sender);
			if (channelCli != NULL) {
				ll_remove(channel->clients,channelCli);
				//inform all channel members that we've left
				//TODO: the hw pdf doesn't say we should inform everyone that we left the channel if it happened via QUIT, but this makes the most sense
				char outBuff[BUFFSIZE];
				sprintf(outBuff,"#%s> %s has left the channel.\n",channel->name,sender->nickname);
				sendToChannelMembers(outBuff,channel,sender);
			}
		}
		
		removeClient(senderNode);
		return;
	}

	//handle USER command
	if (amntRead >= 5 && strncmp(buff,"USER ",5) == 0) {
		//check username isn't already set
		if (sender->nickname != NULL) {
			sendMessage(sender,"Error: username has already been set for this user\n");
			return;
		}
		//check that the username is a valid string
		if (!checkValidString(5,buff,amntRead,sender,true,false))
			return;
		//check that username isn't in use by someone else
		if (findClientWithName(buff+5) != NULL) {
			sendMessage(sender,"Error: username is already taken by someone else\n");
			return;
		}
		sender->nickname = malloc(amntRead-5);
		strcpy(sender->nickname,buff+5);
		//let the user know they're all good
		snprintf(buff,BUFFSIZE-1,"Welcome, %s\n",sender->nickname);
		sendMessage(sender, buff);
		return;
	}

	//make sure the username is set before continuing on
	if (!checkNameSet(sender)) return;

	//handle LIST command
	if (amntRead >= 4 && strncmp(buff,"LIST",4) == 0) {
		if (amntRead > 4 && buff[4] != ' ') {
			return sendMessage(sender,"Error: command not recognized. Did you mean LIST?\n");
		}
		char outBuff[BUFFSIZE];
		//check if a valid channel was specified
		if (amntRead >= 6 && buff[5] == '#' && checkValidString(6,buff,amntRead,sender,false,false)) {
			//valid channel name was specified; check if the channel with that name exists
			struct channel* foundChannel = findChannel(buff+6);
			if (foundChannel != NULL) {
				//channel matching specified name detected; list contents of that channel instead of channel list
				sprintf(outBuff,"There are currently %d members.\n",foundChannel->clients->numElements);
				for (struct node* node = foundChannel->clients->head; node != NULL; node = node->next) {
					struct client* client = node->data;
					sprintf(outBuff+strlen(outBuff),"* %s\n",client->nickname);
				}
				sendMessage(sender,outBuff);
				return;
			}
		}
		//no valid channel was specified, so list channels
		sprintf(outBuff,"There are currently %d channels.\n",channels->numElements);
		for (struct node* node = channels->head; node != NULL; node = node->next) {
			struct channel* channel = node->data;
			sprintf(outBuff+strlen(outBuff),"* %s\n",channel->name);
		}
		sendMessage(sender,outBuff);
		return;
	}

	//handle JOIN command
	if (amntRead >= 5 && strncmp(buff,"JOIN ",5) == 0) {
		handleJOIN(buff, amntRead, sender);
	}

	//handle PART command
	if (amntRead >= 4 && strncmp(buff,"PART",4) == 0) {
		handlePART(buff, amntRead, sender);
	}

	//handle OPERATOR command
	if (amntRead >= 9 && strncmp(buff,"OPERATOR ",5) == 0) {
		handleOPERATOR(buff, amntRead, sender);
	}

	//handle KICK command
	if (amntRead >= 5 && strncmp(buff,"KICK ",5) == 0) {
		handleKICK(buff, amntRead, sender);
	}

	//handle PRIVMSG command
	if (amntRead >= 8 && strncmp(buff,"PRIVMSG ",8) == 0) {
		handlePRVMSG(buff, amntRead, sender);
	}

	//unrecognized command
	sendMessage(sender, "Invalid command.\n");
}

int main(int argc, char** argv)
{
	//set password to argv[1] if valid expression
	if (argc > 1) {
		int argLen = strlen(argv[1]);
		if (argLen > 11 && strncmp(argv[1],"--opt-pass=",11) == 0 && checkValidString(11,argv[1],argLen,NULL,false,false)) {
			password = argv[1]+11;
		}
	}
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