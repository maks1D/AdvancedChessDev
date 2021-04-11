#include "handlers.h"

int Handlers_Initialize(Handlers* handlers)
{
	return 1;
}

void Handlers_OnOpen(Handlers* handlers, Server* server, int connectionIndex, char* cookies)
{
	printf("connected\n");
}

void Handlers_OnMessage(Handlers* handlers, Server* server, int connectionIndex, char* message, int messageLength)
{
	printf("message\n");
}

void Handlers_OnClose(Handlers* handlers, Server* server, int connectionIndex)
{
	printf("disconnect\n");
}