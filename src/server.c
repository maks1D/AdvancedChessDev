#include "server.h"

char* Server_GetTime()
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

void Server_CloseConnection(Server* server, int connectionIndex)
{
#ifdef WIN32
	closesocket(server->connections[connectionIndex].socket);
#else
	close(connections[index].socket);
#endif
	server->connectionsCount--;
	server->connections[connectionIndex].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
	server->connectionsIndexes[server->connectionsCount] = connectionIndex;
}

void Server_Start(Server* server, const char* port, const char* internetProtocolVersion)
{
	server->connectionsCount = 0;
	server->maxConnections = 1;
	server->connectionsIndexes = malloc(sizeof(int));
	server->connections = malloc(sizeof(Server_Connection));

	if (server->connectionsIndexes == NULL || server->connections == NULL)
	{
		ERROR("Failed to allocate memory!");
		return;
	}

	server->connectionsIndexes[0] = 0;
	server->connections[0].type = SERVER_CONNECTION_TYPE_UNCONNECTED;
	
#ifdef WIN32
	WSADATA windowsSocketsData;

	if (WSAStartup(MAKEWORD(2, 2), &windowsSocketsData) != 0)
	{
		ERROR("Failed to initialize Windows Sockets!");
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
		ERROR("Invalid Internet Protocol version!");
		return;
	}

	addressInformation.ai_socktype = SOCK_STREAM;
	addressInformation.ai_protocol = IPPROTO_TCP;
	addressInformation.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &addressInformation, &result) != 0)
	{
		ERROR("Failed to get address information!");
		return;
	}

	Server_Socket serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (serverSocket == SERVER_INVALID_SOCKET)
	{
		ERROR("Failed to create a socket!");
		return;
	}

	int reuseAddress = 1;

	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddress, sizeof(reuseAddress)) == -1)
	{
		ERROR("Failed set socket options!");
		return;
	}

#ifdef WIN32
	unsigned long mode = 1;

	if (ioctlsocket(serverSocket, FIONBIO, &mode) != 0)
	{
		ERROR("Failed to set non-blocking mode!");
		return;
	}
#else
	if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) == -1)
	{
		ERROR("Failed to set non-blocking mode!");
		return;
	}
