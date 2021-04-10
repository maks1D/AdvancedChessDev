#include "handlers.h"

int Handlers_Initialize(Handlers* handlers)
{
	return 1;
}

void Handlers_HandleWebSocketConnection(Handlers* handlers, Server* server, int connectionIndex, char* cookies)
{
	printf("connected\n");
}

void Handlers_HandleWebSocketMessage(Handlers* handlers, Server* server, int connectionIndex, char* message, int messageLength)
{
	printf("message\n");
}