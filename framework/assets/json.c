#include "framework/logger.h"
#include "framework/memory.h"
#include "framework/parsing.h"

#include "framework/containers/array_any.h"
#include "framework/containers/array_byte.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/containers/strings.h"

#include "json_scanner.h"

#include <string.h>

//
#include "framework/internal/json_to_system.h"

static struct JSON_System {
	struct Strings strings;
} json_system;

void json_to_system_init(void) {
	strings_init(&json_system.strings);
	strings_add(&json_system.strings, 0, "");
}

void json_to_system_free(void) {
	strings_free(&json_system.strings);
}

//
#include "json.h"

// -- JSON system part
uint32_t json_system_add_string(char const * value) {
	return strings_add(&json_system.strings, (uint32_t)strlen(value), value);
}

uint32_t json_system_find_string(char const * value) {
	return strings_find(&json_system.strings, (uint32_t)strlen(value), value);
}

// -- JSON value part
enum JSON_Type {
	JSON_NULL,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOLEAN,
};

struct JSON_Object {
	struct Hash_Table_U32 value;
};

struct JSON_Array {
	struct Array_Any value;
};

struct JSON_String {
	uint32_t value;
};

struct JSON_Number {
	float value;
};

struct JSON_Boolean {
	bool value;
};

struct JSON {
	enum JSON_Type type;
	union {
		struct JSON_Object  object;
		struct JSON_Array   array;
		struct JSON_String  string;
		struct JSON_Number  number;
		struct JSON_Boolean boolean;
	} as;
};

static struct JSON json_null = {.type = JSON_NULL,};

static void json_parser_do_internal(char const * data, struct JSON * value);
struct JSON * json_init(char const * data) {
	struct JSON * value = MEMORY_ALLOCATE(NULL, struct JSON);
	json_parser_do_internal(data, value);
	return value;
}

static void json_free_internal(struct JSON * value) {
	switch (value->type) {
		case JSON_OBJECT:
			struct JSON_Object * object = &value->as.object;
			for (struct Hash_Table_U32_Iterator it = {0}; hash_table_u32_iterate(&object->value, &it); /*empty*/) {
				json_free_internal(it.value);
			}
			hash_table_u32_free(&object->value);
			break;

		case JSON_ARRAY:
			struct JSON_Array * array = &value->as.array;
			for (uint32_t i = 0; i < array->value.count; i++) {
				json_free_internal(array_any_at(&array->value, i));
			}
			array_any_free(&array->value);
			break;

		default: break;
	}
}

void json_free(struct JSON * value) {
	json_free_internal(value);
	MEMORY_FREE(value, value);
}

bool json_is_null(struct JSON const * value)    { return value->type == JSON_NULL; }
bool json_is_object(struct JSON const * value)  { return value->type == JSON_OBJECT; }
bool json_is_array(struct JSON const * value)   { return value->type == JSON_ARRAY; }
bool json_is_string(struct JSON const * value)  { return value->type == JSON_STRING; }
bool json_is_number(struct JSON const * value)  { return value->type == JSON_NUMBER; }
bool json_is_boolean(struct JSON const * value) { return value->type == JSON_BOOLEAN; }

struct JSON const * json_object_get(struct JSON const * value, char const * key) {
	if (value->type != JSON_OBJECT) { return &json_null; }
	struct JSON_Object const * object = &value->as.object;
	uint32_t const string_id = strings_find(&json_system.strings, (uint32_t)strlen(key), key);
	void * result = hash_table_u32_get(&object->value, string_id);
	return (result != NULL) ? result : &json_null;
}

struct JSON const * json_array_at(struct JSON const * value, uint32_t index) {
	if (value->type != JSON_ARRAY) { return &json_null; }
	struct JSON_Array const * array = &value->as.array;
	void * result = array_any_at(&array->value, index);
	return (result != NULL) ? result : &json_null;
}

char const * json_as_string(struct JSON const * value, char const * default_value) {
	if (value->type != JSON_STRING) { return default_value; }
	struct JSON_String const * string = &value->as.string;
	return strings_get(&json_system.strings, string->value);
}

float json_as_number(struct JSON const * value, float default_value) {
	if (value->type != JSON_NUMBER) { return default_value; }
	struct JSON_Number const * number = &value->as.number;
	return number->value;
}

