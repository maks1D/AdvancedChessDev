#include "server.h"

static char* Server_GetTime()
{
	static time_t rawTime;
	time(&rawTime);

	static char* output;
	output = asctime(gmtime(&rawTime));
	output[strlen(output) - 1] = '\x00';

	return output;
}

void Server_Start(Server* server, const char* port, const char* internetProtocolVersion, int mainLoopDelay)
{
	int connectionsCount = 0;
	int* connectionsIndexes = malloc(SERVER_MAX_CONNECTIONS * sizeof(int));

	Server_Connection* connections = malloc(SERVER_MAX_CONNECTIONS * sizeof(Server_Connection));

	for (int index = 0; index < SERVER_MAX_CONNECTIONS; index++)
	{
		connectionsIndexes[index] = index;
		connections[index].type = 0;
	}
	
#ifdef WIN32
	WSADATA windowsSocketsData;

	if (WSAStartup(MAKEWORD(2, 2), &windowsSocketsData) != 0)
	{
		printf("Failed to initialize Windows Sockets!\n");

		return;
	}
#endif

	struct addrinfo addressInformation, *result;

#ifdef WIN32
	ZeroMemory(&addressInformation, sizeof(addressInformation));
#endif

	if (strcmp(internetProtocolVersion, "4") == 0)
	{
		addressInformation.ai_family = AF_INET;
	}
	else if (strcmp(internetProtocolVersion, "6") == 0)
	{
		addressInformation.ai_family = AF_INET6;
	}
	else
	{
		printf("Invalid internet protocol version!\n");

		return;
	}

	addressInformation.ai_socktype = SOCK_STREAM;
	addressInformation.ai_protocol = IPPROTO_TCP;
	addressInformation.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &addressInformation, &result) != 0)
	{
		printf("Failed to get address information!\n");

		return;
	}

	Server_Socket serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (serverSocket == SERVER_INVALID_SOCKET)
	{
		printf("Failed to create a socket!\n");

		return;
	}

#ifdef WIN32
	unsigned long mode = 1;

	if (ioctlsocket(serverSocket, FIONBIO, &mode) != 0)
	{
		printf("Failed to set non-blocking mode!");
		
		return;
	}
#else
	if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) == -1)
	{
		printf("Failed to set non-blocking mode!");

		return;
	}
#endif

	if (bind(serverSocket, result->ai_addr, result->ai_addrlen) == -1)
	{
#ifdef WIN32
		printf("Failed to bind to the port!\n");
#else
		printf("Failed to bind to the port! Try running the application with root permissions.\n");
#endif

		return;
	}

	freeaddrinfo(result);

	if (listen(serverSocket, SOMAXCONN) == -1)
	{
		printf("Failed to listen!\n");

		return;
	}

	printf("Server is now listening on port %s!\n", port);

	char* buffer = malloc(SERVER_BUFFER_LENGTH);
	int responseLength;

#ifndef WIN32
	static struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = mainLoopDelay * 1000000;
