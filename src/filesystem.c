#include "filesystem.h"

int Filesystem_ReadFile(const char* path, unsigned char** buffer, char allocateMemory)
{
#ifdef WIN32
	static HANDLE handle;
	handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	static LARGE_INTEGER size;
	GetFileSizeEx(handle, &size);

	if (allocateMemory)
	{
		*buffer = malloc((size_t)size.QuadPart + 1);
	}

	if (*buffer == NULL || !ReadFile(handle, *buffer, (unsigned int)size.QuadPart, NULL, NULL))
	{
		CloseHandle(handle);
		return -1;
	}

	(*buffer)[size.QuadPart] = '\x00';
	CloseHandle(handle);

	return (int)size.QuadPart;
#else
	static int handle;
	handle = open(path, O_RDONLY);

	if (handle == -1)
	{
		return -1;
	}

	static struct stat data;
	fstat(handle, &data);

	if (allocateMemory)
	{
		*buffer = malloc(data.st_size + 1);
	}

	if (*buffer == NULL || read(handle, *buffer, data.st_size) == -1)
	{
		printf("Failed read the configuration file!\n");
		return -1;
	}

	(*buffer)[data.st_size] = '\x00';
	close(handle);

	return data.st_size;
#endif
}