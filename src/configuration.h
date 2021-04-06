#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#define CONFIGURATION_PARSING_MODE_INDEX_BEGIN 0
#define CONFIGURATION_PARSING_MODE_INDEX 1
#define CONFIGURATION_PARSING_MODE_VALUE_BEGIN 2
#define CONFIGURATION_PARSING_MODE_VALUE 3
#define CONFIGURATION_PARSING_MODE_COMMENT 4

typedef struct
{
	int entries;
	int maxEntries;
	char** properties;
	char** values;
} Configuration;

int Configuration_Load(Configuration* configuration, const char* path);
char* Configuration_Read(Configuration* configuration, const char* property);