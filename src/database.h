#pragma once
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"

typedef struct
{
	int numberOfEntries;
	int maximumNumberOfEntries;
	char** entries;
} Database_Table;

typedef struct
{
	int maximumNumberOfTables;
	int numberOfTables;
	Database_Table* tables;
	FILE* file;
} Database;

int Database_Open(Database* database, char* path, int minimumNumberOfTables);
int Database_AddEntry(Database* database, int tableIndex, char* data);
int Database_ModifyEntry(Database* database, int tableIndex, int entryIndex, char* data);