#include "framework/memory.h"
#include "framework/formatter.h"
#include "framework/containers/buffer.h"

#include <initguid.h> // `DEFINE_GUID`
#include <Windows.h>

// @idea: async file access through OS API

//
#include "framework/platform/file.h"

struct File {
	HANDLE handle;
	enum File_Mode mode;
	uint32_t path_length;
	char * path;
};

struct Buffer platform_file_read_entire(struct CString path) {
	struct Buffer buffer = buffer_init(NULL);

	struct File * file = platform_file_init(path, FILE_MODE_NONE);
	if (file == NULL) { goto finalize; }

	uint64_t const size = platform_file_size(file);
	if (size == 0) { goto finalize; }

	buffer_resize(&buffer, size + 1);
	buffer.size = platform_file_read(file, buffer.data, size);
	buffer_push_many(&buffer, 1, "\0"); buffer.size--;

	if (buffer.size != size) {
		WRN("read failure: %zu / %zu bytes; \"%.*s\"", buffer.size, size, path.length, path.data);
		buffer_free(&buffer);
	}

	finalize:
	if (file != NULL) { platform_file_free(file); }
	return buffer;
}

bool platform_file_delete(struct CString path) {
	return DeleteFileA(path.data);
}

static HANDLE platform_file_internal_create(struct CString path, enum File_Mode mode);
struct File * platform_file_init(struct CString path, enum File_Mode mode) {
	if (path.data == NULL) { return NULL; }

	HANDLE handle = platform_file_internal_create(path, mode);
	if (handle == INVALID_HANDLE_VALUE) { goto fail; }

	struct File * file = MEMORY_ALLOCATE(struct File);
	*file = (struct File){
		.handle = handle,
		.mode = mode,
		.path_length = path.length,
		.path = MEMORY_ALLOCATE_ARRAY(char, path.length),
	};
	common_memcpy(file->path, path.data, file->path_length);

	return file;

	// process errors
	fail: ERR("'CreateFile' failed; \"%.*s\"", path.length, path.data);
	REPORT_CALLSTACK(); DEBUG_BREAK();
	return NULL;
}

void platform_file_free(struct File * file) {
	CloseHandle(file->handle);
	MEMORY_FREE(file->path);
	common_memset(file, 0, sizeof(*file));
	MEMORY_FREE(file);
}

void platform_file_end(struct File * file) {
	SetEndOfFile(file->handle);
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

	uint64_t read = 0;
	while (read < size) {
		DWORD const to_read = ((size - read) < (uint64_t)max_chunk_size)
			? (DWORD)(size - read)
			: max_chunk_size;

		DWORD read_chunk_size;
		if (!ReadFile(file->handle, buffer + read, to_read, &read_chunk_size, NULL)) {
			ERR("'ReadFile' failed; \"%.*s\"", file->path_length, file->path);
			REPORT_CALLSTACK(); DEBUG_BREAK(); break;
		}

		read += read_chunk_size;
		if (read_chunk_size < to_read) { break; }
	}

	return read;
}

uint64_t platform_file_write(struct File * file, uint8_t * buffer, uint64_t size) {
	DWORD const max_chunk_size = UINT16_MAX + 1;

	if (!(file->mode & FILE_MODE_WRITE)) {
		ERR("can't write read-only files; \"%.*s\"", file->path_length, file->path);
		REPORT_CALLSTACK(); DEBUG_BREAK(); return 0;
	}
	
	uint64_t written = 0;
	while (written < size) {
		DWORD const to_write = ((size - written) < (uint64_t)max_chunk_size)
			? (DWORD)(size - written)
			: max_chunk_size;

		DWORD read_chunk_size;
		if (!WriteFile(file->handle, buffer + written, to_write, &read_chunk_size, NULL)) {
			ERR("'WriteFile' failed; \"%.*s\"", file->path_length, file->path);
			REPORT_CALLSTACK(); DEBUG_BREAK(); break;
		}

		written += read_chunk_size;
		if (read_chunk_size < to_write) { break; }
	}

	return written;
}

//

static HANDLE platform_file_internal_create(struct CString path, enum File_Mode mode) {
	DWORD access = GENERIC_READ;
	DWORD share = FILE_SHARE_READ;
	DWORD creation = OPEN_EXISTING;

	if (mode & FILE_MODE_WRITE) {
		access |= GENERIC_WRITE;
		creation = (mode & FILE_MODE_FORCE)
			? OPEN_ALWAYS
			: CREATE_NEW;
	}

	return CreateFileA(
		path.data,
		access, share, NULL, creation,
		FILE_ATTRIBUTE_NORMAL, NULL
	);
}
