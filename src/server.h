#pragma once
#include "headers.h"
#include "crypto.h"

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
	int maximumNumberOfStaticFiles;
	Server_StaticFile* staticFiles;
	int maximumNumberOfConnections;
	int numberOfConnections;
	int* connectionsIndexes;
	Server_Connection* connections;
	void (*websocketConnectionHandler)(int, char*);
	int (*websocketPacketHandler)(int, char*, int);
} Server;

int Server_Initialize(Server* server);
void Server_SendWebsocketMessage(Server* server, int connectionIndex, char* message, int length);
void Server_CloseConnection(Server* server, int socketIndex);
void Server_Start(Server* server, char* port, char* internetProtocolVersion);
int Server_AddStaticFile(Server* server, char* path, char* url, char* contentType);