bool json_as_boolean(struct JSON const * value, bool default_value) {
	if (value->type != JSON_BOOLEAN) { return default_value; }
	struct JSON_Boolean const * boolean = &value->as.boolean;
	return boolean->value;
}

char const * json_get_string(struct JSON const * value, char const * key, char const * default_value) {
	return json_as_string(json_object_get(value, key), default_value);
}

float json_get_number(struct JSON const * value, char const * key, float default_value) {
	return json_as_number(json_object_get(value, key), default_value);
}

bool json_get_boolean(struct JSON const * value, char const * key, bool default_value) {
	return json_as_boolean(json_object_get(value, key), default_value);
}

//

struct JSON_Parser {
	struct JSON_Scanner scanner;
	struct JSON_Token previous, current;
	bool error, panic;
};

static void json_parser_error_at(struct JSON_Parser * parser, struct JSON_Token const * token, char const * message) {
	parser->error = true;

	if (parser->panic) { return; }
	parser->panic = true;

	logger_to_console("[line %u]", token->line + 1);
	switch (token->type) {
		case JSON_TOKEN_EOF:
			logger_to_console("[context: eof]");
			break;

		case JSON_TOKEN_ERROR:
			logger_to_console("[context: \"%.*s\"]", token->length, token->data);
			break;

		default:
			logger_to_console("[context: %.*s]", token->length, token->data);
			break;
	}
	if (message != NULL) { logger_to_console(": %s", message); }
	logger_to_console("\n");
}

static void json_parser_error_previous(struct JSON_Parser * parser, char const * message) {
	json_parser_error_at(parser, &parser->previous, message);
}

static void json_parser_error_current(struct JSON_Parser * parser, char const * message) {
	json_parser_error_at(parser, &parser->current, message);
}

static void json_parser_consume(struct JSON_Parser * parser) {
	parser->previous = parser->current;
	for(;;) {
		parser->current = json_scanner_next(&parser->scanner);
		if (parser->current.type == JSON_TOKEN_COMMENT) {
			continue;
		}
		if (parser->current.type == JSON_TOKEN_IDENTIFIER) {
			json_parser_error_current(parser, "identifier is an unvalid token");
			continue;
		}
		if (parser->current.type == JSON_TOKEN_ERROR) {
			json_parser_error_previous(parser, "context for vvvvv"); parser->panic = false;
			json_parser_error_current(parser, "scanner error");
			continue;
		}
		break;
	}
}

static bool json_parser_match(struct JSON_Parser * parser, enum JSON_Token_Type type) {
	if (parser->current.type != type) { return false; }
	json_parser_consume(parser);
	return true;
}

static bool json_parser_synchronize_to(struct JSON_Parser * parser, enum JSON_Token_Type scope) {
	parser->panic = false;
	for (;;) {
		json_parser_consume(parser);
		if (parser->current.type == scope) { break; }
		if (parser->current.type == JSON_TOKEN_EOF) { break; }

		if (parser->current.type == JSON_TOKEN_COMMA) {
			json_parser_consume(parser); return true;
		}
	}
	return false;
}

static void json_parser_do_value(struct JSON_Parser * parser, struct JSON * value);
static void json_parser_do_string(struct JSON_Parser * parser, struct JSON * value);

static void json_parser_do_object(struct JSON_Parser * parser, struct JSON * value) {
	*value = (struct JSON){.type = JSON_OBJECT,};
	struct JSON_Object * object = &value->as.object;
	hash_table_u32_init(&object->value, sizeof(struct JSON));

	while (parser->current.type != JSON_TOKEN_RIGHT_BRACE && parser->current.type != JSON_TOKEN_EOF) {
		if (parser->current.type != JSON_TOKEN_STRING) {
			json_parser_error_current(parser, "expected string");
			if (!json_parser_synchronize_to(parser, JSON_TOKEN_RIGHT_BRACE)) { break; }
		}

		struct JSON entry_key;
		json_parser_do_string(parser, &entry_key);
		json_parser_consume(parser);

		if (!json_parser_match(parser, JSON_TOKEN_COLON)) {
			json_parser_error_previous(parser, "expected ':'");
			if (!json_parser_synchronize_to(parser, JSON_TOKEN_RIGHT_BRACE)) { break; }
		}

		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		hash_table_u32_set(&object->value, entry_key.as.string.value, &entry_value);

		if (parser->current.type == JSON_TOKEN_RIGHT_BRACE) { break; }
		if (!json_parser_match(parser, JSON_TOKEN_COMMA)) {
			json_parser_error_previous(parser, "expected ','");
			if (!json_parser_synchronize_to(parser, JSON_TOKEN_RIGHT_BRACE)) { break; }
		}
	}
	if (!json_parser_match(parser, JSON_TOKEN_RIGHT_BRACE)) {
		json_parser_error_previous(parser, "expected '}'");
	}
}

