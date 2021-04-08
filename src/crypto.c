#include "crypto.h"

char Crypto_ToBase64(int number)
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

char* Crypto_Hash(unsigned char* message, int messageLength)
{
	long long originalMessageLength = (long long)messageLength * 8;

	message[messageLength] = 0x80U;
	messageLength++;

	while (messageLength % 64 != 56)
	{
		message[messageLength] = 0;
		messageLength++;
	}

	for (int index = 0; index < 8; index++)
	{
		message[messageLength + index] = ((originalMessageLength >> ((7 - index) * 8)) & 0xff);
	}

	messageLength = (messageLength + 8) / 64;

	unsigned int results[5];
	results[0] = 0x67452301U;
	results[1] = 0xEFCDAB89U;
	results[2] = 0x98BADCFEU;
	results[3] = 0x10325476U;
	results[4] = 0xC3D2E1F0U;

	for (int chunk = 0; chunk < messageLength; chunk++)
	{
		unsigned int words[80];

		for (int index = 0; index < 16; index++)
		{
			words[index] = ((unsigned int)message[(chunk * 64) + (index * 4)] << 24) |
				((unsigned int)message[(chunk * 64) + (index * 4) + 1] << 16) |
				((unsigned int)message[(chunk * 64) + (index * 4) + 2] << 8) |
				(unsigned int)message[(chunk * 64) + (index * 4) + 3];

		}

		for (int index = 16; index < 80; index++)
		{
			words[index] = words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16];
			words[index] = (words[index] << 1) | (words[index] >> 31);
		}

		unsigned int numbers[6];
		numbers[0] = results[0];
		numbers[1] = results[1];
		numbers[2] = results[2];
		numbers[3] = results[3];
		numbers[4] = results[4];

		int value = 0x5A827999U;

		for (int index = 0; index < 80; index++)
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

			int number = ((numbers[0] << 5) | (numbers[0] >> 27)) + numbers[5] + numbers[4] + value + words[index];

			numbers[4] = numbers[3];
			numbers[3] = numbers[2];
			numbers[2] = (numbers[1] << 30) | (numbers[1] >> 2);
			numbers[1] = numbers[0];
			numbers[0] = number;
		}

		for (int index = 0; index < 5; index++)
		{
			results[index] += numbers[index];
		}
	}

	unsigned char output[20];

	for (int index = 0; index < 5; index++)
	{
		for (int byte = 0; byte < 4; byte++)
		{
			output[4 * index + byte] = (results[index] >> ((3 - byte) * 8)) & 0xff;
		}
	}

	static char encoded[29];

	for (int index = 0; index < 6; index++)
	{
		encoded[index * 4] = Crypto_ToBase64(output[index * 3] >> 2);
		encoded[index * 4 + 1] = Crypto_ToBase64(((output[index * 3] & 0b11) << 4) | (output[index * 3 + 1] >> 4));
		encoded[index * 4 + 2] = Crypto_ToBase64(((output[index * 3 + 1] & 0b1111) << 2) | (output[index * 3 + 2] >> 6));
		encoded[index * 4 + 3] = Crypto_ToBase64(output[index * 3 + 2] & 0b111111);
	}

	encoded[24] = Crypto_ToBase64(output[18] >> 2);
	encoded[25] = Crypto_ToBase64(((output[18] & 0b11) << 4) | (output[19] >> 4));
	encoded[26] = Crypto_ToBase64((output[19] & 0b1111) << 2);
	encoded[27] = '=';
	encoded[28] = '\x00';

	return encoded;
}

char* Crypto_RandomString()
{
	char* bytes = malloc(96);

	if (bytes == NULL)
	{
		return NULL;
	}

#ifdef WIN32
	if (FAILED(BCryptGenRandom(NULL, bytes, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
	{
		return NULL;
	}

#else
	if (getrandom(bytes, 32, 0) != 32)
	{
		return NULL;
	}
#endif

	char* result = Crypto_Hash(bytes, 32);

	free(bytes);
	return result;
}