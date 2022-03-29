#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/assets/json.h"
#include "framework/platform_file.h"
#include "framework/logger.h"

//
#include "utilities.h"

void process_json(void (* action)(struct JSON const * json, void * output), void * data, struct CString path) {
	struct Buffer file_buffer = platform_file_read_entire(path);
	if (file_buffer.capacity == 0) { DEBUG_BREAK(); return; }

	struct Strings strings = strings_init();

	struct JSON json = json_init(&strings, (char const *)file_buffer.data);
	buffer_free(&file_buffer);

	action(&json, data);

	json_free(&json);
	strings_free(&strings);
}
