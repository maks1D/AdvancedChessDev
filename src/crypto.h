#pragma once
#include "headers.h"

char Crypto_ToBase64(int number);
char Crypto_ToBase16(int number);
char* Crypto_Hash(unsigned char* message, int messageLength);
char* Crypto_RandomString();