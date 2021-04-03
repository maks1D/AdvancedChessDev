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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <time.h>

#include "hasher.h"

#define SERVER_BUFFER_LENGTH 10240
#define SERVER_MAX_HEADERS 64
#define SERVER_MAX_CONNECTIONS 1024
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
	unsigned char type;
	time_t lastPacket;
	Server_Socket socket;
} Server_Connection;

typedef struct
{
	const char* path;
	const char* url;
	const char* contentType;
	char* buffer;
	int bufferLength;
} Server_StaticFile;

typedef struct
{
	int staticFilesCount;
	Server_StaticFile* staticFiles;
} Server;

static char* Server_GetTime();
void Server_Start(Server* server, const char* port, const char* internetProtocolVersion);
void Server_SetStaticFilesMaxSize(Server* server, int size);
char Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType);