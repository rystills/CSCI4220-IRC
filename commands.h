void list(const char* channelName, struct channel* channels, int numChannels)
{
	for (struct channel* channel=channels; channel < channels+numChannels; ++channel)
		if (strcmp(channel -> name, channelName) == 0)
		{
			printf("There are currently %d members.\n", channel -> numClients);
			return;
		}
	
	printf("There are currently %d channels.\n", numChannels);
}

void join()
{
	
}