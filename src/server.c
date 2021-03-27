#include "server.h"

void Server_PrintError(const char* message)
{
	printf("\x1b[1m\x1b[31m%s\x1b[0m", message);
}

unsigned char Server_Start(Server* server, const char* port)
{
	WSADATA windowsSocketsData;

	if (WSAStartup(MAKEWORD(2, 2), &windowsSocketsData) != NO_ERROR)
	{
		Server_PrintError("Failed to initialize Windows Sockets!\n");

		return 0;
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

		return 0;
	}

	server->socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (server->socket == INVALID_SOCKET)
	{
		Server_PrintError("Failed to create a socket!\n");
		freeaddrinfo(result);
		WSACleanup();

		return 0;
	}

	if (bind(server->socket, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
	{
		Server_PrintError("Failed to bind to the port!\n");
		closesocket(server->socket);
		freeaddrinfo(result);
		WSACleanup();

		return 0;
	}

	freeaddrinfo(result);

	if (listen(server->socket, SOMAXCONN) == SOCKET_ERROR)
	{
		Server_PrintError("Failed to listen!\n");
		closesocket(server->socket);
		WSACleanup();
	}

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
			HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Server_ConnectionThread, connectionThreadArgument, 0, NULL);

			if (thread != NULL)
			{
#pragma warning(push)
#pragma warning (disable : 6001)
				CloseHandle(thread);
#pragma warning(pop)
			}
		}
	}
}

void Server_SetStaticFilesMaxSize(Server* server, unsigned int size)
{
	server->staticFilesCount = 0;
	server->staticFiles = malloc(sizeof(Server_StaticFile) * size);
}

void Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType)
{
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
	{
		Server_PrintError("Failed to add a static file!");
		return;
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);

	server->staticFiles[server->staticFilesCount].buffer = malloc((unsigned int)size.QuadPart + 1);
	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = (unsigned int)size.QuadPart;

	if (server->staticFiles[server->staticFilesCount].buffer == NULL || !ReadFile(file, (void*)server->staticFiles[server->staticFilesCount].buffer, (unsigned int)size.QuadPart, NULL, NULL))
	{
		Server_PrintError("Failed to read a static file!");
	}
	else
	{
		server->staticFiles[server->staticFilesCount].buffer[size.QuadPart] = 0;
		server->staticFilesCount++;
	}

	CloseHandle(file);
}

