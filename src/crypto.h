#pragma once
#ifdef WIN32
#include <Windows.h>

#pragma comment (lib, "Bcrypt.lib")
#else
#include <sys/random.h>
#endif

#include <stdlib.h>

char Crypto_ToBase64(int number);
char* Crypto_Hash(unsigned char* message, int messageLength);
char* Crypto_GenerateRandomBytes();