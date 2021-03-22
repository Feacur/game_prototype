#include "framework/containers/array_byte.h"
#include "memory.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

//
#include "framework/platform_file.h"

/*
bool platform_file_read(char const * path, struct Array_Byte * buffer) {
	HANDLE handle = CreateFileA(
		path,
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "'CreateFileA' failed\n"); DEBUG_BREAK();
		CloseHandle(handle);
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
*/

uint64_t platform_file_size(char const * path) {
	WIN32_FIND_DATAA find_file_data;
	HANDLE handle = FindFirstFileA(path, &find_file_data);
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
	WIN32_FIND_DATAA find_file_data;
	HANDLE handle = FindFirstFileA(path, &find_file_data);
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
