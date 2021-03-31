#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable : 4244 6001 6011 6031 6387 28182 28183)

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

#define SERVER_BUFFER_LENGTH 1024
#define SERVER_MAX_CONNECTIONS 64
#define SERVER_CONNECTION_TIMEOUT_IN_SECONDS 10

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
void Server_Start(Server* server, const char* port, const char* internetProtocolVersion, int mainLoopDelay);
void Server_SetStaticFilesMaxSize(Server* server, int size);
void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType);