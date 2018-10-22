#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>

#include "channel.h"
#include "commands.h"

int numChannels = 0;
struct channel* channels = NULL;

void list(const char* channelName)
{
	for (struct channel* channel=channels; channel < channels+numChannels; ++channel)
		if (strcmp(channel -> name, channelName) == 0)
		{
			printf("There are currently %d members.\n", channel -> numClients);
			return;
		}
	
	printf("There are currently %d channels.\n", numChannels);
}

void exitError(char* str)
{
	perror(str); 
	exit(EXIT_FAILURE); 
}

int main(int argc, char** argv)
{
	int connectionSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (connectionSocket < 0)
		exitError("unable to create socket"); 
	
	//init server socket
	struct sockaddr_in servaddr;
	socklen_t s = sizeof servaddr;
	memset(&servaddr, 0, s);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = 69;
	
	//bind the socket with the server address
	if (bind(connectionSocket, (const struct sockaddr *) &servaddr, s) < 0)
		exitError("unable to bind socket");

	//check/get sin_port
	getsockname(connectionSocket, (struct sockaddr *) &servaddr,&s);
	printf("port is: %d\n", ntohs(servaddr.sin_port));
	fflush(stdout);

	while (true)
	{
		fd_set rfds = selectOnClients();
		FD_SET(connectionSocket, &rfds);

		if (FD_ISSET(connectionSocket, &rfds))
			acceptClient();
		
		// Loop through clients' ports
			// If FD_ISSET(client.port, &rfds);
				// Read command and dispatch on result
	}

	close(connectionSocket);
}