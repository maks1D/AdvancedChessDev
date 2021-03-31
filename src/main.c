#include "database.h"
#include "captcha.h"
#include "server.h"
#include "chess.h"

int main()
{
	Server server;
	Server_SetStaticFilesMaxSize(&server, 4);
	Server_AddStaticFile(&server, "static/index.html", "/", "text/html");
	Server_AddStaticFile(&server, "static/style.css", "/style.css", "text/css");
	Server_AddStaticFile(&server, "static/script.js", "/script.js", "text/javascript");
	Server_AddStaticFile(&server, "static/favicon.ico", "/favicon.ico", "image/vnd.microsoft.icon");
	Server_Start(&server, "80", "4", 5);
}