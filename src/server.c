#include "server.h"

static char* Server_GetTime()
{
	static time_t rawTime;
	time(&rawTime);

	static char* output;
	output = asctime(gmtime(&rawTime));

	if (output == NULL)
	{
		return "";
	}

	output[strlen(output) - 1] = '\x00';

	return output;
}

void Server_Start(Server* server, const char* port, const char* internetProtocolVersion)
{
	int connectionsCount = 0;
	int* connectionsIndexes = malloc(SERVER_MAX_CONNECTIONS * sizeof(int));

	Server_Connection* connections = malloc(SERVER_MAX_CONNECTIONS * sizeof(Server_Connection));

	if (connectionsIndexes == NULL || connections == NULL)
	{
		printf("Failed to allocate memory!\n");
		return;
	}

	for (int index = 0; index < SERVER_MAX_CONNECTIONS; index++)
	{
		connectionsIndexes[index] = index;
		connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
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

	int reuseAddress = 1;

	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddress, sizeof(reuseAddress)) == -1)
	{
		printf("Failed set socket options!\n");
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
	char* websocketKeyUnhashed = malloc(SERVER_BUFFER_LENGTH + 100);

	while (1)
	{
		static int index;

		for (index = 0; index < SERVER_MAX_CONNECTIONS; index++)
		{
			if (connections[index].type == SERVER_CONNECTION_TYPE_UNCONNECTED)
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
				if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
				{
					if (connections[index].lastPacket + SERVER_CONNECTION_TIMEOUT_IN_SECONDS < time(0))
					{
						if (connections[index].type == SERVER_CONNECTION_TYPE_HTTP)
						{
							bufferLength = sprintf(buffer, "HTTP/1.1 408 Request Timeout\r\nDate: %s\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<h1>Request timeout</h1>", Server_GetTime());
							send(connections[index].socket, buffer, bufferLength, 0);
						}

#ifdef WIN32
						closesocket(connections[index].socket);
#else
						close(connections[index].socket);
#endif
						connectionsCount--;
						connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
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
					connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
					connectionsIndexes[connectionsCount] = index;
				}

				continue;
			}

			if (connections[index].type == SERVER_CONNECTION_TYPE_WEBSOCKET)
			{
				connections[index].lastPacket = time(0);

			}
			else
			{
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

				static int headLength;
				headLength = strlen(buffer);

				static char* method;
				method = buffer;

				while (position < headLength && buffer[position] != ' ')
				{
					position++;
				}

				if (position == headLength)
				{
					bufferLength = sprintf(buffer, "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nConnection: close\r\n\r\n", Server_GetTime());
					send(connections[index].socket, buffer, bufferLength, 0);

#ifdef WIN32
					closesocket(connections[index].socket);
#else
					close(connections[index].socket);
#endif

					connectionsCount--;
					connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
					connectionsIndexes[connectionsCount] = index;

					continue;
				}

				buffer[position] = '\x00';
				position++;

				static char* url;
				url = &buffer[position];

				while (position < headLength && buffer[position] != ' ' && buffer[position] != '?')
				{
					position++;
				}

				if (position == headLength)
				{
					bufferLength = sprintf(buffer, "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nConnection: close\r\n\r\n", Server_GetTime());
					send(connections[index].socket, buffer, bufferLength, 0);

#ifdef WIN32
					closesocket(connections[index].socket);
#else
					close(connections[index].socket);
#endif

					connectionsCount--;
					connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
					connectionsIndexes[connectionsCount] = index;

					continue;
				}

				buffer[position] = '\x00';

				if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
				{
					bufferLength = sprintf(buffer, "HTTP/1.1 405 Method Not Allowed\r\nDate: %s\r\nConnection: close\r\nAllow: GET, HEAD\r\n\r\n", Server_GetTime());
					send(connections[index].socket, buffer, bufferLength, 0);

#ifdef WIN32
					closesocket(connections[index].socket);
#else
					close(connections[index].socket);
#endif

					connectionsCount--;
					connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
					connectionsIndexes[connectionsCount] = index;

					continue;
				}

				if (strcmp(url, "/websocket/") == 0)
				{
					static char* upgradeHeader;
					upgradeHeader = NULL;

					char* websocketKeyHeader;
					websocketKeyHeader = NULL;

					static char* headerName;
					headerName = &buffer[headLength + 2];

					static char* headerValue;

					static int parsingMode;
					parsingMode = SERVER_HEADERS_PARSING_MODE_NAME;

					for (position = headLength + 3; position < bufferLength; position++)
					{
						if (parsingMode == SERVER_HEADERS_PARSING_MODE_NAME)
						{
							if (buffer[position - 1] == ':' && buffer[position] == ' ')
							{
								buffer[position - 1] = '\x00';

								position++;
								headerValue = &buffer[position];
								parsingMode = SERVER_HEADERS_PARSING_NODE_VALUE;
							}
						}
						else if (buffer[position - 1] == '\r' && buffer[position] == '\n')
						{
							buffer[position - 1] = '\x00';

							if (strcmp(headerName, "Upgrade") == 0)
							{
								upgradeHeader = headerValue;
							}
							else if (strcmp(headerName, "Sec-WebSocket-Key") == 0)
							{
								websocketKeyHeader = headerValue;
							}

							position++;
							headerName = &buffer[position];
							parsingMode = SERVER_HEADERS_PARSING_MODE_NAME;
						}
					}

					if (upgradeHeader == NULL || websocketKeyHeader == NULL || strcmp(upgradeHeader, "websocket") != 0)
					{
						bufferLength = sprintf(buffer, "HTTP/1.1 303 See Other\r\nDate: %s\r\nConnection: close\r\nLocation: /\r\n\r\n", Server_GetTime());
						send(connections[index].socket, buffer, bufferLength, 0);

#ifdef WIN32
						closesocket(connections[index].socket);
#else
						close(connections[index].socket);
#endif

						connectionsCount--;
						connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
						connectionsIndexes[connectionsCount] = index;

						continue;
					}

					sprintf(websocketKeyUnhashed, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", websocketKeyHeader);

					bufferLength = sprintf(buffer, "HTTP/1.1 101 Switching Protocols\r\nDate: %s\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Accept: %s\r\n\r\n", Server_GetTime(), Hasher_Hash(websocketKeyUnhashed, strlen(websocketKeyUnhashed)));
					send(connections[index].socket, buffer, bufferLength, 0);

					connections[index].type = SERVER_CONNECTION_TYPE_WEBSOCKET;

					continue;
				}

				static int file;

				for (file = 0; file < server->staticFilesCount; file++)
				{
					if (strcmp(url, server->staticFiles[file].url) == 0)
					{
						static int fullResponse;
						fullResponse = strcmp(method, "GET");

						bufferLength = sprintf(buffer, "HTTP/1.1 200 OK\r\nDate: %s\r\nConnection: close\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", Server_GetTime(), server->staticFiles[file].contentType, server->staticFiles[file].bufferLength);
						send(connections[index].socket, buffer, bufferLength, 0);

						if (fullResponse == 0)
						{
							send(connections[index].socket, server->staticFiles[file].buffer, server->staticFiles[file].bufferLength, 0);
						}

#ifdef WIN32
						closesocket(connections[index].socket);
#else
						close(connections[index].socket);
#endif

						connectionsCount--;
						connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
						connectionsIndexes[connectionsCount] = index;

						break;
					}
				}

				if (connections[index].type == SERVER_CONNECTION_TYPE_UNCONNECTED)
				{
					continue;
				}

				bufferLength = sprintf(buffer, "HTTP/1.1 303 See Other\r\nDate: %s\r\nConnection: close\r\nLocation: /\r\n\r\n", Server_GetTime());
				send(connections[index].socket, buffer, bufferLength, 0);

#ifdef WIN32
				closesocket(connections[index].socket);
#else
				close(connections[index].socket);
#endif

				connectionsCount--;
				connections[index].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
				connectionsIndexes[connectionsCount] = index;
			}
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
			connections[connectionIndex].type = SERVER_CONNECTION_TYPE_HTTP;
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

char Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType)
{
	static int length;
	length = Filesystem_ReadFile(path, &server->staticFiles[server->staticFilesCount].buffer, 1);

	if (length == -1)
	{
		printf("Failed to add a static file (%s)!\n", path);
		return 0;
	}

	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = length;
	server->staticFilesCount++;

	return 1;
}