#endif

	if (bind(serverSocket, result->ai_addr, result->ai_addrlen) == -1)
	{
#ifdef WIN32
		ERROR("Failed to bind to the port!");
#else
		ERROR("Failed to bind to the port! Try running the application with root permissions.");
#endif

		return;
	}

	freeaddrinfo(result);

	if (listen(serverSocket, SOMAXCONN) == -1)
	{
		ERROR("Failed to listen!");
		return;
	}

	printf("Server is now listening on port %s!\n", port);

	char* buffer = malloc(SERVER_BUFFER_LENGTH);
	char* websocketKeyUnhashed = malloc(SERVER_BUFFER_LENGTH + 100);

	while (1)
	{
		static int index;

		for (index = 0; index < server->maxConnections; index++)
		{
			if (server->connections[index].type == SERVER_CONNECTION_TYPE_UNCONNECTED)
			{
				continue;
			}

			static int bufferLength;
			bufferLength = recv(server->connections[index].socket, buffer, SERVER_BUFFER_LENGTH, 0);

			if (bufferLength == -1)
			{
#if WIN32
				if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
				if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
				{
					if (server->connections[index].lastPacket + SERVER_CONNECTION_TIMEOUT_IN_SECONDS < time(0))
					{
						if (server->connections[index].type == SERVER_CONNECTION_TYPE_HTTP)
						{
							bufferLength = sprintf(buffer, "HTTP/1.1 408 Request Timeout\r\nDate: %s\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<h1>Request timeout</h1>", Server_GetTime());
							send(server->connections[index].socket, buffer, bufferLength, 0);
						}

						Server_CloseConnection(server, index);
					}
				}
				else
				{
					Server_CloseConnection(server, index);
				}

				continue;
			}

			if (server->connections[index].type == SERVER_CONNECTION_TYPE_WEBSOCKET)
			{
				server->connections[index].lastPacket = time(0);
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
					send(server->connections[index].socket, buffer, bufferLength, 0);
					Server_CloseConnection(server, index);

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
					send(server->connections[index].socket, buffer, bufferLength, 0);
					Server_CloseConnection(server, index);

					continue;
				}

				buffer[position] = '\x00';

				if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
				{
					bufferLength = sprintf(buffer, "HTTP/1.1 405 Method Not Allowed\r\nDate: %s\r\nConnection: close\r\nAllow: GET, HEAD\r\n\r\n", Server_GetTime());
					send(server->connections[index].socket, buffer, bufferLength, 0);
					Server_CloseConnection(server, index);

					continue;
				}

				if (strcmp(url, "/websocket/") == 0 && strcmp(method, "GET") == 0)
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
						send(server->connections[index].socket, buffer, bufferLength, 0);
						Server_CloseConnection(server, index);

						continue;
					}

					sprintf(websocketKeyUnhashed, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", websocketKeyHeader);

					bufferLength = sprintf(buffer, "HTTP/1.1 101 Switching Protocols\r\nDate: %s\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Accept: %s\r\n\r\n", Server_GetTime(), Crypto_Hash(websocketKeyUnhashed, strlen(websocketKeyUnhashed)));
					send(server->connections[index].socket, buffer, bufferLength, 0);

					server->connections[index].type = SERVER_CONNECTION_TYPE_WEBSOCKET;

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
						send(server->connections[index].socket, buffer, bufferLength, 0);

						if (fullResponse == 0)
						{
							send(server->connections[index].socket, server->staticFiles[file].buffer, server->staticFiles[file].bufferLength, 0);
						}

						Server_CloseConnection(server, index);

						break;
					}
				}

				if (server->connections[index].type == SERVER_CONNECTION_TYPE_UNCONNECTED)
				{
					continue;
				}

				bufferLength = sprintf(buffer, "HTTP/1.1 303 See Other\r\nDate: %s\r\nConnection: close\r\nLocation: /\r\n\r\n", Server_GetTime());
				send(server->connections[index].socket, buffer, bufferLength, 0);
				Server_CloseConnection(server, index);
			}
		}

		static int socket;
		socket = accept(serverSocket, NULL, NULL);

		if (socket == SERVER_INVALID_SOCKET)
		{
			continue;
		}

		if (server->maxConnections == server->connectionsCount)
		{
			static int* newConnectionIndexes;
			newConnectionIndexes = realloc(server->connectionsIndexes, 2 * server->maxConnections * sizeof(int));

			static Server_Connection* newConnections;
			newConnections = realloc(server->connections, 2 * server->maxConnections * sizeof(Server_Connection));

			if (newConnectionIndexes == NULL || newConnections == NULL)
			{
				ERROR("Failed to allocate memory!");
				return;
			}

			server->connectionsIndexes = newConnectionIndexes;
			server->connections = newConnections;

			static int connectionIndex;

			for (connectionIndex = server->maxConnections; connectionIndex < 2 * server->maxConnections; connectionIndex++)
			{
				server->connectionsIndexes[connectionIndex] = connectionIndex;
			}

			server->maxConnections *= 2;
		}

		static int connectionIndex;
		connectionIndex = server->connectionsIndexes[server->connectionsCount];
		server->connections[connectionIndex].type = SERVER_CONNECTION_TYPE_HTTP;
		server->connections[connectionIndex].lastPacket = time(0);
		server->connections[connectionIndex].socket = socket;
		server->connectionsCount++;
	}
}

void Server_SetStaticFilesMaxSize(Server* server, int size)
{
	server->staticFilesCount = 0;
	server->staticFiles = malloc(sizeof(Server_StaticFile) * size);
}

int Server_AddStaticFile(Server* server, const char* path, const char* url, const char* contentType)
{
	FILE* file = fopen(path, "rb");

	if (file == NULL)
	{
		ERROR("Failed to open a static file!");
		return 0;
	}

	fseek(file, 0L, SEEK_END);
	int length = ftell(file);

	if (length == -1)
	{
		ERROR("Failed to read a static file!");
		return 0;
	}

	rewind(file);

	server->staticFiles[server->staticFilesCount].buffer = malloc(length);

	if (server->staticFiles[server->staticFilesCount].buffer == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	if (fread(server->staticFiles[server->staticFilesCount].buffer, sizeof(char), length, file) != length)
	{
		ERROR("Failed to read a static file!");
		return 0;
	}

	fclose(file);

	server->staticFiles[server->staticFilesCount].path = path;
	server->staticFiles[server->staticFilesCount].url = url;
	server->staticFiles[server->staticFilesCount].contentType = contentType;
	server->staticFiles[server->staticFilesCount].bufferLength = length;
	server->staticFilesCount++;

	return 1;
}