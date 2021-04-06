#include "framework/containers/array_byte.h"
#include "framework/memory.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

#if defined(UNICODE)
	static wchar_t * platform_file_allocate_utf8_to_utf16(char const * value);
#endif

//
#include "framework/platform_file.h"

// @todo: sidestep `MAX_PATH` limit?

struct File {
	HANDLE handle;
	enum File_Mode mode;
};

bool platform_file_read_entire(char const * path, struct Array_Byte * buffer) {
	array_byte_init(buffer);

	struct File * file = platform_file_init(path, FILE_MODE_READ);
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

void platform_file_delete(char const * path) {
#if defined(UNICODE)
	wchar_t * path_valid = platform_file_allocate_utf8_to_utf16(path);
#else
	char const * path_valid = path;
#endif

	DeleteFile(path_valid);

#if defined(UNICODE)
	MEMORY_FREE(NULL, path_valid);
#endif
}

struct File * platform_file_init(char const * path, enum File_Mode mode) {
	DWORD access = 0, share = 0, creation = OPEN_EXISTING;
	if (mode & FILE_MODE_READ) {
		access |= GENERIC_READ;
		share |= FILE_SHARE_READ;
	}

	if (mode & FILE_MODE_WRITE) {
		access |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
		creation = OPEN_ALWAYS;
	}

	if (mode & FILE_MODE_DELETE) {
		share |= FILE_SHARE_DELETE;
	}

#if defined(UNICODE)
	wchar_t * path_valid = platform_file_allocate_utf8_to_utf16(path);
#else
	char const * path_valid = path;
#endif

	HANDLE handle = CreateFile(
		path_valid,
		access, share, NULL, creation,
		FILE_ATTRIBUTE_NORMAL, NULL
	);

#if defined(UNICODE)
	MEMORY_FREE(NULL, path_valid);
#endif

	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "'CreateFile' failed\n"); DEBUG_BREAK();
		return NULL;
	}

	struct File * file = MEMORY_ALLOCATE(NULL, struct File);
	*file = (struct File){
		.handle = handle,
		.mode = mode,
	};
	return file;
}

void platform_file_free(struct File * file) {
	CloseHandle(file->handle);
	memset(file, 0, sizeof(*file));
	MEMORY_FREE(file, file);
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

	if (!(file->mode & FILE_MODE_READ)) {
		fprintf(stderr, "'platform_file_read' failed\n"); DEBUG_BREAK();
		return 0;
	}
	
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

	if (!(file->mode & FILE_MODE_WRITE)) {
		fprintf(stderr, "'platform_file_write' failed\n"); DEBUG_BREAK();
		return 0;
	}
	
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

//

#if defined(UNICODE)
	static wchar_t * platform_file_allocate_utf8_to_utf16(char const * value) {
		// @todo: use scratch buffer
		const int length = MultiByteToWideChar(CP_UTF8, 0, value, -1, NULL, 0);
		wchar_t * buffer = MEMORY_ALLOCATE_ARRAY(NULL, wchar_t, length);
		MultiByteToWideChar(CP_UTF8, 0, value, -1, buffer, length);
		return buffer;
	}
#endif
