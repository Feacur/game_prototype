#include "framework/logger.h"
#include "framework/parsing.h"

#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"

#include "json_scanner.h"

// @note: JSON is based off of rfc8259

//
#include "json.h"

struct JSON const json_true  = {.type = JSON_BOOLEAN, .as.boolean = true,};
struct JSON const json_false = {.type = JSON_BOOLEAN, .as.boolean = false,};
struct JSON const json_null  = {.type = JSON_NULL,};
struct JSON const json_error = {.type = JSON_ERROR,};

static void json_init_internal(struct JSON * value, struct Strings * strings, char const * data);
void json_init(struct JSON * value, struct Strings * strings, char const * data) {
	json_init_internal(value, strings, data);
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
	// common_memset(value, 0, sizeof(*value));
}

// -- JSON find id
uint32_t json_find_id(struct JSON const * value, struct CString text) {
	if (value->strings == NULL) { DEBUG_BREAK(); return INDEX_EMPTY; }
	return strings_find(value->strings, text);
}

// -- JSON get/at element
struct JSON const * json_get(struct JSON const * value, struct CString key) {
	if (value->type != JSON_OBJECT) { DEBUG_BREAK(); return &json_error; }
	if (value->strings == NULL) { DEBUG_BREAK(); return &json_error; }
	uint32_t const key_id = strings_find(value->strings, key);
	if (key_id == INDEX_EMPTY) { return &json_null; }
	void const * result = hash_table_u32_get(&value->as.table, key_id);
	return (result != NULL) ? result : &json_null;
}

struct JSON const * json_at(struct JSON const * value, uint32_t index) {
	if (value->type != JSON_ARRAY) { DEBUG_BREAK(); return &json_error; }
	void * result = array_any_at(&value->as.array, index);
	return (result != NULL) ? result : &json_null;
}

uint32_t json_count(struct JSON const * value) {
	if (value->type != JSON_ARRAY) { return 0; }
	return value->as.array.count;
}

// -- JSON as data
uint32_t json_as_id(struct JSON const * value) {
	if (value->type != JSON_STRING) { return INDEX_EMPTY; }
	return value->as.id;
}

struct CString json_as_string(struct JSON const * value, struct CString default_value) {
	if (value->type != JSON_STRING) { return default_value; }
	return strings_get(value->strings, value->as.id);
}

float json_as_number(struct JSON const * value, float default_value) {
	if (value->type != JSON_NUMBER) { return default_value; }
	return value->as.number;
}

bool json_as_boolean(struct JSON const * value, bool default_value) {
	if (value->type != JSON_BOOLEAN) { return default_value; }
	return value->as.boolean;
}

// -- JSON get data
uint32_t json_get_id(struct JSON const * value, struct CString key) {
	return json_as_id(json_get(value, key));
}

struct CString json_get_string(struct JSON const * value, struct CString key, struct CString default_value) {
	return json_as_string(json_get(value, key), default_value);
}

float json_get_number(struct JSON const * value, struct CString key, float default_value) {
	return json_as_number(json_get(value, key), default_value);
}

bool json_get_boolean(struct JSON const * value, struct CString key, bool default_value) {
	return json_as_boolean(json_get(value, key), default_value);
}

// -- JSON at data
uint32_t json_at_id(struct JSON const * value, uint32_t index) {
	return json_as_id(json_at(value, index));
}

struct CString json_at_string(struct JSON const * value, uint32_t index, struct CString default_value) {
	return json_as_string(json_at(value, index), default_value);
}

float json_at_number(struct JSON const * value, uint32_t index, float default_value) {
	return json_as_number(json_at(value, index), default_value);
}

bool json_at_boolean(struct JSON const * value, uint32_t index, bool default_value) {
	return json_as_boolean(json_at(value, index), default_value);
}

//

struct JSON_Parser {
	struct Strings * strings;
	struct JSON_Scanner scanner;
	struct JSON_Token previous, current;
	bool error, panic;
};

static void json_parser_error_at(struct JSON_Parser * parser, struct JSON_Token const * token, char const * message) {
	parser->error = true;

	if (parser->panic) { return; }
	parser->panic = true;

	static char const * c_json_token_names[] = {
		[JSON_TOKEN_ERROR_IDENTIFIER] = "identifier",
		[JSON_TOKEN_ERROR_UNKNOWN_CHARACTER] = "unknown character",
		[JSON_TOKEN_ERROR_UNTERMINATED_STRING] = "unterminated string",
		[JSON_TOKEN_EOF] = "eof",
	};

	char const * const reason = c_json_token_names[token->type];

	logger_to_console("json");
	logger_to_console(" [line: %u]", token->line + 1);
	logger_to_console(" [context: '%.*s']", token->text.length, token->text.data);
	if (reason != NULL) { logger_to_console(" [%s]", reason); }
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
		switch (parser->current.type) {
			case JSON_TOKEN_COMMENT:
				continue;

			case JSON_TOKEN_ERROR_IDENTIFIER:
			case JSON_TOKEN_ERROR_UNKNOWN_CHARACTER:
			case JSON_TOKEN_ERROR_UNTERMINATED_STRING:
				json_parser_error_current(parser, "scanner error");
				continue;

			default: break;
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
		json_parser_consume(parser);
		if (parser->current.type == JSON_TOKEN_RIGHT_BRACE) { break; }
		if (parser->previous.type == JSON_TOKEN_COMMA && parser->current.type == JSON_TOKEN_STRING) { break; }
	}
	parser->panic = false;
}

