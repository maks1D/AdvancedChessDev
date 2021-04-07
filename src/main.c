#include "captcha.h"
#include "server.h"
#include "chess.h"
#include "configuration.h"
#include "database.h"
#include "crypto.h"

#include "database.h"

int main()
{
	Database database;
	
	if (!Database_Open(&database, "AdvancedChess.database"))
	{
		return 1;
	}

	Configuration configuration;

	if (!Configuration_Load(&configuration, "AdvancedChess.configuration"))
	{
		return 1;
	}
	
	Server server;

	if (!Server_SetStaticFilesMaxSize(&server, 4))
	{
		return 1;
	}
	
	if (!Server_AddStaticFile(&server, "static/index.html", "/", "text/html") ||
		!Server_AddStaticFile(&server, "static/style.css", "/style.css", "text/css") ||
		!Server_AddStaticFile(&server, "static/script.js", "/script.js", "text/javascript") ||
		!Server_AddStaticFile(&server, "static/favicon.ico", "/favicon.ico", "image/vnd.microsoft.icon"))
	{

		return 1;
	}

	Server_Start(&server, Configuration_Read(&configuration, "SERVER_PORT"), Configuration_Read(&configuration, "SERVER_INTERNET_PROTOCOL_VERSION"));
	return 0;
}