#pragma once

char Endianness_IsLittleEndian();
unsigned int Endianness_IntFromBigEndian(unsigned int number);
unsigned int Endianness_IntToBigEndian(unsigned int number);
unsigned long long Endianness_LongLongToBigEndian(unsigned long long number);