static void json_parser_synchronize_array(struct JSON_Parser * parser) {
	while (parser->current.type != JSON_TOKEN_EOF) {
		json_parser_consume(parser);
		if (parser->current.type == JSON_TOKEN_RIGHT_SQUARE) { break; }
		if (parser->previous.type == JSON_TOKEN_COMMA) { break; }
	}
	parser->panic = false;
}

static void json_parser_do_value(struct JSON_Parser * parser, struct JSON * value);
static void json_parser_do_object(struct JSON_Parser * parser, struct JSON * value) {
	*value = (struct JSON){.strings = parser->strings, .type = JSON_OBJECT,};
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
			goto syncronization_point;
		}

		if (!json_parser_match(parser, JSON_TOKEN_COLON)) {
			json_parser_error_previous(parser, "expected ':'");
			goto syncronization_point;
		}

		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		if (entry_value.type == JSON_ERROR) {
			goto syncronization_point;
		}

		// add
		bool const is_new = hash_table_u32_set(table, entry_key.as.id, &entry_value);
		if (!is_new) {
			struct CString const key = strings_get(parser->strings, entry_key.as.id);
			logger_to_console("key duplicate: \"%.*s\"\n", key.length, key.data);
			DEBUG_BREAK();
		}

		// finalize
		bool const is_comma = json_parser_match(parser, JSON_TOKEN_COMMA);
		if (json_parser_match(parser, scope)) { break; }
		if (is_comma) { continue; }
		json_parser_error_previous(parser, "expected ',' or '}'");

		syncronization_point:
		json_parser_synchronize_object(parser);
	}
}

static void json_parser_do_array(struct JSON_Parser * parser, struct JSON * value) {
	*value = (struct JSON){.strings = parser->strings, .type = JSON_ARRAY,};
	struct Array_Any * array = &value->as.array;
	array_any_init(array, sizeof(struct JSON));

	enum JSON_Token_Type const scope = JSON_TOKEN_RIGHT_SQUARE;
	if (parser->current.type == scope) { json_parser_consume(parser); return; }

	while (parser->current.type != JSON_TOKEN_EOF) {
		// read
		struct JSON entry_value;
		json_parser_do_value(parser, &entry_value);
		if (entry_value.type == JSON_ERROR) {
			goto syncronization_point;
		}

		// add
		array_any_push(array, &entry_value);

		// finalize
		bool const is_comma = json_parser_match(parser, JSON_TOKEN_COMMA);
		if (json_parser_match(parser, scope)) { break; }
		if (is_comma) { continue; }
		json_parser_error_previous(parser, "expected ',' or ']'");

		syncronization_point:
		json_parser_synchronize_array(parser);
	}
}

static void json_parser_do_string(struct JSON_Parser * parser, struct JSON * value) {
	struct CString const cstring = (struct CString){
		.length = parser->current.text.length - 2,
		.data = parser->current.text.data + 1,
	};
	uint32_t const string_id = strings_add(parser->strings, cstring);
	*value = (struct JSON){.strings = parser->strings, .type = JSON_STRING, .as.id = string_id,};
	json_parser_consume(parser);
}

static void json_parser_do_number(struct JSON_Parser * parser, struct JSON * value) {
	float const number = parse_float(parser->current.text.data);
	*value = (struct JSON){.strings = parser->strings, .type = JSON_NUMBER, .as.number = number,};
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

static void json_init_internal(struct JSON * value, struct Strings * strings, char const * data) {
	if (strings == NULL) { *value = json_error; return; }
	if (data == NULL) { *value = json_error; return; }

	struct JSON_Parser parser = {
		.strings = strings,
	};
	json_scanner_init(&parser.scanner, data);
	json_parser_consume(&parser);

	if (parser.current.type == JSON_TOKEN_EOF) {
		*value = json_null;
		goto finalize;
	}

	json_parser_do_value(&parser, value);
	if (parser.current.type != JSON_TOKEN_EOF) {
		json_parser_error_current(&parser, "expected eof");
	}

	if (parser.error) {
		json_free(value);
		*value = json_error;
		DEBUG_BREAK();
	}

	finalize:
	json_scanner_free(&parser.scanner);
}
