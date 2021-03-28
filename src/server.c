#include "server.h"

#pragma warning(disable : 4244 6001 6011 6031 6387)

void Server_PrintError(const char* message)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 4);

	printf("%s", message);

	SetConsoleTextAttribute(console, 7);
	CloseHandle(console);
}

void Server_PrintMessage(const char* message)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(console, 2);

	printf("%s", message);

	SetConsoleTextAttribute(console, 7);
	CloseHandle(console);
}

void Server_Start(Server* server, const char* port)
{
	WSADATA windowsSocketsData;

	if (WSAStartup(MAKEWORD(2, 2), &windowsSocketsData) != NO_ERROR)
	{
		Server_PrintError("Failed to initialize Windows Sockets!\n");

		return;
	}

	struct addrinfo addressInformation, *result;

	ZeroMemory(&addressInformation, sizeof(addressInformation));
	addressInformation.ai_family = AF_INET;
	addressInformation.ai_socktype = SOCK_STREAM;
	addressInformation.ai_protocol = IPPROTO_TCP;
	addressInformation.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &addressInformation, &result) != NO_ERROR)
	{
		Server_PrintError("Failed to get address information!\n");
		WSACleanup();

		return;
	}

	server->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (server->socket == INVALID_SOCKET)
	{
		Server_PrintError("Failed to create a socket!\n");
		freeaddrinfo(result);
		WSACleanup();

		return;
	}

	if (bind(server->socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
	{
		Server_PrintError("Failed to bind to the port!\n");
		closesocket(server->socket);
		freeaddrinfo(result);
		WSACleanup();

		return;
	}

	freeaddrinfo(result);

	if (listen(server->socket, SOMAXCONN) == SOCKET_ERROR)
	{
		Server_PrintError("Failed to listen!\n");
		closesocket(server->socket);
		WSACleanup();

		return;
	}

	Server_PrintMessage("Server started!\n");

	while (1)
	{
		SOCKET socket = accept(server->socket, NULL, NULL);

		if (socket != INVALID_SOCKET)
		{
			Server_ConnectionThreadArgument* connectionThreadArgument = malloc(sizeof(Server_ConnectionThreadArgument));

			if (connectionThreadArgument == NULL)
			{
				continue;
			}

			connectionThreadArgument->server = server;
			connectionThreadArgument->socket = socket;

			CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Server_ConnectionThread, connectionThreadArgument, 0, NULL));
		}
	}
}

void Server_SetStaticFilesMaxSize(Server* server, int size)
{
	server->staticFilesCount = 0;
	server->staticFiles = malloc(sizeof(Server_StaticFile) * size);
}

void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType)
{
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
	{
		Server_PrintError("Failed to add a static file!\n");
		return;
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);

	server->staticFiles[server->staticFilesCount].buffer = malloc(size.QuadPart + 1);
	ReadFile(file, server->staticFiles[server->staticFilesCount].buffer, size.QuadPart, NULL, NULL);

	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = size.QuadPart;
	server->staticFiles[server->staticFilesCount].buffer[size.QuadPart] = 0;
	server->staticFilesCount++;

	CloseHandle(file);
}

void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgument)
{
	char* buffer = malloc(SERVER_BUFFER_LENGTH);
	int result = recv(connectionThreadArgument->socket, buffer, SERVER_BUFFER_LENGTH, 0);

	if (result == SERVER_BUFFER_LENGTH)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int bufferLength = Server_GenerateResponse("HTTP/1.1", 413, "Payload Too Large", responseBuffer, SERVER_BUFFER_LENGTH);
		
		send(connectionThreadArgument->socket, responseBuffer, bufferLength, 0);

		closesocket(connectionThreadArgument->socket);
		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	if (result < 0)
	{
		closesocket(connectionThreadArgument->socket);
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
		int responseLength = Server_GenerateResponse("HTTP/1.1", 400, "Bad Request", responseBuffer, SERVER_BUFFER_LENGTH);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

		closesocket(connectionThreadArgument->socket);
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
		int responseLength = Server_GenerateResponse("HTTP/1.1", 400, "Bad Request", responseBuffer, SERVER_BUFFER_LENGTH);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

		closesocket(connectionThreadArgument->socket);
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
		int responseLength = Server_GenerateResponse("HTTP/1.1", 400, "Bad Request", responseBuffer, SERVER_BUFFER_LENGTH);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

		closesocket(connectionThreadArgument->socket);
		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int responseLength = Server_GenerateResponseWithHeader(httpVersion, 405, "Method Not Allowed", "Allow: GET, HEAD", responseBuffer, SERVER_BUFFER_LENGTH);
		
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

		closesocket(connectionThreadArgument->socket);
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
			int responseLength = Server_GenerateResponseWithBody(httpVersion, 200, "OK", connectionThreadArgument->server->staticFiles[index].bufferLength, connectionThreadArgument->server->staticFiles[index].contentType, responseBuffer, SERVER_BUFFER_LENGTH);

			send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

			if (strcmp(method, "GET") == 0)
			{
				send(connectionThreadArgument->socket, connectionThreadArgument->server->staticFiles[index].buffer, connectionThreadArgument->server->staticFiles[index].bufferLength, 0);
			}
			
			closesocket(connectionThreadArgument->socket);
			free(responseBuffer);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}
	}

	char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
	int responseLength = Server_GenerateResponseWithHeader(httpVersion, 303, "See Other", "Location: /", responseBuffer, SERVER_BUFFER_LENGTH);
	
	send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

	closesocket(connectionThreadArgument->socket);
	free(responseBuffer);
	free(buffer);
	free(connectionThreadArgument);
}

char* Server_GetTime()
{
	time_t timestamp;
	time(&timestamp);

	struct tm timeInformation;
	gmtime_s(&timeInformation, &timestamp);

	char* buffer = malloc(30);
	asctime_s(buffer, 30, &timeInformation);
	buffer[strlen(buffer) - 1] = '\x00';

	return buffer;
}

int Server_GenerateResponse(const char* httpVersion, int responseCode, const char* responseStatus, char* output, int outputLength)
{
	char* time = Server_GetTime();
	int length = sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, time);

	free(time);

	return length;
}

int Server_GenerateResponseWithHeader(const char* httpVersion, int responseCode, const char* responseStatus, const char* header, char* output, int outputLength)
{
	char* time = Server_GetTime();
	int length = sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nConnection: close\r\n%s\r\n\r\n", httpVersion, responseCode, responseStatus, time, header);

	free(time);

	return length;
}

int Server_GenerateResponseWithBody(const char* httpVersion, int responseCode, const char* responseStatus, int bufferLength, const char* contentType, char* output, int outputLength)
{
	char* time = Server_GetTime();
	int length = sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, time, bufferLength, contentType);

	free(time);

	return length;
}