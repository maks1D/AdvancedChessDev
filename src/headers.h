#pragma once

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef WIN32
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Bcrypt.lib")
#else
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef WIN32
#pragma warning (disable: 6385 6386)
#endif

#ifdef WIN32
#define ERROR_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define ERROR_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef ERROR
#undef ERROR
#endif

#define ERROR(message) printf("[%s:%d] %s\n", ERROR_FILENAME, __LINE__, message)

#ifdef min
#undef min
#endif

#define min(first, second) ((first) > (second) ? (first) : (second))