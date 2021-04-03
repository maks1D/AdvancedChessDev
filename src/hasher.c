#include "hasher.h"

unsigned int Hasher_LeftRotate(unsigned int number, int bits)
{
	return (number << bits) | (number >> (32 - bits));
}

char* Hasher_Hash(unsigned char* message, int messageLength)
{
	unsigned long long originalMessageLength = Endianness_LongLongToBigEndian((unsigned long long)messageLength * 8);

	message[messageLength] = (unsigned char)0x80;
	messageLength++;

	while (messageLength % 64 != 56)
	{
		message[messageLength] = (unsigned char)0;
		messageLength++;
	}
	
	static int index;

	for (index = 0; index < 8; index++)
	{
		message[messageLength + index] = ((unsigned char*)&originalMessageLength)[index];
	}

	messageLength = (messageLength + 8) / 64;

	static unsigned int results[5];
	results[0] = 0x67452301U;
	results[1] = 0xEFCDAB89U;
	results[2] = 0x98BADCFEU;
	results[3] = 0x10325476U;
	results[4] = 0xC3D2E1F0U;

	static int chunk;

	for (chunk = 0; chunk < messageLength; chunk++)
	{
		static unsigned int words[80];

		for (index = 0; index < 16; index++)
		{
			words[index] = ((unsigned int)message[(chunk * 64) + (index * 4)] << 24) |
				((unsigned int)message[(chunk * 64) + (index * 4) + 1] << 16) |
				((unsigned int)message[(chunk * 64) + (index * 4) + 2] << 8) |
				(unsigned int)message[(chunk * 64) + (index * 4) + 3];

		}

		for (index = 16; index < 80; index++)
		{
			words[index] = Hasher_LeftRotate(words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16], 1);
		}

		static unsigned int numbers[6];
		numbers[0] = results[0];
		numbers[1] = results[1];
		numbers[2] = results[2];
		numbers[3] = results[3];
		numbers[4] = results[4];

		static int value;
		value = 0x5A827999U;

		for (index = 0; index < 80; index++)
		{
			if (index < 20)
			{
				numbers[5] = (numbers[1] & numbers[2]) | ((~numbers[1]) & numbers[3]);
			}
			else if (index < 40)
			{
				numbers[5] = numbers[1] ^ numbers[2] ^ numbers[3];
			}
			else if (index < 60)
			{
				numbers[5] = (numbers[1] & numbers[2]) | (numbers[1] & numbers[3]) | (numbers[2] & numbers[3]);
			}
			else
			{
				numbers[5] = numbers[1] ^ numbers[2] ^ numbers[3];
			}

			if (index == 20)
			{
				value = 0x6ED9EBA1U;
			}
			else if (index == 40)
			{
				value = 0x8F1BBCDCU;
			}
			else if (index == 60)
			{
				value = 0xCA62C1D6U;
			}
			
			static int number;
			number = Hasher_LeftRotate(numbers[0], 5) + numbers[5] + numbers[4] + value + words[index];

			numbers[4] = numbers[3];
			numbers[3] = numbers[2];
			numbers[2] = Hasher_LeftRotate(numbers[1], 30);
			numbers[1] = numbers[0];
			numbers[0] = number;
		}

		for (index = 0; index < 5; index++)
		{
			results[index] += numbers[index];
		}
	}

	for (index = 0; index < 5; index++)
	{
		results[index] = Endianness_IntToBigEndian(results[index]);
	}

	static unsigned char output[20];

	for (index = 0; index < 20; index++)
	{
		output[index] = ((unsigned char*)&results)[index];
	}

	static char encoded[29];
	Base64_StringToBase64(output, 20, encoded);

	return encoded;
}