#include <sys/select.h>

struct channel
{
	char* name;
	int numClients;
	//struct client* clients
};

fd_set selectOnClients()
{
	fd_set set;
	FD_ZERO(&set);

	//for (int i=0; i<numChannels; ++i)
	//	FD_SET();

	return set;
}