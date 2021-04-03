#include "configuration.h"

char Configuration_Load(Configuration* configuration, const char* path)
{
#ifdef WIN32
	HANDLE handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open the configuration file!\n");
		return 0;
	}

	LARGE_INTEGER size;
	GetFileSizeEx(handle, &size);

	int length = (int)size.QuadPart;
	char* buffer = malloc(length + 1);

	if (buffer == NULL || !ReadFile(handle, buffer, length, NULL, NULL))
	{
		printf("Failed read the configuration file!\n");
		return 0;
	}

	buffer[length] = '\x00';

	CloseHandle(handle);
#else
	int handle = open(path, O_RDONLY);

	if (handle == -1)
	{
		printf("Failed to open the configuration file!\n");
		return 0;
	}

	struct stat data;
	fstat(handle, &data);

	int length = data.st_size;
	char* buffer = malloc(length + 1);

	if (buffer == NULL || read(handle, buffer, length) == -1)
	{
		printf("Failed read the configuration file!\n");
		return 0;
	}

	buffer[length] = '\x00';

	close(handle);
#endif

	int begin;
	int mode = CONFIGURATION_PARSING_MODE_INDEX_BEGIN;

	configuration->entries = 0;

	for (int index = 0; index < length; index++)
	{
		switch (mode)
		{
			case CONFIGURATION_PARSING_MODE_INDEX_BEGIN:
			{
				if (('A' <= buffer[index] && buffer[index] <= 'Z') || ('0' <= buffer[index] && buffer[index] <= '9') || buffer[index] == '_')
				{
					begin = index;
					mode = CONFIGURATION_PARSING_MODE_INDEX;
				}
				else if (buffer[index] == '#')
				{
					mode = CONFIGURATION_PARSING_MODE_COMMENT;
				}
				else if(buffer[index] != ' ' && buffer[index] != '\r' && buffer[index] != '\n')
				{
					printf("An error occurred while parsing the configuraiton file!\n");
					return 0;
				}

				break;
			}

			case CONFIGURATION_PARSING_MODE_INDEX:
			{
				if (buffer[index] == '=')
				{
					buffer[index] = '\x00';

					configuration->properties[configuration->entries] = malloc(index - begin + 1);
					sprintf(configuration->properties[configuration->entries], "%s", buffer + begin);

					begin = index + 1;
					mode = CONFIGURATION_PARSING_MODE_VALUE_BEGIN;
				}
				else if(!(('A' <= buffer[index] && buffer[index] <= 'Z') || ('0' <= buffer[index] && buffer[index] <= '9') || buffer[index] == '_'))
				{
					printf("An error occurred while parsing the configuraiton file!\n");
					return 0;
				}

				break;
			}

			case CONFIGURATION_PARSING_MODE_VALUE_BEGIN:
			{
				if (!(('A' <= buffer[index] && buffer[index] <= 'Z') || ('0' <= buffer[index] && buffer[index] <= '9') || buffer[index] == '_'))
				{
					printf("An error occurred while parsing the configuraiton file!\n");
					return 0;
				}


				mode = CONFIGURATION_PARSING_MODE_VALUE;

				break;
			}

			case CONFIGURATION_PARSING_MODE_VALUE:
			{
				if (buffer[index] == ' ' || buffer[index] == '\r' || buffer[index] == '\n')
				{
					buffer[index] = '\x00';

					configuration->values[configuration->entries] = malloc(index - begin + 1);
					sprintf(configuration->values[configuration->entries], "%s", buffer + begin);

					configuration->entries++;

					mode = CONFIGURATION_PARSING_MODE_INDEX_BEGIN;
				}
				else if (buffer[index] == '#')
				{
					buffer[index] = '\x00';

					configuration->values[configuration->entries] = malloc(index - begin + 1);
					sprintf(configuration->values[configuration->entries], "%s", buffer + begin);

					configuration->entries++;

					mode = CONFIGURATION_PARSING_MODE_COMMENT;
				}
				else if (!(('A' <= buffer[index] && buffer[index] <= 'Z') || ('0' <= buffer[index] && buffer[index] <= '9') || buffer[index] == '_'))
				{
					printf("An error occurred while parsing the configuraiton file!\n");
					return 0;
				}

				break;
			}

			case CONFIGURATION_PARSING_MODE_COMMENT:
			{
				if (buffer[index] == '\n')
				{
					mode = CONFIGURATION_PARSING_MODE_INDEX_BEGIN;
				}

				break;
			}
		}
	}

	if (mode == CONFIGURATION_PARSING_MODE_VALUE)
	{
		configuration->values[configuration->entries] = malloc(length - begin + 1);
		sprintf(configuration->values[configuration->entries], "%s", buffer + begin);

		configuration->entries++;

		mode = CONFIGURATION_PARSING_MODE_INDEX_BEGIN;
	}

	if (mode == CONFIGURATION_PARSING_MODE_INDEX || mode == CONFIGURATION_PARSING_MODE_VALUE_BEGIN)
	{
		printf("An error occurred while parsing the configuraiton file!\n");
		return 0;
	}

	free(buffer);
	return 1;
}

char* Configuration_Read(Configuration* configuration, const char* property)
{
	for (int index = 0; index < configuration->entries; index++)
	{
		if (strcmp(property, configuration->properties[index]) == 0)
		{
			return configuration->values[index];
		}
	}

	return "";
}