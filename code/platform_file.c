#include "memory.h"
#include "platform_file.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t * platform_file_read(char const * path, size_t * out_size) {
	FILE * file = fopen(path, "rb");
	if (file == NULL) { fprintf(stderr, "'fopen' failed\n"); DEBUG_BREAK(); exit(1); }

	fseek(file, 0L, SEEK_END);
	size_t file_size = (size_t)ftell(file);
	rewind(file);

	uint8_t * buffer = MEMORY_ALLOCATE_ARRAY(uint8_t, file_size + 1);
	if (buffer == NULL) { fprintf(stderr, "'malloc' failed\n"); DEBUG_BREAK(); exit(1); }

	size_t bytes_read = (size_t)fread(buffer, sizeof(uint8_t), file_size, file);
	if (bytes_read < file_size) { fprintf(stderr, "'fread' failed\n"); DEBUG_BREAK(); exit(1); }

	fclose(file);

	*out_size = file_size;
	return buffer;
}
