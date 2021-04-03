#include "endianness.h"

char Endianness_IsLittleEndian()
{
	static unsigned int number = 1;
	return *((char*)&number);
}

unsigned int Endianness_IntFromBigEndian(unsigned int number)
{
	return Endianness_IntToBigEndian(number);
}

unsigned int Endianness_IntToBigEndian(unsigned int number)
{
	if (Endianness_IsLittleEndian())
	{
		number = ((number << 8) & 0xFF00FF00) | ((number >> 8) & 0xFF00FF);
		return (number << 16) | (number >> 16);
	}

	return number;
}

unsigned long long Endianness_LongLongToBigEndian(unsigned long long number)
{
	if (Endianness_IsLittleEndian())
	{
		number = ((number << 8) & 0xFF00FF00FF00FF00ULL) | ((number >> 8) & 0x00FF00FF00FF00FFULL);
		number = ((number << 16) & 0xFFFF0000FFFF0000ULL) | ((number >> 16) & 0x0000FFFF0000FFFFULL);
		number = (number << 32) | (number >> 32);
	}

	return number;
}