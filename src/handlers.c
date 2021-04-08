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
	if (cookies == NULL)
	{
		return;
	}

	int length = strlen(cookies);
	char* usernameCookie = NULL;
	char* expiresCookie = NULL;
	char* signatureCookie = NULL;
	char* cookieName = &cookies[0];

	int mode = HANDLERS_COOKIES_PARSING_MODE_NAME_BEGIN;

	for (int index = 0; index < length; index++)
	{
		if (mode == HANDLERS_COOKIES_PARSING_MODE_NAME_BEGIN || mode == HANDLERS_COOKIES_PARSING_MODE_NAME)
		{
			if (cookies[index] == '=')
			{
				cookies[index] = '\x00';

				if (strcmp(cookieName, "username") == 0)
				{
					usernameCookie = &cookies[index + 1];
				}
				else if (strcmp(cookieName, "expires") == 0)
				{
					expiresCookie = &cookies[index + 1];
				}
				else if (strcmp(cookieName, "signature") == 0)
				{
					signatureCookie = &cookies[index + 1];
				}

				mode = HANDLERS_COOKIES_PARSING_MODE_VALUE;
			}
			else
			{
				mode = HANDLERS_COOKIES_PARSING_MODE_NAME;
			}
		}
		else if(index + 1 < length && cookies[index] == ';' && cookies[index + 1] == ' ')
		{
			mode = HANDLERS_COOKIES_PARSING_MODE_NAME_BEGIN;
			cookieName = &cookies[index + 2];
			cookies[index] = '\x00';
			index++;
		}
	}

	if (mode == HANDLERS_COOKIES_PARSING_MODE_NAME || usernameCookie == NULL || expiresCookie == NULL || signatureCookie == NULL)
	{
		return;
	}

	length = sprintf(cookies, "%s%s%s", usernameCookie, expiresCookie, database.tables[HANDLERS_DATABASE_TABLE_INDEX_SECRET].entries[0]);

	if (strcmp(Crypto_Hash(cookies, length), signatureCookie) != 0)
	{
		return;
	}
}

int Handlers_HandleWebsocketPacket(Server* server, int connectionIndex, char* packet, int length)
{
	printf("new packet\n");
	return 1;
}