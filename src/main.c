#include "handlers.h"
#include "server.h"
#include "configuration.h"


int main()
{
	Configuration configuration;
	Server server;

	if (!Handlers_Initialize() ||
		!Configuration_Load(&configuration, "AdvancedChess.configuration") || 
		!Server_SetStaticFilesMaxSize(&server, 4) ||
		!Server_AddStaticFile(&server, "static/index.html", "/", "text/html") ||
		!Server_AddStaticFile(&server, "static/style.css", "/style.css", "text/css") ||
		!Server_AddStaticFile(&server, "static/script.js", "/script.js", "text/javascript") ||
		!Server_AddStaticFile(&server, "static/favicon.ico", "/favicon.ico", "image/vnd.microsoft.icon")
	)
	{
		return 1;
	}

	
	server.websocketPacketHandler = Handlers_HandleWebsocketPacket;
	server.websocketConnectionHandler = Handlers_HandleWebsocketConnection;

	Server_Start(&server, Configuration_Read(&configuration, "SERVER_PORT"), Configuration_Read(&configuration, "SERVER_INTERNET_PROTOCOL_VERSION"));
	return 0;
}