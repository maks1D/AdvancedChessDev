#pragma once
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>

#include "server.h"
#include "database.h"

#define HANDLERS_COOKIES_PARSING_MODE_NAME_BEGIN 0
#define HANDLERS_COOKIES_PARSING_MODE_NAME 1
#define HANDLERS_COOKIES_PARSING_MODE_VALUE 2
#define HANDLERS_DATABASE_TABLE_INDEX_USERS 0
#define HANDLERS_DATABASE_TABLE_INDEX_MESSAGES 1
#define HANDLERS_DATABASE_TABLE_INDEX_GAMES 2
#define HANDLERS_DATABASE_TABLE_INDEX_SECRET 3

int Handlers_Initialize();
void Handlers_HandleWebsocketConnection(Server* server, int connectionIndex, char* cookies);
int Handlers_HandleWebsocketPacket(Server* server, int connectionIndex, char* packet, int length);