void Server_ConnectionThread(Server_ConnectionThreadArgument* connectionThreadArgument)
{
	unsigned char* buffer = malloc(SERVER_BUFFER_LENGTH);

	if (buffer == NULL)
	{
		return;
	}

	int result = recv(connectionThreadArgument->socket, buffer, SERVER_BUFFER_LENGTH, 0);

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);

	if (result == SERVER_BUFFER_LENGTH)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);
		int length = Server_GenerateResponse("HTTP/1.1", 413, "Payload Too Large", responseBuffer, SERVER_BUFFER_LENGTH);
		send(connectionThreadArgument->socket, responseBuffer, length, 0);

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

	unsigned int index = 0, length = strlen(buffer);
	unsigned char* method = buffer;

	while (index < length && buffer[index] != ' ')
	{
		index++;
	}

	if (index == length)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);

		if (responseBuffer == NULL)
		{
			closesocket(connectionThreadArgument->socket);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}

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

	unsigned char* url = &buffer[index];

	while (index < length && buffer[index] != ' ')
	{
		index++;
	}

	if (index == length)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);

		if (responseBuffer == NULL)
		{
			closesocket(connectionThreadArgument->socket);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}

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

	unsigned char* httpVersion = &buffer[index];

	if (strcmp(httpVersion, "HTTP/1.0") != 0 && strcmp(httpVersion, "HTTP/1.1") != 0)
	{
		char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);

		if (responseBuffer == NULL)
		{
			closesocket(connectionThreadArgument->socket);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}

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

		if (responseBuffer == NULL)
		{
			closesocket(connectionThreadArgument->socket);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}

		int responseLength = Server_GenerateResponseWithHeader(httpVersion, 405, "Method Not Allowed", "Allow: GET, HEAD", responseBuffer, SERVER_BUFFER_LENGTH);
		send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

		closesocket(connectionThreadArgument->socket);
		free(responseBuffer);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	for (unsigned int index = 0; index < connectionThreadArgument->server->staticFilesCount; index++)
	{
		if (strcmp(url, connectionThreadArgument->server->staticFiles[index].url) == 0)
		{
			char* responseBuffer;
			
			if (strcmp(method, "GET") == 0)
			{
				responseBuffer = malloc(SERVER_BUFFER_LENGTH + connectionThreadArgument->server->staticFiles[index].bufferLength);
			}
			else
			{
				responseBuffer = malloc(SERVER_BUFFER_LENGTH);
			}

			if (responseBuffer == NULL)
			{
				closesocket(connectionThreadArgument->socket);
				free(buffer);
				free(connectionThreadArgument);

				return;
			}

			int responseLength;
			
			if (strcmp(method, "GET") == 0)
			{
				responseLength = Server_GenerateResponseWithBody(httpVersion, 200, "OK", connectionThreadArgument->server->staticFiles[index].bufferLength, connectionThreadArgument->server->staticFiles[index].buffer, connectionThreadArgument->server->staticFiles[index].contentType, responseBuffer, SERVER_BUFFER_LENGTH + connectionThreadArgument->server->staticFiles[index].bufferLength);
			}
			else
			{
				responseLength = Server_GenerateResponseWithBodyOnlyHead(httpVersion, 200, "OK", connectionThreadArgument->server->staticFiles[index].bufferLength, connectionThreadArgument->server->staticFiles[index].contentType, responseBuffer, SERVER_BUFFER_LENGTH);
			}
			
			LARGE_INTEGER end;
			QueryPerformanceCounter(&end);

			send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);

			unsigned int urlLength = strlen(url);
			char* newUrl = malloc(31);

			if (newUrl == NULL)
			{
				closesocket(connectionThreadArgument->socket);
				free(responseBuffer);
				free(buffer);
				free(connectionThreadArgument);

				return;
			}

			if (urlLength > 30)
			{
				for (unsigned int index = 0; index < 27; index++)
				{
					newUrl[index] = url[index];
				}

				for (unsigned int index = 27; index < 30; index++)
				{
					newUrl[index] = '.';
				}
			}
			else
			{
				for (unsigned int index = 0; index < urlLength; index++)
				{
					newUrl[index] = url[index];
				}

				for (unsigned int index = urlLength; index < 30; index++)
				{
					newUrl[index] = ' ';
				}
			}

			newUrl[30] = '\x00';

			double time = ((double)(end.QuadPart - start.QuadPart)) * ((double)1000 / (double)(frequency.QuadPart));
			printf("\x1b[1m\x1b[32m%s \x1b[34m%s \x1b[32m%.3lfms\x1b[0m\n", method, newUrl, time);

			closesocket(connectionThreadArgument->socket);
			free(newUrl);
			free(responseBuffer);
			free(buffer);
			free(connectionThreadArgument);

			return;
		}
	}

	char* responseBuffer = malloc(SERVER_BUFFER_LENGTH);

	if (responseBuffer == NULL)
	{
		closesocket(connectionThreadArgument->socket);
		free(buffer);
		free(connectionThreadArgument);

		return;
	}

	int responseLength = Server_GenerateResponseWithHeader(httpVersion, 303, "See Other", "Location: /", responseBuffer, SERVER_BUFFER_LENGTH);
	send(connectionThreadArgument->socket, responseBuffer, responseLength, 0);

	closesocket(connectionThreadArgument->socket);
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

	if (output != NULL)
	{
		output[strlen(output) - 1] = '\x00';
	}

	return output;
}

int Server_GenerateResponse(const char* httpVersion, unsigned int responseCode, const char* responseStatus, char* output, unsigned int outputLength)
{
	return sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime());
}

int Server_GenerateResponseWithHeader(const char* httpVersion, unsigned int responseCode, const char* responseStatus, const char* header, char* output, unsigned int outputLength)
{
	return sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nConnection: close\r\n%s\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime(), header);
}

int Server_GenerateResponseWithBody(const char* httpVersion, unsigned short responseCode, const char* responseStatus, unsigned int bufferLength, const char* buffer, const char* contentType, char* output, unsigned int outputLength)
{
	int length = sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime(), bufferLength, contentType);

	for (unsigned int index = 0; index < bufferLength; index++)
	{
		output[length + index] = buffer[index];
	}

	return length + bufferLength;
}

int Server_GenerateResponseWithBodyOnlyHead(const char* httpVersion, unsigned short responseCode, const char* responseStatus, unsigned int bufferLength, const char* contentType, char* output, unsigned int outputLength)
{
	return sprintf_s(output, outputLength, "%s %d %s\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", httpVersion, responseCode, responseStatus, Server_GetTime(), bufferLength, contentType);
}