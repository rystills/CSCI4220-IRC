#include "channel.h"
#include "linkedList.h"

#ifndef _COMMANDS_GUARD
#define _COMMANDS_GUARD

struct kick
{
	struct channel* channel;
	struct client* user;
};

int checkAlphanumericLength(const char* msg)
{
	if (!isalpha(msg[0]))
		return -1;
	int ans=1;
	while (msg[ans] != '\0')
	{
		if (ans > 20 || (!isalnum(msg[ans]) && msg[ans] != '_'))
			return -1;
		msg++;
		ans++;
	}
	return ans;
}

char* stripChannel(char* msg)
{
	char* ans = strchr(msg, ' ');
	*ans = '\0';
	ans++;

	int channelLength = checkAlphanumericLength(msg+1);
	if (msg[0] != '#' || channelLength == -1 || channelLength > 19)
		return NULL;
	return ans;
}

char* stripUser(struct linkedList* clients, char* msg)
{
	char* ans = strchr(msg, ' ');
	*ans = '\0';
	ans++;

	int userLength = checkAlphanumericLength(msg);
	if (userLength == -1 || userLength > 20)
		return NULL;
	for (struct node* node = clients->head; node != NULL; node = node->next)
		if (strcmp(((struct client*) node -> data) -> nickname, msg) == 0)
			return ans;
	printf("Invalid username\n");
	return NULL;
}

struct kick parseKick(char* channel)
{
	struct kick ans;

	char* user = stripChannel(channel);
	if (user == NULL)
		;

	return ans;
}

#endif