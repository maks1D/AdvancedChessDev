#include "configuration.h"

int Configuration_Load(Configuration* configuration, char* path)
{
	FILE* file = fopen(path, "rb");

	if (file == NULL)
	{
		ERROR("Failed to open the configuration file!");
		return 0;
	}

	configuration->entries = 0;
	configuration->maxEntries = 1;
	configuration->properties = malloc(sizeof(char*));
	configuration->values = malloc(sizeof(char*));

	if (configuration->properties == NULL || configuration->values == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	fseek(file, 0L, SEEK_END);
	int length = ftell(file);

	if (length == -1)
	{
		ERROR("Failed to read the configuration file!");
		return 0;
	}

	rewind(file);

	char* buffer = malloc(length);

	if (buffer == NULL)
	{
		ERROR("Failed to allocate memory!");
		return 0;
	}

	if (fread(buffer, sizeof(char), length, file) != length)
	{
		ERROR("Failed to read the configuration file!");
		return 0;
	}

	fclose(file);

	int begin;
	int mode = CONFIGURATION_PARSING_MODE_INDEX_BEGIN;

	for(int index = 0; index < length; index++)
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
					ERROR("The configuration file has invalid data!");
					return 0;
				}

				break;
			}

			case CONFIGURATION_PARSING_MODE_INDEX:
			{
				if (buffer[index] == '=')
				{
					buffer[index] = '\x00';

					if (configuration->maxEntries == configuration->entries)
					{
						char** newProperties = realloc(configuration->properties, 2 * configuration->entries * sizeof(char*));
						char** newValues = realloc(configuration->values, 2 * configuration->entries * sizeof(char*));

						if (newProperties == NULL || newValues == NULL)
						{
							ERROR("Failed to allocate memory!");
							return 0;
						}

						configuration->properties = newProperties;
						configuration->values = newValues;
					}

					configuration->properties[configuration->entries] = malloc(index - begin + 1);

					if (configuration->properties[configuration->entries] == NULL)
					{
						ERROR("Failed to open the configuration file!");
						return 0;
					}

					sprintf(configuration->properties[configuration->entries], "%s", buffer + begin);

					begin = index + 1;
					mode = CONFIGURATION_PARSING_MODE_VALUE_BEGIN;
				}
				else if(!(('A' <= buffer[index] && buffer[index] <= 'Z') || ('0' <= buffer[index] && buffer[index] <= '9') || buffer[index] == '_'))
				{
					ERROR("The configuration file has invalid data!");
					return 0;
				}

				break;
			}

			case CONFIGURATION_PARSING_MODE_VALUE_BEGIN:
			{
				if (!(('A' <= buffer[index] && buffer[index] <= 'Z') || ('0' <= buffer[index] && buffer[index] <= '9') || buffer[index] == '_'))
				{
					ERROR("The configuration file has invalid data!");
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
					ERROR("The configuration file has invalid data!");
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
	}

	if (mode == CONFIGURATION_PARSING_MODE_INDEX || mode == CONFIGURATION_PARSING_MODE_VALUE_BEGIN)
	{
		ERROR("The configuration file has invalid data!");
		return 0;
	}

	free(buffer);
	return 1;
}

char* Configuration_Read(Configuration* configuration, char* property)
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