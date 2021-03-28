#include <ws2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>
#include <time.h>

#pragma comment (lib, "Ws2_32.lib")

#define SERVER_BUFFER_LENGTH 1024


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
	SOCKET socket;
	int staticFilesCount;
	Server_StaticFile* staticFiles;
} Server;

typedef struct
{
	SOCKET socket;
	Server* server;
} Server_ConnectionThreadArgument;

char* Server_GetTime();
void Server_SetStaticFilesMaxSize(Server* server, int size);
void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType);
void Server_Start(Server* server, const char* port);
void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgument);
int Server_GenerateResponse(const char* httpVersion, int responseCode, const char* responseStatus, char* output, int outputLength);
int Server_GenerateResponseWithHeader(const char* httpVersion, int responseCode, const char* responseStatus, const char* header, char* output, int outputLength);
int Server_GenerateResponseWithBody(const char* httpVersion, int responseCode, const char* responseStatus, int bufferLength, const char* contentType, char* output, int outputLength);