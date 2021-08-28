#include "framework/logger.h"

#include "json_scanner.h"

//
#include "json.h"

struct JSON_Parser {
	struct JSON_Scanner scanner;
	struct JSON_Token previous, current;
	uint32_t depth;
};

static void json_left_pad(struct JSON_Parser * parser) {
	logger_to_console("%*s", parser->depth * 4, "");
}

static void json_consume(struct JSON_Parser * parser) {
	parser->previous = parser->current;
	for(;;) {
		parser->current = json_scanner_next(&parser->scanner);
		if (parser->current.type != JSON_TOKEN_COMMENT) { break; }
		json_left_pad(parser);
		logger_to_console("%.*s\n", parser->current.length - 1, parser->current.data);
	}
}

static bool json_match(struct JSON_Parser * parser, enum JSON_Token_Type type) {
	if (parser->current.type != type) { return false; }
	json_consume(parser);
	return true;
}

static void json_do_value(struct JSON_Parser * parser);
static void json_do_object(struct JSON_Parser * parser);
static void json_do_array(struct JSON_Parser * parser);
static void json_do_string(struct JSON_Parser * parser);
static void json_do_number(struct JSON_Parser * parser);
static void json_do_literal(struct JSON_Parser * parser);

static void json_do_value(struct JSON_Parser * parser) {
	if (json_match(parser, JSON_TOKEN_LEFT_BRACE)) {
		json_do_object(parser);
		return;
	}

	if (json_match(parser, JSON_TOKEN_LEFT_SQUARE)) {
		json_do_array(parser);
		return;
	}

	switch (parser->current.type) {
		case JSON_TOKEN_STRING: json_do_string(parser); break;
		case JSON_TOKEN_NUMBER: json_do_number(parser); break;

		case JSON_TOKEN_TRUE:
		case JSON_TOKEN_FALSE:
		case JSON_TOKEN_NULL:
			json_do_literal(parser); break;

		default: DEBUG_BREAK(); break;
	}

	json_consume(parser);
}

static void json_do_object(struct JSON_Parser * parser) {
	logger_to_console("{\n");
	parser->depth++;

	while (parser->current.type != JSON_TOKEN_RIGHT_BRACE) {
		if (parser->current.type != JSON_TOKEN_STRING) {
			logger_to_console("ERROR: expected string key\n");
			break;
		}

		json_left_pad(parser);
		json_do_string(parser);
		json_consume(parser);

		if (!json_match(parser, JSON_TOKEN_COLON)) {
			logger_to_console("ERROR: expected ':'\n");
			break;
		}

		logger_to_console(":");
		json_do_value(parser);
		logger_to_console(",\n");

		if (parser->current.type == JSON_TOKEN_RIGHT_BRACE) { break; }
		if (!json_match(parser, JSON_TOKEN_COMMA)) {
			logger_to_console("ERROR: expected ','\n");
		}
	}
	if (!json_match(parser, JSON_TOKEN_RIGHT_BRACE)) {
		logger_to_console("ERROR: expected '}'\n");
	}

	parser->depth--;
	json_left_pad(parser);
	logger_to_console("}");
}

static void json_do_array(struct JSON_Parser * parser) {
	logger_to_console("[\n");
	parser->depth++;

	while (parser->current.type != JSON_TOKEN_RIGHT_SQUARE) {
		json_left_pad(parser);
		json_do_value(parser);
		logger_to_console(",\n");

		if (parser->current.type == JSON_TOKEN_RIGHT_SQUARE) { break; }
		if (!json_match(parser, JSON_TOKEN_COMMA)) {
			logger_to_console("ERROR: expected ','\n");
		}
	}
	if (!json_match(parser, JSON_TOKEN_RIGHT_SQUARE)) {
		logger_to_console("ERROR: expected ']'\n");
	}

	parser->depth--;
	json_left_pad(parser);
	logger_to_console("]");
}

static void json_do_string(struct JSON_Parser * parser) {
	logger_to_console("%.*s", parser->current.length, parser->current.data);
}

static void json_do_number(struct JSON_Parser * parser) {
	logger_to_console("%.*s", parser->current.length, parser->current.data);
}

static void json_do_literal(struct JSON_Parser * parser) {
	switch (parser->current.type) {
		case JSON_TOKEN_TRUE: logger_to_console("true"); break;
		case JSON_TOKEN_FALSE: logger_to_console("false"); break;
		case JSON_TOKEN_NULL: logger_to_console("null"); break;

		default: DEBUG_BREAK(); break;
	}
}

struct JSON_Value json_read(char const * data) {
	struct JSON_Value value = {0};

	struct JSON_Parser parser = {0};
	json_scanner_init(&parser.scanner, data);
	json_consume(&parser);

	while (parser.current.type != JSON_TOKEN_EOF) {
		json_do_value(&parser);
		logger_to_console(",\n");
		json_left_pad(&parser);

		if (parser.current.type == JSON_TOKEN_EOF) { break; }
		if (!json_match(&parser, JSON_TOKEN_COMMA)) {
			logger_to_console("ERROR: expected ','\n");
		}
	}

	json_scanner_free(&parser.scanner);

	return value;
}
