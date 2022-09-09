#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/assets/json.h"
#include "framework/platform_file.h"
#include "framework/parsing.h"
#include "framework/logger.h"
#include "framework/maths.h"

//
#include "json_read.h"

uint32_t json_read_hex(struct JSON const * json) {
	struct CString const value = json_as_string(json);
	if (value.length > 2 && value.data[0] == '0' && value.data[1] == 'x')
	{
		return parse_hex_u32(value.data + 2);
	}
	return parse_hex_u32(value.data);
}

void process_json(struct CString path, void * data, void (* action)(struct JSON const * json, void * output)) {
	struct Buffer file_buffer = platform_file_read_entire(path);
	if (file_buffer.capacity == 0) { action(&c_json_error, data); return; }

	struct Strings strings = strings_init();

	struct JSON json = json_init(&strings, (char const *)file_buffer.data);
	buffer_free(&file_buffer);

	action(&json, data);

	json_free(&json);
	strings_free(&strings);
}


void json_read_many_flt(struct JSON const * json, uint32_t length, float * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (float)json_at_number(json, i);
		}
	}
	else if (json->type == JSON_NUMBER) {
		*result = (float)json_as_number(json);
	}
}

void json_read_many_u32(struct JSON const * json, uint32_t length, uint32_t * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (uint32_t)json_at_number(json, i);
		}
	}
	else if (json->type == JSON_NUMBER) {
		*result = (uint32_t)json_as_number(json);
	}
}

void json_read_many_s32(struct JSON const * json, uint32_t length, int32_t * result) {
	if (json->type == JSON_ARRAY) {
		uint32_t const count = min_u32(length, json_count(json));
		for (uint32_t i = 0; i < count; i++) {
			result[i] = (int32_t)json_at_number(json, i);
		}
	}
	else if (json->type == JSON_NUMBER) {
		*result = (int32_t)json_as_number(json);
	}
}
