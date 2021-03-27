#define _CRT_SECURE_NO_WARNINGS

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
	unsigned int bufferLength;
} Server_StaticFile;

typedef struct
{
	SOCKET socket;
	unsigned int staticFilesCount;
	Server_StaticFile* staticFiles;
} Server;

typedef struct
{
	SOCKET socket;
	Server* server;
} Server_ConnectionThreadArgument;

char* Server_GetTime();
void Server_PrintError(const char* message);
void Server_SetStaticFilesMaxSize(Server* server, unsigned int size);
void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType);
unsigned char Server_Start(Server* server, const char* port);
void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgument);
int Server_GenerateResponse(const char* httpVersion, unsigned int responseCode, const char* responseStatus, char* output, unsigned int outputLength);
int Server_GenerateResponseWithHeader(const char* httpVersion, unsigned int responseCode, const char* responseStatus, const char* header, char* output, unsigned int outputLength);
int Server_GenerateResponseWithBody(const char* httpVersion, unsigned short responseCode, const char* responseStatus, unsigned int bufferLength, const char* buffer, const char* contentType, char* output, unsigned int outputLength);
int Server_GenerateResponseWithBodyOnlyHead(const char* httpVersion, unsigned short responseCode, const char* responseStatus, unsigned int bufferLength, const char* contentType, char* output, unsigned int outputLength);