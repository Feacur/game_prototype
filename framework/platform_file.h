#if !defined(GAME_PLATFORM_FILE)
#define GAME_PLATFORM_FILE

#include "common.h"

struct Array_Byte;
struct File;

enum File_Mode {
	FILE_MODE_NONE   = 0,
	FILE_MODE_READ   = (1 << 0),
	FILE_MODE_WRITE  = (1 << 1),
	FILE_MODE_DELETE = (1 << 2),
};

bool platform_file_read_entire(char const * path, struct Array_Byte * buffer);
void platform_file_delete(char const * path);

struct File * platform_file_init(char const * path, enum File_Mode mode);
void platform_file_free(struct File * file);

uint64_t platform_file_size(struct File * file);
uint64_t platform_file_time(struct File * file);

uint64_t platform_file_position_get(struct File * file);
uint64_t platform_file_position_set(struct File * file, uint64_t position);

uint64_t platform_file_read(struct File * file, uint8_t * buffer, uint64_t size);
uint64_t platform_file_write(struct File * file, uint8_t * buffer, uint64_t size);

#endif
