#include "framework/containers/array_byte.h"
#include "framework/memory.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

struct File {
	HANDLE handle;
};

//
#include "framework/platform_file.h"

bool platform_file_read_entire(char const * path, struct Array_Byte * buffer) {
	array_byte_init(buffer);

	struct File * file = platform_file_init(path);
	uint64_t size = platform_file_size(file);

	if (size == 0) { platform_file_free(file); return true; }

	array_byte_resize(buffer, size + 1);
	buffer->count = platform_file_read(file, buffer->data, size);

	platform_file_free(file);
	if (buffer->count == size) { return true; }

	fprintf(stderr, "'platform_file_read_entire' failed\n"); DEBUG_BREAK();
	array_byte_free(buffer);
	return false;
}

struct File * platform_file_init(char const * path) {
// @todo: sidestep `MAX_PATH` limit?
#if defined(UNICODE)
	wchar_t * path_valid;
	{
		const int length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
		path_valid = MEMORY_ALLOCATE_ARRAY(wchar_t, length);
		MultiByteToWideChar(CP_UTF8, 0, path, -1, path_valid, length);
	}
#else
	char const * path_valid = path;
#endif

	HANDLE handle = CreateFile(
		path_valid,
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

#if defined(UNICODE)
	// @todo: use scratch buffer
	MEMORY_FREE(path_valid);
#endif

	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "'CreateFile' failed\n"); DEBUG_BREAK();
		return NULL;
	}

	struct File * file = MEMORY_ALLOCATE(struct File);
	*file = (struct File){
		.handle = handle,
	};
	return file;
}

void platform_file_free(struct File * file) {
	CloseHandle(file->handle);
	memset(file, 0, sizeof(*file));
	MEMORY_FREE(file);
}

uint64_t platform_file_size(struct File * file) {
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(file->handle, &file_size)) { return 0; }
	return (uint64_t)file_size.QuadPart;
}

uint64_t platform_file_time(struct File * file) {
	FILETIME write;
	if (!GetFileTime(file->handle, NULL, NULL, &write)) { return 0; }

	ULARGE_INTEGER const large = {
		.LowPart = write.dwLowDateTime,
		.HighPart = write.dwHighDateTime,
	};

	return (uint64_t)large.QuadPart;
}

uint64_t platform_file_position_get(struct File * file) {
	LARGE_INTEGER input = {.QuadPart = 0}, output;
	if (!SetFilePointerEx(file->handle, input, &output, FILE_CURRENT)) { return 0; }
	return (uint64_t)output.QuadPart;
}

uint64_t platform_file_position_set(struct File * file, uint64_t position) {
	LARGE_INTEGER input = {.QuadPart = (LONGLONG)position}, output;
	if (!SetFilePointerEx(file->handle, input, &output, FILE_BEGIN)) { return 0; }
	return (uint64_t)output.QuadPart;
}

uint64_t platform_file_read(struct File * file, uint8_t * buffer, uint64_t size) {
	DWORD const max_chunk_size = UINT16_MAX + 1;
	
	uint64_t read = 0;
	while (read < size) {
		DWORD const to_read = ((size - read) < (uint64_t)max_chunk_size)
			? (DWORD)(size - read)
			: max_chunk_size;

		DWORD read_chunk_size;
		if (!ReadFile(file->handle, buffer + read, to_read, &read_chunk_size, NULL)) {
			fprintf(stderr, "'ReadFile' failed\n"); DEBUG_BREAK();
			break;
		}

		read += read_chunk_size;
		if (read_chunk_size < to_read) { break; }
	}

	return read;
}

uint64_t platform_file_write(struct File * file, uint8_t * buffer, uint64_t size) {
	DWORD const max_chunk_size = UINT16_MAX + 1;
	
	uint64_t written = 0;
	while (written < size) {
		DWORD const to_write = ((size - written) < (uint64_t)max_chunk_size)
			? (DWORD)(size - written)
			: max_chunk_size;

		DWORD read_chunk_size;
		if (!WriteFile(file->handle, buffer + written, to_write, &read_chunk_size, NULL)) {
			fprintf(stderr, "'WriteFile' failed\n"); DEBUG_BREAK();
			break;
		}

		written += read_chunk_size;
		if (read_chunk_size < to_write) { break; }
	}

	return written;
}
