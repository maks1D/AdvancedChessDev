#include "handlers.h"

Database database;

int Handlers_Initialize()
{
	if (!Database_Open(&database, "AdvancedChess.database", 4))
	{
		return 0;
	}

	if (database.tables[HANDLERS_DATABASE_TABLE_INDEX_SECRET].numberOfEntries == 0)
	{
		if (!Database_AddEntry(&database, HANDLERS_DATABASE_TABLE_INDEX_SECRET, Crypto_RandomString()))
		{
			return 0;
		}
	}

	return 1;
}

void Handlers_HandleWebsocketConnection(Server* server, int connectionIndex, char* cookies)
{
	if (cookies != NULL)
	{
		int length = strlen(cookies);

		char* usernameCookie = NULL;
		char* expiresCookie = NULL;
		char* signatureCookie = NULL;

		char* cookieName = &cookies[0];
		char* cookieValue = NULL;

		int mode = HANDLERS_COOKIES_PARSING_MODE_NAME;

		for (int index = 0; index < length; index++)
		{
			if (mode == HANDLERS_COOKIES_PARSING_MODE_NAME)
			{
				if (cookies[index] == '=')
				{
					cookies[index] = '\x00';
					cookieValue = &cookies[index + 1];
					mode = HANDLERS_COOKIES_PARSING_MODE_VALUE;
				}
			}
			else if (index + 1 < length && cookies[index] == ';' && cookies[index + 1] == ' ')
			{
				cookies[index] = '\x00';

				if (strcmp(cookieName, "username") == 0)
				{
					usernameCookie = cookieValue;
				}
				else if (strcmp(cookieName, "expires") == 0)
				{
					expiresCookie = cookieValue;
				}
				else if (strcmp(cookieName, "signature") == 0)
				{
					signatureCookie = cookieValue;
				}

				mode = HANDLERS_COOKIES_PARSING_MODE_NAME;
				cookieName = &cookies[index + 2];
				index++;
			}
		}

		if (usernameCookie != NULL && expiresCookie != NULL && signatureCookie != NULL)
		{
			length = sprintf(cookies, "%s%s%s", usernameCookie, expiresCookie, database.tables[HANDLERS_DATABASE_TABLE_INDEX_SECRET].entries[0]);

			if (strcmp(Crypto_Hash(cookies, length), signatureCookie) == 0)
			{
				// TODO: assign uid
			}
		}
	}

	
}

int Handlers_HandleWebsocketPacket(Server* server, int connectionIndex, char* packet, int length)
{
	printf("%s\n", packet);
	return 1;
}