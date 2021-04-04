#include "configuration.h"

char Configuration_Load(Configuration* configuration, const char* path)
{
	static char* buffer;

	static int length;
	length = Filesystem_ReadFile(path, &buffer, 1);

	if (length == -1)
	{
		printf("Failed to open the configuration file!\n");
		return 0;
	}

	static int begin;

	static int mode;
	mode = CONFIGURATION_PARSING_MODE_INDEX_BEGIN;

	configuration->entries = 0;

	static int index;

	for (index = 0; index < length; index++)
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

					if (configuration->properties[configuration->entries] == 0)
					{
						printf("Failed to open the configuration file!\n");
						return 0;
					}

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
	static int index;

	for (index = 0; index < configuration->entries; index++)
	{
		if (strcmp(property, configuration->properties[index]) == 0)
		{
			return configuration->values[index];
		}
	}

	return "";
}