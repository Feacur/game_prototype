#include "framework/logger.h"

#include "json_scanner.h"

//
#include "json.h"

struct JSON_Value json_read(char const * data) {
	struct JSON_Value value = {0};

	struct JSON_Scanner scanner;
	json_scanner_init(&scanner, data);
	while (true) {
		struct JSON_Token token = json_scanner_next(&scanner);
		if (token.type == JSON_TOKEN_EOF) { break; }
		// if (token.type == JSON_TOKEN_ERROR) { break; }
		switch (token.type) {
			case JSON_TOKEN_NONE: logger_to_console("JSON_TOKEN_NONE\n"); break;
			case JSON_TOKEN_COMMENT: logger_to_console("JSON_TOKEN_COMMENT\n"); break;
			case JSON_TOKEN_COLON: logger_to_console("JSON_TOKEN_COLON\n"); break;
			case JSON_TOKEN_LEFT_BRACE: logger_to_console("JSON_TOKEN_LEFT_BRACE\n"); break;
			case JSON_TOKEN_RIGHT_BRACE: logger_to_console("JSON_TOKEN_RIGHT_BRACE\n"); break;
			case JSON_TOKEN_LEFT_SQUARE: logger_to_console("JSON_TOKEN_LEFT_SQUARE\n"); break;
			case JSON_TOKEN_RIGHT_SQUARE: logger_to_console("JSON_TOKEN_RIGHT_SQUARE\n"); break;
			case JSON_TOKEN_COMMA: logger_to_console("JSON_TOKEN_COMMA\n"); break;
			case JSON_TOKEN_IDENTIFIER: logger_to_console("JSON_TOKEN_IDENTIFIER\n"); break;
			case JSON_TOKEN_NUMBER: logger_to_console("JSON_TOKEN_NUMBER\n"); break;
			case JSON_TOKEN_STRING: logger_to_console("JSON_TOKEN_STRING\n"); break;
			case JSON_TOKEN_TRUE: logger_to_console("JSON_TOKEN_TRUE\n"); break;
			case JSON_TOKEN_FALSE: logger_to_console("JSON_TOKEN_FALSE\n"); break;
			case JSON_TOKEN_NULL: logger_to_console("JSON_TOKEN_NULL\n"); break;
			case JSON_TOKEN_ERROR: logger_to_console("JSON_TOKEN_ERROR %s\n", token.data); break;
			case JSON_TOKEN_EOF: logger_to_console("JSON_TOKEN_EOF\n"); break;
		}
	}
	json_scanner_free(&scanner);

	return value;
}
