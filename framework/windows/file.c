#include "framework/containers/array_byte.h"
#include "framework/memory.h"

// @todo: sidestep `MAX_PATH` limit?
// @todo: support `UNICODE` define?

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

//
#include "framework/platform_file.h"

static wchar_t * platform_file_allocate_utf8_to_utf16(char const * value);

bool platform_file_read(char const * path, struct Array_Byte * buffer) {
	wchar_t * path_utf16 = platform_file_allocate_utf8_to_utf16(path);
	HANDLE handle = CreateFileW(
		path_utf16,
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	MEMORY_FREE(path_utf16);

	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "'CreateFile' failed\n"); DEBUG_BREAK();
		return false;
	}

	LARGE_INTEGER file_size_from_api;
	if (!GetFileSizeEx(handle, &file_size_from_api)) {
		fprintf(stderr, "'GetFileSizeEx' failed\n"); DEBUG_BREAK();
		CloseHandle(handle);
		return false;
	}

	// @todo: support large files?
	if (file_size_from_api.QuadPart >= UINT32_MAX) { fprintf(stderr, "file is too large\n"); DEBUG_BREAK(); return false; }

	uint64_t const file_size = (uint64_t)file_size_from_api.QuadPart;
	uint8_t * data = MEMORY_ALLOCATE_ARRAY(uint8_t, file_size + 1);

	DWORD bytes_read;
	if (!ReadFile(handle, data, (DWORD)file_size, &bytes_read, NULL) || bytes_read < file_size) {
		fprintf(stderr, "'ReadFile' failed\n"); DEBUG_BREAK();
		CloseHandle(handle);
		return false;
	}

	CloseHandle(handle);

	*buffer = (struct Array_Byte){
		.capacity = (uint32_t)(file_size + 1),
		.count = (uint32_t)file_size,
		.data = data,
	};
	return true;
}

uint64_t platform_file_size(char const * path) {
	WIN32_FIND_DATAW find_file_data;
	wchar_t * path_utf16 = platform_file_allocate_utf8_to_utf16(path);
	HANDLE handle = FindFirstFileW(path_utf16, &find_file_data);
	MEMORY_FREE(path_utf16);

	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "'FindFirstFileA' failed\n"); DEBUG_BREAK();
		return 0;
	}

	ULARGE_INTEGER const large = {
		.LowPart = find_file_data.nFileSizeLow,
		.HighPart = find_file_data.nFileSizeHigh,
	};

	FindClose(handle);

	return (uint64_t)large.QuadPart;
}

uint64_t platform_file_time(char const * path) {
	WIN32_FIND_DATAW find_file_data;
	wchar_t * path_utf16 = platform_file_allocate_utf8_to_utf16(path);
	HANDLE handle = FindFirstFileW(path_utf16, &find_file_data);
	MEMORY_FREE(path_utf16);

	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "'FindFirstFileA' failed\n"); DEBUG_BREAK();
		return 0;
	}

	ULARGE_INTEGER const large = {
		.LowPart = find_file_data.ftLastWriteTime.dwLowDateTime,
		.HighPart = find_file_data.ftLastWriteTime.dwHighDateTime,
	};

	FindClose(handle);

	return (uint64_t)large.QuadPart;
}

//

static wchar_t * platform_file_allocate_utf8_to_utf16(char const * value) {
	// @todo: use scratch buffer
	const int length = MultiByteToWideChar(CP_UTF8, 0, value, -1, NULL, 0);
	wchar_t * buffer = MEMORY_ALLOCATE_ARRAY(wchar_t, length);
	MultiByteToWideChar(CP_UTF8, 0, value, -1, buffer, length);
	return buffer;
}

/*
FILE * platform_file_fopen(char const * path, char const * mode) {
	// @todo: use scratch buffer
	const int path_length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
	const int mode_length = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);
	wchar_t * buffer = MEMORY_ALLOCATE_ARRAY(wchar_t, path_length + mode_length);
	MultiByteToWideChar(CP_UTF8, 0, path, -1, buffer + 0, path_length);
	MultiByteToWideChar(CP_UTF8, 0, mode, -1, buffer + path_length, mode_length);

	FILE * file = _wfopen(buffer + 0, buffer + path_length);
	MEMORY_FREE(buffer);

	return file;
}
*/