#endif

	while (1)
	{
#ifdef WIN32
		Sleep(mainLoopDelay);
#else
		nanosleep(&time, NULL);
#endif

		static int index;

		for (index = 0; index < SERVER_MAX_CONNECTIONS; index++)
		{
			if (connections[index].type == 0)
			{
				continue;
			}

			static int bufferLength;
			bufferLength = recv(connections[index].socket, buffer, SERVER_BUFFER_LENGTH, 0);
			
			if (bufferLength == -1)
			{
#if WIN32
				if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
				if(errno == EAGAIN || errno == EWOULDBLOCK)
#endif
				{
					if (connections[index].lastPacket + SERVER_CONNECTION_TIMEOUT_IN_SECONDS < time(0))
					{
						if (connections[index].type == 1)
						{
							responseLength = sprintf(buffer, "HTTP/1.1 408 Request Timeout\r\nDate: %s\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<h1>Request timeout</h1>", Server_GetTime());
							send(connections[index].socket, buffer, responseLength, 0);
						}

#ifdef WIN32
						closesocket(connections[index].socket);
#else
						close(connections[index].socket);
#endif
						connectionsCount--;
						connections[index].type = 0;
						connectionsIndexes[connectionsCount] = index;
					}
				}
				else
				{
#ifdef WIN32
					closesocket(connections[index].socket);
#else
					close(connections[index].socket);
#endif
					connectionsCount--;
					connections[index].type = 0;
					connectionsIndexes[connectionsCount] = index;
				}

				continue;
			}

			static int position;

			for (position = 1; position < bufferLength; position++)
			{
				if (buffer[position - 1] == '\r' && buffer[position] == '\n')
				{
					buffer[position - 1] = '\x00';
					break;
				}
			}

			position = 0;

			static int length;
			length = strlen(buffer);

			static char* method;
			method = buffer;

			while (position < length && buffer[position] != ' ')
			{
				position++;
			}

			if (position == length)
			{
				responseLength = sprintf(buffer, "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nConnection: close\r\n\r\n", Server_GetTime());
				send(connections[index].socket, buffer, responseLength, 0);

#ifdef WIN32
				closesocket(connections[index].socket);
#else
				close(connections[index].socket);
#endif

				connectionsCount--;
				connections[index].type = 0;
				connectionsIndexes[connectionsCount] = index;

				continue;
			}

			buffer[position] = '\x00';
			position++;

			static char* url;
			url = &buffer[position];

			while (position < length && buffer[position] != ' ' && buffer[position] != '?')
			{
				position++;
			}

			if (position == length)
			{
				responseLength = sprintf(buffer, "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nConnection: close\r\n\r\n", Server_GetTime());
				send(connections[index].socket, buffer, responseLength, 0);

#ifdef WIN32
				closesocket(connections[index].socket);
#else
				close(connections[index].socket);
#endif

				connectionsCount--;
				connections[index].type = 0;
				connectionsIndexes[connectionsCount] = index;

				continue;
			}

			buffer[position] = '\x00';

			if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
			{
				responseLength = sprintf(buffer, "HTTP/1.1 405 Method Not Allowed\r\nDate: %s\r\nConnection: close\r\nAllow: GET, HEAD\r\n\r\n", Server_GetTime());
				send(connections[index].socket, buffer, responseLength, 0);

#ifdef WIN32
				closesocket(connections[index].socket);
#else
				close(connections[index].socket);
#endif

				connectionsCount--;
				connections[index].type = 0;
				connectionsIndexes[connectionsCount] = index;

				continue;
			}

			if (strcmp(url, "/websocket/") == 0)
			{
				// TODO: websockets
			}

			static int file;

			for (file = 0; file < server->staticFilesCount; file++)
			{
				if (strcmp(url, server->staticFiles[file].url) == 0)
				{
					static int fullResponse;
					fullResponse = strcmp(method, "GET") == 0;

					responseLength = sprintf(buffer, "HTTP/1.1 200 OK\r\nDate: %s\r\nConnection: close\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", Server_GetTime(), server->staticFiles[file].contentType, server->staticFiles[file].bufferLength);
					send(connections[index].socket, buffer, responseLength, 0);

					if (fullResponse)
					{
						send(connections[index].socket, server->staticFiles[file].buffer, server->staticFiles[file].bufferLength, 0);
					}

#ifdef WIN32
					closesocket(connections[index].socket);
#else
					close(connections[index].socket);
#endif

					connectionsCount--;
					connections[index].type = 0;
					connectionsIndexes[connectionsCount] = index;

					break;
				}
			}

			if (connections[index].type == 0)
			{
				continue;
			}

			responseLength = sprintf(buffer, "HTTP/1.1 303 See Other\r\nDate: %s\r\nConnection: close\r\nLocation: /\r\n\r\n", Server_GetTime());
			send(connections[index].socket, buffer, responseLength, 0);

#ifdef WIN32
			closesocket(connections[index].socket);
#else
			close(connections[index].socket);
#endif

			connectionsCount--;
			connections[index].type = 0;
			connectionsIndexes[connectionsCount] = index;
		}

		if (connectionsCount < SERVER_MAX_CONNECTIONS)
		{
			static int socket;
			socket = accept(serverSocket, NULL, NULL);

			if (socket == SERVER_INVALID_SOCKET)
			{
				continue;
			}

			static int connectionIndex;
			connectionIndex = connectionsIndexes[connectionsCount];
			connections[connectionIndex].type = 1;
			connections[connectionIndex].lastPacket = time(0);
			connections[connectionIndex].socket = socket;
			connectionsCount++;
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
#ifdef WIN32
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
	{
		printf("Failed to add a static file (%s)!\n", path);
		return;
	}

	LARGE_INTEGER size;
	GetFileSizeEx(file, &size);

	server->staticFiles[server->staticFilesCount].buffer = malloc(size.QuadPart);
	
	if (!ReadFile(file, server->staticFiles[server->staticFilesCount].buffer, size.QuadPart, NULL, NULL))
	{
		printf("Failed read a static file (%s)!\n", path);
		return;
	}

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
		printf("Failed to add a static file (%s)!\n", path);

		return;
	}

	struct stat data;
	fstat(handle, &data);

	server->staticFiles[server->staticFilesCount].buffer = malloc(data.st_size);
	
	if (read(handle, server->staticFiles[server->staticFilesCount].buffer, data.st_size) == -1)
	{
		printf("Failed read a static file (%s)!\n", path);
		return;
	}

	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = data.st_size;
	server->staticFilesCount++;

	close(handle);
#endif
}