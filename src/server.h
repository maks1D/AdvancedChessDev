#pragma once
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS

#include <ws2tcpip.h>
#include <WinSock2.h>

#pragma comment (lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "crypto.h"
#include "error.h"

#define SERVER_BUFFER_LENGTH 10240
#define SERVER_CONNECTION_TIMEOUT_IN_SECONDS 10
#define SERVER_CONNECTION_TYPE_UNCONNECTED 0
#define SERVER_CONNECTION_TYPE_HTTP 1
#define SERVER_CONNECTION_TYPE_WEBSOCKET 2
#define SERVER_HEADERS_PARSING_MODE_NAME 0
#define SERVER_HEADERS_PARSING_NODE_VALUE 1

#if WIN32
#define SERVER_INVALID_SOCKET INVALID_SOCKET

typedef SOCKET Server_Socket;
#else
#define SERVER_INVALID_SOCKET -1

typedef int Server_Socket;
#endif

typedef struct
{
	int userId;
	char type;
	time_t lastPacket;
	Server_Socket socket;
} Server_Connection;

typedef struct
{
	char* path;
	char* url;
	char* contentType;
	char* buffer;
	int bufferLength;
} Server_StaticFile;

typedef struct
{
	int numberOfStaticFiles;
	Server_StaticFile* staticFiles;
	int maxConnections;
	int numberOfConnections;
	int* connectionsIndexes;
	Server_Connection* connections;
	void (*websocketConnectionHandler)(void*, int, char*);
	int (*websocketPacketHandler)(void*, int, char*, int);
} Server;

void Server_SendWebsocketMessage(Server* server, int connectionIndex, char* message, int length);
void Server_CloseConnection(Server* server, int socketIndex);
void Server_Start(Server* server, char* port, char* internetProtocolVersion);
int Server_SetStaticFilesMaxSize(Server* server, int size);
int Server_AddStaticFile(Server* server, char* path, char* url, char* contentType);