#ifndef NOR_ART_PATH_H
#define NOR_ART_PATH_H

#include <stddef.h>
#include <string.h>

static inline void NOR_StoreArtPathRecord(char *record, size_t capacity,
	const char *path)
{
	size_t length;

	if(!record || capacity == 0)
		return;
	memset(record, 0, capacity);
	if(!path)
		return;
	length = strlen(path);
	if(length < capacity)
		memcpy(record, path, length + 1);
}

static inline const char *NOR_ValidArtPathRecord(const char *record,
	size_t capacity)
{
	if(!record || capacity == 0 || (unsigned char)record[0] == 0xFF)
		return "";
	return memchr(record, '\0', capacity) ? record : "";
}

#endif
