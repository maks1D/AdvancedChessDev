#include "base64.h"

char Base64_NumberToBase64(int number)
{
	if (number <= 25)
	{
		return 'A' + number;
	}

	if (number <= 51)
	{
		return 'a' + number - 26;
	}

	if (number <= 61)
	{
		return '0' + number - 52;
	}

	if (number == 62)
	{
		return '+';
	}

	return '/';
}

void Base64_StringToBase64(unsigned char* buffer, int length, char* output)
{
	static int full;
	full = length / 3;

	static int index;
	
	for (index = 0; index < full; index++)
	{
		output[index * 4] = Base64_NumberToBase64(buffer[index * 3] >> 2);
		output[index * 4 + 1] = Base64_NumberToBase64(((buffer[index * 3] & 0b11) << 4) | (buffer[index * 3 + 1] >> 4));
		output[index * 4 + 2] = Base64_NumberToBase64(((buffer[index * 3 + 1] & 0b1111) << 2) | (buffer[index * 3 + 2] >> 6));
		output[index * 4 + 3] = Base64_NumberToBase64(buffer[index * 3 + 2] & 0b111111);
	}

	if (length - (full * 3) == 1)
	{
		output[index * 4] = Base64_NumberToBase64(buffer[index * 3] >> 2);
		output[index * 4 + 1] = Base64_NumberToBase64((buffer[index * 3] & 0b11) << 4);
		output[index * 4 + 2] = '=';
		output[index * 4 + 3] = '=';
		output[index * 4 + 4] = '\x00';
	}
	else if (length - (full * 3) == 2)
	{
		output[index * 4] = Base64_NumberToBase64(buffer[index * 3] >> 2);
		output[index * 4 + 1] = Base64_NumberToBase64(((buffer[index * 3] & 0b11) << 4) | (buffer[index * 3 + 1] >> 4));
		output[index * 4 + 2] = Base64_NumberToBase64((buffer[index * 3 + 1] & 0b1111) << 2);
		output[index * 4 + 3] = '=';
		output[index * 4 + 4] = '\x00';
	}
	else
	{
		output[index * 4] = '\x00';
	}
}