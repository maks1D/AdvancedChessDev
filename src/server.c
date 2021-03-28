#include "server.h"

void Server_PrintError(const char* message)
{
#ifdef WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 4);

	printf("%s", message);

	SetConsoleTextAttribute(console, 7);
	CloseHandle(console);
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
	CloseHandle(console);
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

	while (1)
	{
		int socket = accept(server->socket, NULL, NULL);

		if (socket == -1)
		{
			continue;
		}

		Server_ConnectionThreadArgument* connectionThreadArgument = malloc(sizeof(Server_ConnectionThreadArgument));
		connectionThreadArgument->server = server;
		connectionThreadArgument->socket = socket;

#ifdef WIN32
		CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Server_ConnectionThread, connectionThreadArgument, 0, NULL));
#else
		pthread_t thread;
		pthread_create(&thread, NULL, (void*)&Server_ConnectionThread, connectionThreadArgument);
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

void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgument)
{
	char* buffer = malloc(SERVER_BUFFER_LENGTH);
	int result = recv(connectionThreadArgument->socket, buffer, SERVER_BUFFER_LENGTH, 0);

	if (result == SERVER_BUFFER_LENGTH)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int bufferLength = Server_GenerateResponse("HTTP/1.1", 413, "Payload Too Large", responseBuffer);
		
		send(connectionThreadArgument->socket, responseBuffer, bufferLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument->socket);
#else
		close(connectionThreadArgument->socket);
#endif

		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	if (result < 0)
	{
#ifdef WIN32
		closesocket(connectionThreadArgument->socket);
#else
		close(connectionThreadArgument->socket);
#endif

		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	for (int index = 1; index < result; index++)
	{
		if (buffer[index - 1] == '\r' && buffer[index] == '\n')
		{
			buffer[index - 1] = '\x00';
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
		int responseLength = Server_GenerateResponse("HTTP/1.1", 400, "Bad Request", responseBuffer);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument->socket);
#else
		close(connectionThreadArgument->socket);
#endif

		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	buffer[index] = '\x00';
	index++;

	char* url = &buffer[index];

	while (index < length && buffer[index] != ' ')
	{
		index++;
	}

	if (index == length)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponse("HTTP/1.1", 400, "Bad Request", responseBuffer);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument->socket);
#else
		close(connectionThreadArgument->socket);
#endif

		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	buffer[index] = '\x00';
	index++;

	char* httpVersion = &buffer[index];

	if (strcmp(httpVersion, "HTTP/1.0") != 0 && strcmp(httpVersion, "HTTP/1.1") != 0)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponse("HTTP/1.1", 400, "Bad Request", responseBuffer);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument->socket);
#else
		close(connectionThreadArgument->socket);
#endif

		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponseWithHeader(httpVersion, 405, "Method Not Allowed", "Allow: GET, HEAD", responseBuffer);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

#ifdef WIN32
		closesocket(connectionThreadArgument->socket);
#else
		close(connectionThreadArgument->socket);
#endif

		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	for (int index = 0; index < connectionThreadArgument->server->staticFilesCount; index++)
	{
		if (strcmp(url, connectionThreadArgument->server->staticFiles[index].url) == 0)
		{
			char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
			int responseLength = Server_GenerateResponseWithBody(httpVersion, 200, "OK", connectionThreadArgument->server->staticFiles[index].bufferLength, connectionThreadArgument->server->staticFiles[index].contentType, responseBuffer);

			send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

			if (strcmp(method, "GET") == 0)
			{
				send(connectionThreadArgument->socket, connectionThreadArgument->server->staticFiles[index].buffer, connectionThreadArgument->server->staticFiles[index].bufferLength, 0);
			}
			
#ifdef WIN32
			closesocket(connectionThreadArgument->socket);
#else
			close(connectionThreadArgument->socket);
#endif

			free(responseBuffer);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}
	}

	char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
	int responseLength = Server_GenerateResponseWithHeader(httpVersion, 303, "See Other", "Location: /", responseBuffer);
	
	send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

#ifdef WIN32
	closesocket(connectionThreadArgument->socket);
#else
	close(connectionThreadArgument->socket);
#endif

	free(responseBuffer);
	free(buffer);
	free(connectionThreadArgument);
}

char* Server_GetTime()
{
	time_t rawTime;
	time(&rawTime);

	struct tm* timeInfo = gmtime(&rawTime);

	char* output = asctime(timeInfo);
	output[strlen(output) - 1] = '\x00';

	return output;
}

int Server_GenerateResponse(const char* httpVersion, int responseCode, const char* responseStatus, char* output)
{
	return sprintf(output, "%s %d %s\r\nDate: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime());
}

int Server_GenerateResponseWithHeader(const char* httpVersion, int responseCode, const char* responseStatus, const char* header, char* output)
{
	return sprintf(output, "%s %d %s\r\nDate: %s\r\nConnection: close\r\n%s\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime(), header);
}

int Server_GenerateResponseWithBody(const char* httpVersion, int responseCode, const char* responseStatus, int bufferLength, const char* contentType, char* output)
{
	return sprintf(output, "%s %d %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime(), bufferLength, contentType);
}