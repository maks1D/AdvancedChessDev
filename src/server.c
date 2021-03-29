#include "server.h"

void Server_PrintError(const char* message)
{
#ifdef WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 4);

	printf("%s", message);

	SetConsoleTextAttribute(console, 7);
#else
	printf("\x1b[31m%s\x1b[0m", message);
#endif
}

void Server_PrintMessage(const char* message)
{
#ifdef WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 2);

	printf("%s", message);

	SetConsoleTextAttribute(console, 7);
#else
	printf("\x1b[32m%s\x1b[0m", message);
#endif
}

void Server_Start(Server* server, const char* port)
{
#ifdef WIN32
	WSADATA windowsSocketsData;

	if (WSAStartup(MAKEWORD(2, 2), &windowsSocketsData) != NO_ERROR)
	{
		Server_PrintError("Failed to initialize Windows Sockets!\n");

		return;
	}
#endif

	struct addrinfo addressInformation, *result;

#ifdef WIN32
	ZeroMemory(&addressInformation, sizeof(addressInformation));
#endif

	addressInformation.ai_family = AF_INET;
	addressInformation.ai_socktype = SOCK_STREAM;
	addressInformation.ai_protocol = IPPROTO_TCP;
	addressInformation.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &addressInformation, &result) != 0)
	{
		Server_PrintError("Failed to get address information!\n");

#ifdef WIN32
		WSACleanup();
#endif

		return;
	}

	server->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (server->socket == -1)
	{
		Server_PrintError("Failed to create a socket!\n");
		freeaddrinfo(result);

#ifdef WIN32
		WSACleanup();
#endif

		return;
	}

	if (bind(server->socket, result->ai_addr, result->ai_addrlen) == -1)
	{
		Server_PrintError("Failed to bind to the port!\n");

#ifdef WIN32
		closesocket(server->socket);
#else
		close(server->socket);
#endif

		freeaddrinfo(result);

#ifdef WIN32
		WSACleanup();
#endif

		return;
	}

	freeaddrinfo(result);

	if (listen(server->socket, SOMAXCONN) == -1)
	{
		Server_PrintError("Failed to listen!\n");

#ifdef WIN32
		closesocket(server->socket);
		WSACleanup();
#else
		close(server->socket);
#endif
		return;
	}

	Server_PrintMessage("Server started!\n");

	Server_ConnectionThreadArgument connectionThreadArgument;
	connectionThreadArgument.server = server;
	connectionThreadArgument.socket = -1;

	while (1)
	{
		int socket = accept(server->socket, NULL, NULL);

		if (socket == -1)
		{
			continue;
		}

		while (connectionThreadArgument.socket != -1)
		{
			printf("");
		}

		connectionThreadArgument.socket = socket;

#ifdef WIN32
		CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Server_ConnectionThread, &connectionThreadArgument, 0, NULL));
#else
		pthread_t thread;
		pthread_create(&thread, NULL, (void*)&Server_ConnectionThread, &connectionThreadArgument);
#endif
	}
}

void Server_SetStaticFilesMaxSize(Server* server, int size)
{
	server->staticFilesCount = 0;
	server->staticFiles = malloc(sizeof(Server_StaticFile) * size);
}

void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType)
{
#ifdef WIN32
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
	{
		Server_PrintError("Failed to add a static file!\n");
		return;
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);

	server->staticFiles[server->staticFilesCount].buffer = malloc(size.QuadPart);
	ReadFile(file, server->staticFiles[server->staticFilesCount].buffer, size.QuadPart, NULL, NULL);

	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = size.QuadPart;
	server->staticFilesCount++;

	CloseHandle(file);
#else
	int handle = open(path, O_RDONLY);

	if(handle == -1)
	{
		Server_PrintError("Failed to add a static file!\n");

		return;
	}

	struct stat data;
	fstat(handle, &data);

	server->staticFiles[server->staticFilesCount].buffer = malloc(data.st_size);
	read(handle, server->staticFiles[server->staticFilesCount].buffer, data.st_size);

	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = data.st_size;
	server->staticFilesCount++;

	close(handle);
#endif
}

