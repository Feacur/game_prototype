#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/assets/json.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

//
#include "utilities.h"

void process_json(void (* action)(struct JSON const * json, void * output), void * output, struct CString path) {
	struct Buffer buffer;
	bool const read_success = platform_file_read_entire(path, &buffer);
	if (!read_success || buffer.count == 0) { DEBUG_BREAK(); return; }

	struct Strings strings;
	strings_init(&strings);

	struct JSON json;
	json_init(&json, &strings, (char const *)buffer.data);
	buffer_free(&buffer);

	action(&json, output);

	json_free(&json);
	strings_free(&strings);
}
