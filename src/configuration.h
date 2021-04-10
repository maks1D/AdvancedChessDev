#pragma once
#include "headers.h"

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

int Configuration_Load(Configuration* configuration, char* path);
char* Configuration_Read(Configuration* configuration, char* property);