#include "headers.h"
#include "crypto.h"
#include "handlers.h"
#include "configuration.h"

Server server;
Handlers handlers;
Configuration configuration;

void handleWebSocketConnection(int connectionIndex, char* cookies)
{
	Handlers_HandleWebSocketConnection(&handlers, &server, connectionIndex, cookies);
}

void handleWebSocketMessage(int connectionIndex, char* message, int messageLength)
{
	Handlers_HandleWebSocketMessage(&handlers, &server, connectionIndex, message, messageLength);
}

int main()
{
	if (!Handlers_Initialize(&handlers) || !Configuration_Load(&configuration, "AdvancedChess.configuration") || !Server_Initialize(&server) || !Server_AddStaticFile(&server, "static/index.html", "/", "text/html") || !Server_AddStaticFile(&server, "static/style.css", "/style.css", "text/css") || !Server_AddStaticFile(&server, "static/script.js", "/script.js", "text/javascript") || !Server_AddStaticFile(&server, "static/favicon.ico", "/favicon.ico", "image/x-icon"))
	{
		return 1;
	}


	server.websocketConnectionHandler = handleWebSocketConnection;
	server.websocketPacketHandler = handleWebSocketMessage;

	Server_Start(&server, Configuration_Read(&configuration, "SERVER_PORT"), Configuration_Read(&configuration, "SERVER_INTERNET_PROTOCOL_VERSION"));
	return 0;
}