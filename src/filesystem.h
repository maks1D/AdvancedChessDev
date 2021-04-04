#pragma once

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#endif

int Filesystem_ReadFile(const char* path, char** buffer, char allocateMemory);