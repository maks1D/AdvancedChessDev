#pragma once
#include <stdio.h>

#include "base64.h"
#include "endianness.h"

unsigned int Hasher_LeftRotate(unsigned int number, int bits);
char* Hasher_Hash(unsigned char* message, int messageLength);