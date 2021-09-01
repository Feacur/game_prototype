#include "framework/logger.h"
#include "framework/parsing.h"

#include "framework/containers/array_byte.h"
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
static struct JSON const json_true  = {.type = JSON_BOOLEAN, .as.boolean = true,};
static struct JSON const json_false = {.type = JSON_BOOLEAN, .as.boolean = false,};
static struct JSON const json_null  = {.type = JSON_NULL,};
static struct JSON const json_error = {.type = JSON_ERROR,};

static void json_init_internal(char const * data, struct JSON * value);
void json_init(struct JSON * value, char const * data) {
	json_init_internal(data, value);
}

void json_free(struct JSON * value) {
	switch (value->type) {
		case JSON_OBJECT: {
			struct Hash_Table_U32 * table = &value->as.table;
			for (struct Hash_Table_U32_Iterator it = {0}; hash_table_u32_iterate(table, &it); /*empty*/) {
				json_free(it.value);
			}
			hash_table_u32_free(table);
		} break;

		case JSON_ARRAY: {
			struct Array_Any * array = &value->as.array;
			for (uint32_t i = 0; i < array->count; i++) {
				json_free(array_any_at(array, i));
			}
			array_any_free(array);
		} break;

		default: break;
	}
	value->type = JSON_NULL;
	// memset(value, 0, sizeof(*value));
}

// -- JSON complex
struct JSON const * json_object_get(struct JSON const * value, uint32_t key_id) {
	if (value->type != JSON_OBJECT) { return &json_error; }
	void * result = hash_table_u32_get(&value->as.table, key_id);
	return (result != NULL) ? result : &json_null;
}

struct JSON const * json_array_at(struct JSON const * value, uint32_t index) {
	if (value->type != JSON_ARRAY) { return &json_error; }
	void * result = array_any_at(&value->as.array, index);
	return (result != NULL) ? result : &json_null;
}

// -- JSON value to data
struct Hash_Table_U32 const * json_as_table(struct JSON const * value, struct Hash_Table_U32 const * default_value) {
	if (value->type != JSON_OBJECT) { return default_value; }
	return &value->as.table;
}

struct Array_Any const * json_as_array(struct JSON const * value, struct Array_Any const * default_value) {
	if (value->type != JSON_ARRAY) { return default_value; }
	return &value->as.array;
}

uint32_t json_as_string_id(struct JSON const * value, uint32_t default_value) {
	if (value->type != JSON_STRING) { return default_value; }
	return value->as.id;
}

char const * json_as_string(struct JSON const * value, char const * default_value) {
	if (value->type != JSON_STRING) { return default_value; }
	return json_system_get_string_value(value->as.id);
}

float json_as_number(struct JSON const * value, float default_value) {
	if (value->type != JSON_NUMBER) { return default_value; }
	return value->as.number;
}

bool json_as_boolean(struct JSON const * value, bool default_value) {
	if (value->type != JSON_BOOLEAN) { return default_value; }
	return value->as.boolean;
}

// -- JSON object get data
struct Hash_Table_U32 const * json_get_table(struct JSON const * value, uint32_t key_id, struct Hash_Table_U32 const * default_value) {
	return json_as_table(json_object_get(value, key_id), default_value);
}

struct Array_Any const * json_get_array(struct JSON const * value, uint32_t key_id, struct Array_Any const * default_value) {
	return json_as_array(json_object_get(value, key_id), default_value);
}

uint32_t json_get_string_id(struct JSON const * value, uint32_t key_id, uint32_t default_value) {
	return json_as_string_id(json_object_get(value, key_id), default_value);
}

char const * json_get_string(struct JSON const * value, uint32_t key_id, char const * default_value) {
	return json_as_string(json_object_get(value, key_id), default_value);
}

float json_get_number(struct JSON const * value, uint32_t key_id, float default_value) {
	return json_as_number(json_object_get(value, key_id), default_value);
}

bool json_get_boolean(struct JSON const * value, uint32_t key_id, bool default_value) {
	return json_as_boolean(json_object_get(value, key_id), default_value);
}

// -- JSON object find data
struct Hash_Table_U32 const * json_find_table(struct JSON const * value, char const * key, struct Hash_Table_U32 const * default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_get_table(value, key_id, default_value);
}

struct Array_Any const * json_find_array(struct JSON const * value, char const * key, struct Array_Any const * default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_get_array(value, key_id, default_value);
}

uint32_t json_find_string_id(struct JSON const * value, char const * key, uint32_t default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_get_string_id(value, key_id, default_value);
}

char const * json_find_string(struct JSON const * value, char const * key, char const * default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_get_string(value, key_id, default_value);
}

float json_find_number(struct JSON const * value, char const * key, float default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_get_number(value, key_id, default_value);
}

bool json_find_boolean(struct JSON const * value, char const * key, bool default_value) {
	uint32_t const key_id = json_system_find_string_id(key);
	return json_get_boolean(value, key_id, default_value);
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
	struct Hash_Table_U32 * table = &value->as.table;
	hash_table_u32_init(table, sizeof(struct JSON));

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
		if (entry_value.type == JSON_ERROR) {
			goto syncronization_point; // the label is that way vvvvv
		}

		// add
		bool const is_new = hash_table_u32_set(table, entry_key.as.id, &entry_value);
		if (!is_new) {
			logger_to_console("key duplicate: \"%s\"\n", json_system_get_string_value(entry_key.as.id));
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
	struct Array_Any * array = &value->as.array;
	array_any_init(array, sizeof(struct JSON));

	enum JSON_Token_Type const scope = JSON_TOKEN_RIGHT_SQUARE;
	if (parser->current.type == scope) { json_parser_consume(parser); return; }

	while (parser->current.type != JSON_TOKEN_EOF) {
		// read
		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		if (entry_value.type == JSON_ERROR) {
			goto syncronization_point; // the label is that way vvvvv
		}

		// add
		array_any_push(array, &entry_value);

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
	*value = (struct JSON){.type = JSON_STRING, .as.id = string_id,};
	json_parser_consume(parser);
}

static void json_parser_do_number(struct JSON_Parser * parser, struct JSON * value) {
	float const number = parse_float(parser->current.data);
	*value = (struct JSON){.type = JSON_NUMBER, .as.number = number,};
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
	*value = json_error;
}

static void json_init_internal(char const * data, struct JSON * value) {
	struct JSON_Parser parser = {0};
	json_scanner_init(&parser.scanner, data);
	json_parser_consume(&parser);

	json_parser_do_value(&parser, value);
	if (parser.current.type != JSON_TOKEN_EOF) {
		json_parser_error_current(&parser, "expected eof");
	}

	if (parser.error) {
		json_free(value);
		*value = json_error;
		DEBUG_BREAK();
	}

	json_scanner_free(&parser.scanner);
}
