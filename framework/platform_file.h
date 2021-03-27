#if !defined(GAME_PLATFORM_FILE)
#define GAME_PLATFORM_FILE

#include "common.h"

struct Array_Byte;
struct File;

bool platform_file_read_entire(char const * path, struct Array_Byte * buffer);

struct File * platform_file_init(char const * path);
void platform_file_free(struct File * file);

uint64_t platform_file_size(struct File * file);
uint64_t platform_file_time(struct File * file);

uint64_t platform_file_position_get(struct File * file);
uint64_t platform_file_position_set(struct File * file, uint64_t position);

uint64_t platform_file_read(struct File * file, uint8_t * buffer, uint64_t size);
uint64_t platform_file_write(struct File * file, uint8_t * buffer, uint64_t size);

#endif
