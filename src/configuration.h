#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdio.h>

#define CONFIGURATION_ENTRIES_LIMIT 8
#define CONFIGURATION_MODE_INDEX_BEGIN 0
#define CONFIGURATION_MODE_INDEX 1
#define CONFIGURATION_MODE_VALUE_BEGIN 2
#define CONFIGURATION_MODE_VALUE 3
#define CONFIGURATION_MODE_COMMENT 4

typedef struct
{
	int entries;
	char* properties[CONFIGURATION_ENTRIES_LIMIT];
	char* values[CONFIGURATION_ENTRIES_LIMIT];
} Configuration;

char Configuration_Load(Configuration* configuration, const char* path);
char* Configuration_Read(Configuration* configuration, const char* property);