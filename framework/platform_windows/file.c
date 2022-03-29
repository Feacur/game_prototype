#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/containers/buffer.h"

#include <Windows.h>
#include <malloc.h>

// @todo: sidestep `MAX_PATH` limit?
// @idea: async file access through OS API

//
#include "framework/platform_file.h"

struct File {
	HANDLE handle;
	enum File_Mode mode;
	uint32_t path_length;
	char * path;
};

struct Buffer platform_file_read_entire(struct CString path) {
	if (path.length == 0) { return (struct Buffer){0}; }
	// if (path.data == NULL) { return (struct Buffer){0}; }

	struct Buffer buffer = buffer_init();

	struct File * file = platform_file_init(path, FILE_MODE_READ);
	if (file == NULL) { goto finalize; }

	uint64_t const size = platform_file_size(file);
	if (size == 0) { goto finalize; }

	uint64_t const padding = 1;
	buffer_resize(&buffer, size + padding);
	buffer.count = platform_file_read(file, buffer.data, size);
	if (padding > 0) { buffer.data[buffer.count] = '\0'; }

	if (buffer.count != size) {
		logger_to_console("read failure: %zu / %zu bytes; \"%.*s\"\n", buffer.count, size, path.length, path.data);
		buffer_free(&buffer);
	}

	finalize:
	platform_file_free(file);
	return buffer;
}

void platform_file_delete(struct CString path) {
#if defined(UNICODE)
	// @todo: (?) arena/stack allocator
	const int path_valid_length = MultiByteToWideChar(CP_UTF8, 0, path.data, -1, NULL, 0);
	wchar_t * path_valid = alloca(sizeof(wchar_t) * (uint64_t)path_valid_length);
	MultiByteToWideChar(CP_UTF8, 0, path.data, -1, path_valid, path_valid_length);
#else
	char const * path_valid = path.data;
#endif

	DeleteFile(path_valid);
}

static HANDLE platform_internal_create_file(struct CString path, enum File_Mode mode);
struct File * platform_file_init(struct CString path, enum File_Mode mode) {
	HANDLE handle = platform_internal_create_file(path, mode);
	if (handle == INVALID_HANDLE_VALUE) {
		logger_to_console("'CreateFile' failed; \"%.*s\"\n", path.length, path.data);
		return NULL;
	}

	struct File * file = MEMORY_ALLOCATE(NULL, struct File);
	*file = (struct File){
		.handle = handle,
		.mode = mode,
	};

	file->path_length = path.length;
	file->path = MEMORY_ALLOCATE_ARRAY(file, char, path.length);
	common_memcpy(file->path, path.data, file->path_length);

	return file;
}

void platform_file_free(struct File * file) {
	CloseHandle(file->handle);
	MEMORY_FREE(file, file->path);
	common_memset(file, 0, sizeof(*file));
	MEMORY_FREE(NULL, file);
}

uint64_t platform_file_size(struct File const * file) {
	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(file->handle, &file_size)) { return 0; }
	return (uint64_t)file_size.QuadPart;
}

uint64_t platform_file_time(struct File const * file) {
	FILETIME write;
	if (!GetFileTime(file->handle, NULL, NULL, &write)) { return 0; }

	ULARGE_INTEGER const large = {
		.LowPart = write.dwLowDateTime,
		.HighPart = write.dwHighDateTime,
	};

	return (uint64_t)large.QuadPart;
}

uint64_t platform_file_position_get(struct File const * file) {
	LARGE_INTEGER input = {.QuadPart = 0}, output;
	if (!SetFilePointerEx(file->handle, input, &output, FILE_CURRENT)) { return 0; }
	return (uint64_t)output.QuadPart;
}

uint64_t platform_file_position_set(struct File * file, uint64_t position) {
	LARGE_INTEGER input = {.QuadPart = (LONGLONG)position}, output;
	if (!SetFilePointerEx(file->handle, input, &output, FILE_BEGIN)) { return 0; }
	return (uint64_t)output.QuadPart;
}

uint64_t platform_file_read(struct File const * file, uint8_t * buffer, uint64_t size) {
	DWORD const max_chunk_size = UINT16_MAX + 1;

	if (!(file->mode & FILE_MODE_READ)) {
		logger_to_console("can't read write-only files; \"%.*s\"\n", file->path_length, file->path); DEBUG_BREAK();
		return 0;
	}
	
	uint64_t read = 0;
	while (read < size) {
		DWORD const to_read = ((size - read) < (uint64_t)max_chunk_size)
			? (DWORD)(size - read)
			: max_chunk_size;

		DWORD read_chunk_size;
		if (!ReadFile(file->handle, buffer + read, to_read, &read_chunk_size, NULL)) {
			logger_to_console("'ReadFile' failed; \"%.*s\"\n", file->path_length, file->path); DEBUG_BREAK();
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
		logger_to_console("can't write read-only files; \"%.*s\"\n", file->path_length, file->path); DEBUG_BREAK();
		return 0;
	}
	
	uint64_t written = 0;
	while (written < size) {
		DWORD const to_write = ((size - written) < (uint64_t)max_chunk_size)
			? (DWORD)(size - written)
			: max_chunk_size;

		DWORD read_chunk_size;
		if (!WriteFile(file->handle, buffer + written, to_write, &read_chunk_size, NULL)) {
			logger_to_console("'WriteFile' failed; \"%.*s\"\n", file->path_length, file->path); DEBUG_BREAK();
			break;
		}

		written += read_chunk_size;
		if (read_chunk_size < to_write) { break; }
	}

	return written;
}

//

static HANDLE platform_internal_create_file(struct CString path, enum File_Mode mode) {
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
	// @todo: (?) arena/stack allocator
	const int path_valid_length = MultiByteToWideChar(CP_UTF8, 0, path.data, -1, NULL, 0);
	wchar_t * path_valid = alloca(sizeof(wchar_t) * (uint64_t)path_valid_length);
	MultiByteToWideChar(CP_UTF8, 0, path.data, -1, path_valid, path_valid_length);
#else
	char const * path_valid = path.data;
#endif

	return CreateFile(
		path_valid,
		access, share, NULL, creation,
		FILE_ATTRIBUTE_NORMAL, NULL
	);
}