void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgumentPointer)
{
	Server_ConnectionThreadArgument connectionThreadArgument;
	connectionThreadArgument.server = connectionThreadArgumentPointer->server;
	connectionThreadArgument.socket = connectionThreadArgumentPointer->socket;

	connectionThreadArgumentPointer->socket = -1;

	char* buffer = malloc(SERVER_BUFFER_LENGTH);
	int result = recv(connectionThreadArgument.socket, buffer, SERVER_BUFFER_LENGTH, 0);

	if (result < 0)
	{
#ifdef WIN32
		closesocket(connectionThreadArgument.socket);
#else
		close(connectionThreadArgument.socket);
#endif

		free(buffer);

		return;
	}

	for (int index = 1; index < result; index++)
	{
		if (buffer[index - 1] == '\r' && buffer[index] == '\n')
		{
			buffer[index - 1] = '\x00';
			break;
		}
	}

	int index = 0, length = strlen(buffer);
	char* method = buffer;

	while (index < length && buffer[index] != ' ')
	{
		index++;
	}

	if (index == length)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponse(400, "Bad Request", responseBuffer);
		
		send(connectionThreadArgument.socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument.socket);
#else
		close(connectionThreadArgument.socket);
#endif

		free(responseBuffer);
		free(buffer);

		return;
	}

	buffer[index] = '\x00';
	index++;

	char* url = &buffer[index];

	while (index < length && buffer[index] != ' ' && buffer[index] != '?')
	{
		index++;
	}

	if (index == length)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponse(400, "Bad Request", responseBuffer);
		
		send(connectionThreadArgument.socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument.socket);
#else
		close(connectionThreadArgument.socket);
#endif

		free(responseBuffer);
		free(buffer);

		return;
	}

	buffer[index] = '\x00';

	if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponseWithHeader(405, "Method Not Allowed", "Allow: GET, HEAD", responseBuffer);
		
		send(connectionThreadArgument.socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument.socket);
#else
		close(connectionThreadArgument.socket);
#endif

		free(responseBuffer);
		free(buffer);

		return;
	}

	for (int index = 0; index < connectionThreadArgument.server->staticFilesCount; index++)
	{
		if (strcmp(url, connectionThreadArgument.server->staticFiles[index].url) == 0)
		{
			char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
			int responseLength = Server_GenerateResponseWithBody(200, "OK", connectionThreadArgument.server->staticFiles[index].bufferLength, connectionThreadArgument.server->staticFiles[index].contentType, responseBuffer);

			send(connectionThreadArgument.socket, responseBuffer, responseLength, 0);

			if (strcmp(method, "GET") == 0)
			{
				send(connectionThreadArgument.socket, connectionThreadArgument.server->staticFiles[index].buffer, connectionThreadArgument.server->staticFiles[index].bufferLength, 0);
			}
			
#ifdef WIN32
			closesocket(connectionThreadArgument.socket);
#else
			close(connectionThreadArgument.socket);
#endif

			free(responseBuffer);
			free(buffer);

			return;
		}
	}

	char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
	int responseLength = Server_GenerateResponseWithHeader(303, "See Other", "Location: /", responseBuffer);
	
	send(connectionThreadArgument.socket, responseBuffer, responseLength, 0);

#ifdef WIN32
	closesocket(connectionThreadArgument.socket);
#else
	close(connectionThreadArgument.socket);
#endif

	free(responseBuffer);
	free(buffer);
}

char* Server_GetTime()
{
	time_t rawTime;
	time(&rawTime);

	char* output = asctime(gmtime(&rawTime));
	output[strlen(output) - 1] = '\x00';

	return output;
}

int Server_GenerateResponse(int responseCode, const char* responseStatus, char* output)
{
	return sprintf(output, "HTTP/1.1 %d %s\r\nDate: %s\r\nConnection: close\r\n\r\n", responseCode, responseStatus, Server_GetTime());
}

int Server_GenerateResponseWithHeader(int responseCode, const char* responseStatus, const char* header, char* output)
{
	return sprintf(output, "HTTP/1.1 %d %s\r\nDate: %s\r\nConnection: close\r\n%s\r\n\r\n", responseCode, responseStatus, Server_GetTime(), header);
}

int Server_GenerateResponseWithBody(int responseCode, const char* responseStatus, int bufferLength, const char* contentType, char* output)
{
	return sprintf(output, "HTTP/1.1 %d %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", responseCode, responseStatus, Server_GetTime(), bufferLength, contentType);
}