static void json_parser_do_array(struct JSON_Parser * parser, struct JSON * value) {
	*value = (struct JSON){.type = JSON_ARRAY,};
	struct JSON_Array * array = &value->as.array;
	array_any_init(&array->value, sizeof(struct JSON));

	while (parser->current.type != JSON_TOKEN_RIGHT_SQUARE && parser->current.type != JSON_TOKEN_EOF) {
		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		array_any_push(&array->value, &entry_value);

		if (parser->current.type == JSON_TOKEN_RIGHT_SQUARE) { break; }
		if (!json_parser_match(parser, JSON_TOKEN_COMMA)) {
			json_parser_error_previous(parser, "expected ','");
			if (!json_parser_synchronize_to(parser, JSON_TOKEN_RIGHT_SQUARE)) { break; }
		}
	}
	if (!json_parser_match(parser, JSON_TOKEN_RIGHT_SQUARE)) {
		json_parser_error_previous(parser, "expected ']'");
	}
}

static void json_parser_do_string(struct JSON_Parser * parser, struct JSON * value) {
	uint32_t const string_id = strings_add(&json_system.strings, parser->current.length - 2, parser->current.data + 1);
	*value = (struct JSON){
		.type = JSON_STRING,
		.as.string = {.value = string_id,},
	};
}

static void json_parser_do_number(struct JSON_Parser * parser, struct JSON * value) {
	float const number = parse_float(parser->current.data);
	*value = (struct JSON){
		.type = JSON_NUMBER,
		.as.number = {.value = number,},
	};
}

static void json_parser_do_literal(struct JSON_Parser * parser, struct JSON * value) {
	switch (parser->current.type) {
		case JSON_TOKEN_TRUE: *value = (struct JSON){
			.type = JSON_BOOLEAN,
			.as.boolean = {.value = true,},
		}; break;

		case JSON_TOKEN_FALSE: *value = (struct JSON){
			.type = JSON_BOOLEAN,
			.as.boolean = {.value = false,},
		}; break;

		case JSON_TOKEN_NULL: *value = (struct JSON){
			.type = JSON_NULL,
		}; break;

		default:
			DEBUG_BREAK(); // unreachable
			json_parser_error_current(parser, "expected literal");
			*value = json_null; break;
	}
}

static void json_parser_do_value(struct JSON_Parser * parser, struct JSON * value) {
	if (json_parser_match(parser, JSON_TOKEN_LEFT_BRACE)) {
		json_parser_do_object(parser, value);
		return;
	}

	if (json_parser_match(parser, JSON_TOKEN_LEFT_SQUARE)) {
		json_parser_do_array(parser, value);
		return;
	}

	switch (parser->current.type) {
		case JSON_TOKEN_STRING: json_parser_do_string(parser, value); break;
		case JSON_TOKEN_NUMBER: json_parser_do_number(parser, value); break;

		case JSON_TOKEN_TRUE:
		case JSON_TOKEN_FALSE:
		case JSON_TOKEN_NULL:
			json_parser_do_literal(parser, value); break;

		default:
			json_parser_error_current(parser, "expected value");
			*value = json_null; break;
	}

	json_parser_consume(parser);
}

static void json_parser_do_internal(char const * data, struct JSON * value) {
	struct JSON_Parser parser = {0};
	json_scanner_init(&parser.scanner, data);
	json_parser_consume(&parser);

	json_parser_do_value(&parser, value);
	if (parser.current.type != JSON_TOKEN_EOF) {
		json_parser_error_current(&parser, "expected eof");
	}

	if (parser.error) {
		json_free_internal(value);
		*value = json_null;
	}

	json_scanner_free(&parser.scanner);
}
