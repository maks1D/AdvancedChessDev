#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "filesystem.h"

#include <stdio.h>

#define CONFIGURATION_ENTRIES_LIMIT 2
#define CONFIGURATION_PARSING_MODE_INDEX_BEGIN 0
#define CONFIGURATION_PARSING_MODE_INDEX 1
#define CONFIGURATION_PARSING_MODE_VALUE_BEGIN 2
#define CONFIGURATION_PARSING_MODE_VALUE 3
#define CONFIGURATION_PARSING_MODE_COMMENT 4

typedef struct
{
	int entries;
	char* properties[CONFIGURATION_ENTRIES_LIMIT];
	char* values[CONFIGURATION_ENTRIES_LIMIT];
} Configuration;

char Configuration_Load(Configuration* configuration, const char* path);
char* Configuration_Read(Configuration* configuration, const char* property);