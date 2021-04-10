#pragma once
#include "headers.h"
#include "server.h"
#include "database.h"

typedef struct
{
	Database database;
} Handlers;

int Handlers_Initialize(Handlers* handlers);
void Handlers_HandleWebSocketConnection(Handlers* handlers, Server* server, int connectionIndex, char* cookies);
void Handlers_HandleWebSocketMessage(Handlers* handlers, Server* server, int connectionIndex, char* message, int messageLength);