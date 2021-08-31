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
	// @idea: separate key and value strings?
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
uint32_t json_system_add_string_id(char const * value) {
	return strings_add(&json_system.strings, (uint32_t)strlen(value), value);
}

uint32_t json_system_find_string_id(char const * value) {
	return strings_find(&json_system.strings, (uint32_t)strlen(value), value);
}

char const * json_system_get_string_value(uint32_t value) {
	return strings_get(&json_system.strings, value);
}

// -- JSON value part
enum JSON_Type {
	JSON_NONE,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOLEAN,
	JSON_NULL,
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

static struct JSON json_none  = {.type = JSON_NONE,};
static struct JSON json_true  = {.type = JSON_BOOLEAN, .as.boolean = {.value = true},};
static struct JSON json_false = {.type = JSON_BOOLEAN, .as.boolean = {.value = false},};
static struct JSON json_null  = {.type = JSON_NULL,};

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

bool json_is_none(struct JSON const * value)    { return value->type == JSON_NONE; }
bool json_is_object(struct JSON const * value)  { return value->type == JSON_OBJECT; }
bool json_is_array(struct JSON const * value)   { return value->type == JSON_ARRAY; }
bool json_is_string(struct JSON const * value)  { return value->type == JSON_STRING; }
bool json_is_number(struct JSON const * value)  { return value->type == JSON_NUMBER; }
bool json_is_boolean(struct JSON const * value) { return value->type == JSON_BOOLEAN; }
bool json_is_null(struct JSON const * value)    { return value->type == JSON_NULL; }

struct JSON const * json_object_get(struct JSON const * value, uint32_t key_id) {
	if (value->type != JSON_OBJECT) { return &json_none; }
	struct JSON_Object const * object = &value->as.object;
	void * result = hash_table_u32_get(&object->value, key_id);
	return (result != NULL) ? result : &json_null;
}

struct JSON const * json_array_at(struct JSON const * value, uint32_t index) {
	if (value->type != JSON_ARRAY) { return &json_none; }
	struct JSON_Array const * array = &value->as.array;
	void * result = array_any_at(&array->value, index);
	return (result != NULL) ? result : &json_null;
}

char const * json_as_string(struct JSON const * value, char const * default_value) {
	if (value->type != JSON_STRING) { return default_value; }
	struct JSON_String const * string = &value->as.string;
	return json_system_get_string_value(string->value);
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
	uint32_t const key_id = json_system_find_string_id(key);
	return json_as_string(json_object_get(value, key_id), default_value);
}

float json_get_number(struct JSON const * value, char const * key, float default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_as_number(json_object_get(value, key_id), default_value);
}

bool json_get_boolean(struct JSON const * value, char const * key, bool default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_as_boolean(json_object_get(value, key_id), default_value);
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
	while (parser->current.type != JSON_TOKEN_EOF) {
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

static void json_parser_synchronize_object(struct JSON_Parser * parser) {
	while (parser->current.type != JSON_TOKEN_EOF) {
		if (parser->previous.type == JSON_TOKEN_COMMA && parser->current.type == JSON_TOKEN_STRING) { break; }
		if (parser->current.type == JSON_TOKEN_RIGHT_BRACE) { break; }
		json_parser_consume(parser);
	}
	parser->panic = false;
}

static void json_parser_synchronize_array(struct JSON_Parser * parser) {
	while (parser->current.type != JSON_TOKEN_EOF) {
		if (parser->previous.type == JSON_TOKEN_COMMA) { break; }
		if (parser->current.type == JSON_TOKEN_RIGHT_SQUARE) { break; }
		json_parser_consume(parser);
	}
	parser->panic = false;
}

static void json_parser_do_value(struct JSON_Parser * parser, struct JSON * value);
static void json_parser_do_object(struct JSON_Parser * parser, struct JSON * value) {
	*value = (struct JSON){.type = JSON_OBJECT,};
	struct JSON_Object * object = &value->as.object;
	hash_table_u32_init(&object->value, sizeof(struct JSON));

	enum JSON_Token_Type const scope = JSON_TOKEN_RIGHT_BRACE;
	if (parser->current.type == scope) { json_parser_consume(parser); return; }

	while (parser->current.type != JSON_TOKEN_EOF) {
		// read
		struct JSON entry_key;
		json_parser_do_value(parser, &entry_key);
		if (entry_key.type != JSON_STRING) {
			json_parser_error_current(parser, "expected string");
			goto syncronization_point; // the label is that way vvvvv
		}

		if (!json_parser_match(parser, JSON_TOKEN_COLON)) {
			json_parser_error_previous(parser, "expected ':'");
			goto syncronization_point; // the label is that way vvvvv
		}

		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		if (entry_value.type == JSON_NONE) {
			goto syncronization_point; // the label is that way vvvvv
		}

		// add
		bool const is_new = hash_table_u32_set(&object->value, entry_key.as.string.value, &entry_value);
		if (!is_new) {
			logger_to_console("key duplicate: \"%s\"\n", json_system_get_string_value(entry_key.as.string.value));
			DEBUG_BREAK();
		}

		// finalize
		bool const is_comma = json_parser_match(parser, JSON_TOKEN_COMMA);
		if (json_parser_match(parser, scope)) { break; }
		if (is_comma) { continue; }
		json_parser_error_previous(parser, "expected ',' or '}'");

		// oh no, a `goto` label, think of the children!
		syncronization_point: // `goto` are this way ^^^^^;
		json_parser_synchronize_object(parser);
	}
}

static void json_parser_do_array(struct JSON_Parser * parser, struct JSON * value) {
	*value = (struct JSON){.type = JSON_ARRAY,};
	struct JSON_Array * array = &value->as.array;
	array_any_init(&array->value, sizeof(struct JSON));

	enum JSON_Token_Type const scope = JSON_TOKEN_RIGHT_SQUARE;
	if (parser->current.type == scope) { json_parser_consume(parser); return; }

	while (parser->current.type != JSON_TOKEN_EOF) {
		// read
		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		if (entry_value.type == JSON_NONE) {
			goto syncronization_point; // the label is that way vvvvv
		}

		// add
		array_any_push(&array->value, &entry_value);

		// finalize
		bool const is_comma = json_parser_match(parser, JSON_TOKEN_COMMA);
		if (json_parser_match(parser, scope)) { break; }
		if (is_comma) { continue; }
		json_parser_error_previous(parser, "expected ',' or ']'");

		// oh no, a `goto` label, think of the children!
		syncronization_point: // `goto` are this way ^^^^^;
		json_parser_synchronize_array(parser);
	}
}

static void json_parser_do_string(struct JSON_Parser * parser, struct JSON * value) {
	uint32_t const string_id = strings_add(&json_system.strings, parser->current.length - 2, parser->current.data + 1);
	*value = (struct JSON){
		.type = JSON_STRING,
		.as.string = {.value = string_id,},
	};
	json_parser_consume(parser);
}

static void json_parser_do_number(struct JSON_Parser * parser, struct JSON * value) {
	float const number = parse_float(parser->current.data);
	*value = (struct JSON){
		.type = JSON_NUMBER,
		.as.number = {.value = number,},
	};
	json_parser_consume(parser);
}

static void json_parser_do_value(struct JSON_Parser * parser, struct JSON * value) {
	switch (parser->current.type) {
		case JSON_TOKEN_STRING: json_parser_do_string(parser, value); return;
		case JSON_TOKEN_NUMBER: json_parser_do_number(parser, value); return;

		case JSON_TOKEN_TRUE:  *value = json_true;  json_parser_consume(parser); return;
		case JSON_TOKEN_FALSE: *value = json_false; json_parser_consume(parser); return;
		case JSON_TOKEN_NULL:  *value = json_null;  json_parser_consume(parser); return;

		default: break;
	}

	if (json_parser_match(parser, JSON_TOKEN_LEFT_BRACE)) {
		json_parser_do_object(parser, value); return;
	}

	if (json_parser_match(parser, JSON_TOKEN_LEFT_SQUARE)) {
		json_parser_do_array(parser, value); return;
	}

	json_parser_error_current(parser, "expected value");
	*value = json_none;
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
		*value = json_none;
		DEBUG_BREAK();
	}

	json_scanner_free(&parser.scanner);
}
