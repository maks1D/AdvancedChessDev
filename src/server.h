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
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <stdio.h>
#include <time.h>

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
	int socket;
	int staticFilesCount;
	Server_StaticFile* staticFiles;
} Server;

typedef struct
{
	int socket;
	Server* server;
} Server_ConnectionThreadArgument;

void Server_PrintError(const char* message);
void Server_PrintMessage(const char* message);
char* Server_GetTime();
void Server_SetStaticFilesMaxSize(Server* server, int size);
void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType);
void Server_Start(Server* server, const char* port);
void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgument);
int Server_GenerateResponse(const char* httpVersion, int responseCode, const char* responseStatus, char* output);
int Server_GenerateResponseWithHeader(const char* httpVersion, int responseCode, const char* responseStatus, const char* header, char* output);
int Server_GenerateResponseWithBody(const char* httpVersion, int responseCode, const char* responseStatus, int bufferLength, const char* contentType, char* output);