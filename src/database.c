#include "database.h"

#ifdef WIN32
#pragma warning (disable: 6385 6386)
#endif

int Database_Open(Database* database, char* path)
{
	database->numberOfTables = 0;
	database->maximumNumberOfTables = 1;
	database->tables = malloc(sizeof(Database_Table));

	if (database->tables == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	database->tables[0].numberOfEntries = 0;
	database->tables[0].maximumNumberOfEntries = 1;
	database->tables[0].entries = malloc(sizeof(char*));

	if (database->tables[0].entries == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	database->tables[0].entries[0] = NULL;

	database->file = fopen(path, "rb+");

	if (database->file == NULL)
	{
		database->file = fopen(path, "w");

		if (database->file == NULL)
		{
			ERROR("Failed to open the database!");
			return 0;
		}

		return 1;
	}

	fseek(database->file, 0L, SEEK_END);
	int length = ftell(database->file);

	if (length == -1)
	{
		ERROR("Failed to read the database!");
		return 0;
	}

	rewind(database->file);

	char* buffer = malloc(length);

	if (buffer == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	if (fread(buffer, sizeof(char), length, database->file) != length)
	{
		ERROR("Failed to read the database!");
		return 0;
	}

	database->file = freopen(path, "w", database->file);

	if (database->file == NULL)
	{
		ERROR("Failed to open the database!");
		return 0;
	}

	if (length != 0 && buffer[length - 1] != '\x00')
	{
		ERROR("The database has invalid data!");
		return 0;
	}

	for (int index = 0; index < length; index++)
	{
		if (length - index <= 8)
		{
			ERROR("The database has invalid data!");
			return 0;
		}

		int tableIndex = (buffer[index] << 24) | (buffer[index + 1] << 16) | (buffer[index + 2] << 8) | buffer[index + 3];

		if (tableIndex >= database->maximumNumberOfTables)
		{
			int newNumberOfTables = max(2 * database->maximumNumberOfTables, tableIndex + 1);
			Database_Table* newTables = realloc(database->tables, newNumberOfTables * sizeof(Database_Table));

			if (newTables == NULL)
			{
				ERROR("Failed to allocate memory!");
				return 0;
			}

			for (int newTablesIndex = database->maximumNumberOfTables; newTablesIndex < newNumberOfTables; newTablesIndex++)
			{
				newTables[newTablesIndex].numberOfEntries = 0;
				newTables[newTablesIndex].maximumNumberOfEntries = 1;
				newTables[newTablesIndex].entries = malloc(sizeof(char*));
				newTables[newTablesIndex].entries[0] = NULL;
			}

			database->tables = newTables;
			database->maximumNumberOfTables = newNumberOfTables;
		}

		int entryIndex = (buffer[index + 4] << 24) | (buffer[index + 5] << 16) | (buffer[index + 6] << 8) | buffer[index + 7];

		if (entryIndex >= database->tables[tableIndex].maximumNumberOfEntries)
		{
			int newNumberOfEntries = max(2 * database->tables[tableIndex].maximumNumberOfEntries, entryIndex + 1);
			char** newEntries = realloc(database->tables[tableIndex].entries, newNumberOfEntries * sizeof(char**));

			if (newEntries == NULL)
			{
				ERROR("Failed to allocate memory!");
				return 0;
			}

			for (int newEntriesIndex = database->tables[tableIndex].maximumNumberOfEntries; newEntriesIndex < newNumberOfEntries; newEntriesIndex++)
			{
				newEntries[newEntriesIndex] = NULL;
			}

			database->tables[tableIndex].entries = newEntries;
			database->tables[tableIndex].maximumNumberOfEntries = newNumberOfEntries;
		}

		database->numberOfTables = max(database->numberOfTables, tableIndex + 1);
		database->tables[tableIndex].numberOfEntries = max(database->tables[tableIndex].numberOfEntries, entryIndex + 1);

		if (database->tables[tableIndex].entries[entryIndex] != NULL)
		{
			free(database->tables[tableIndex].entries[entryIndex]);
		}

		int length = strlen(&buffer[index + 8]);
		database->tables[tableIndex].entries[entryIndex] = malloc(length + 1);

		if (database->tables[tableIndex].entries[entryIndex] == NULL)
		{
			ERROR("Failed to allocate memory!");
			return 0;
		}

		sprintf(database->tables[tableIndex].entries[entryIndex], "%s", &buffer[index + 8]);

		index += 8 + length;
	}
	
	length = 0;

	for (int tableIndex = 0; tableIndex < database->numberOfTables; tableIndex++)
	{
		for (int entryIndex = 0; entryIndex < database->tables[tableIndex].numberOfEntries; entryIndex++)
		{
			if (database->tables[tableIndex].entries[entryIndex] == NULL)
			{
				ERROR("The database has invalid data!");
				return 0;
			}

			length += 9 + strlen(database->tables[tableIndex].entries[entryIndex]);
		}
	}

	free(buffer);
	buffer = malloc(length);

	if (buffer == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	int index = 0;

	for (int tableIndex = 0; tableIndex < database->numberOfTables; tableIndex++)
	{
		for (int entryIndex = 0; entryIndex < database->tables[tableIndex].numberOfEntries; entryIndex++)
		{
			buffer[index] = tableIndex >> 24;
			buffer[index + 1] = (tableIndex >> 16) & 0xff;
			buffer[index + 2] = (tableIndex >> 8) & 0xff;
			buffer[index + 3] = tableIndex & 0xff;

			buffer[index + 4] = entryIndex >> 24;
			buffer[index + 5] = (entryIndex >> 16) & 0xff;
			buffer[index + 6] = (entryIndex >> 8) & 0xff;
			buffer[index + 7] = entryIndex & 0xff;

			sprintf(&buffer[index + 8], "%s", database->tables[tableIndex].entries[entryIndex]);
			index += 9 + strlen(database->tables[tableIndex].entries[entryIndex]);
		}
	}

	if (fwrite(buffer, sizeof(char), length, database->file) != length)
	{
		ERROR("Failed to save the database!");
		return 0;
	}

	free(buffer);
	fflush(database->file);

	return 1;
}

int Database_AddEntry(Database* database, int tableIndex, char* data)
{
	if (tableIndex >= database->maximumNumberOfTables)
	{
		int newNumberOfTables = max(2 * database->maximumNumberOfTables, tableIndex + 1);
		Database_Table* newTables = realloc(database->tables, newNumberOfTables * sizeof(Database_Table));

		if (newTables == NULL)
		{
			ERROR("Failed to allocate memory!");
			return 0;
		}

		for (int newTablesIndex = database->maximumNumberOfTables; newTablesIndex < newNumberOfTables; newTablesIndex++)
		{
			newTables[newTablesIndex].numberOfEntries = 0;
			newTables[newTablesIndex].maximumNumberOfEntries = 1;
			newTables[newTablesIndex].entries = malloc(1);
			newTables[newTablesIndex].entries[0] = NULL;
		}

		database->tables = newTables;
		database->maximumNumberOfTables = newNumberOfTables;
	}

	int entryIndex = database->tables[tableIndex].numberOfEntries;

	if (entryIndex >= database->tables[tableIndex].maximumNumberOfEntries)
	{
		int newNumberOfEntries = max(2 * database->tables[tableIndex].maximumNumberOfEntries, entryIndex + 1);
		char** newEntries = realloc(database->tables[tableIndex].entries, newNumberOfEntries * sizeof(char**));

		if (newEntries == NULL)
		{
			ERROR("Failed to allocate memory!");
			return 0;
		}

		for (int newEntriesIndex = database->tables[tableIndex].maximumNumberOfEntries; newEntriesIndex < newNumberOfEntries; newEntriesIndex++)
		{
			newEntries[newEntriesIndex] = NULL;
		}

		database->tables[tableIndex].entries = newEntries;
		database->tables[tableIndex].maximumNumberOfEntries = newNumberOfEntries;
	}

	int length = strlen(data);
	database->tables[tableIndex].entries[entryIndex] = malloc(length + 1);

	if (database->tables[tableIndex].entries[entryIndex] == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	sprintf(database->tables[tableIndex].entries[entryIndex], "%s", data);

	database->numberOfTables = max(database->numberOfTables, tableIndex + 1);
	database->tables[tableIndex].numberOfEntries = max(database->tables[tableIndex].numberOfEntries, entryIndex + 1);

	char* buffer = malloc(length + 9);

	if (buffer == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	buffer[0] = tableIndex >> 24;
	buffer[1] = (tableIndex >> 16) & 0xff;
	buffer[2] = (tableIndex >> 8) & 0xff;
	buffer[3] = tableIndex & 0xff;

	buffer[4] = entryIndex >> 24;
	buffer[5] = (entryIndex >> 16) & 0xff;
	buffer[6] = (entryIndex >> 8) & 0xff;
	buffer[7] = entryIndex & 0xff;

	sprintf(&buffer[8], "%s", data);

	if (fwrite(buffer, sizeof(char), length + 9, database->file) != length + 9)
	{
		ERROR("Failed to save the database!");
		return 0;
	}

	fflush(database->file);
	free(buffer);

	return 1;
}

int Database_ModifyEntry(Database* database, int tableIndex, int entryIndex, char* data)
{
	free(database->tables[tableIndex].entries[entryIndex]);

	int length = strlen(data);
	database->tables[tableIndex].entries[entryIndex] = malloc(length + 1);

	if (database->tables[tableIndex].entries[entryIndex] == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	sprintf(database->tables[tableIndex].entries[entryIndex], "%s", data);

	char* buffer = malloc(length + 9);

	if (buffer == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	buffer[0] = tableIndex >> 24;
	buffer[1] = (tableIndex >> 16) & 0xff;
	buffer[2] = (tableIndex >> 8) & 0xff;
	buffer[3] = tableIndex & 0xff;

	buffer[4] = entryIndex >> 24;
	buffer[5] = (entryIndex >> 16) & 0xff;
	buffer[6] = (entryIndex >> 8) & 0xff;
	buffer[7] = entryIndex & 0xff;

	sprintf(&buffer[8], "%s", data);

	if (fwrite(buffer, sizeof(char), length + 9, database->file) != length + 9)
	{
		ERROR("Failed to save the database!");
	}

	fflush(database->file);
	free(buffer);

	return 1;
}