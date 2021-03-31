#include "database.h"
#include "captcha.h"
#include "server.h"
#include "chess.h"
#include "configuration.h"

int main()
{
	Configuration configuration;

	if (!Configuration_Load(&configuration, "settings.txt"))
	{
		printf("Aborting.\n");
		return 1;
	}
	
	Server server;
	Server_SetStaticFilesMaxSize(&server, 4);

	if (!Server_AddStaticFile(&server, "static/index.html", "/", "text/html") ||
		!Server_AddStaticFile(&server, "static/style.css", "/style.css", "text/css") ||
		!Server_AddStaticFile(&server, "static/script.js", "/script.js", "text/javascript") ||
		!Server_AddStaticFile(&server, "static/favicon.ico", "/favicon.ico", "image/vnd.microsoft.icon"))
	{

		printf("Aborting.\n");
		return 1;
	}

	Server_Start(&server, Configuration_Read(&configuration, "SERVER_PORT"), Configuration_Read(&configuration, "SERVER_INTERNET_PROTOCOL_VERSION"));
}