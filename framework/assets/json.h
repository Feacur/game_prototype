#if !defined(GAME_ASSETS_JSON)
#define GAME_ASSETS_JSON

#include "framework/containers/array_any.h"
#include "framework/containers/hash_table_u32.h"

struct Strings;

enum JSON_Type {
	JSON_NULL,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_STRING,
	JSON_NUMBER,
	JSON_BOOLEAN,
	JSON_ERROR,
};

struct JSON {
	enum JSON_Type type;
	union {
		struct Hash_Table_U32 table;
		struct Array_Any array;
		uint32_t id;
		float number;
		bool boolean;
	} as;
};

void json_init(struct JSON * value, struct Strings * strings, char const * data);
void json_free(struct JSON * value);

// -- JSON try get element
struct JSON const * json_object_get(struct JSON const * value, struct Strings const * strings, char const * key);
struct JSON const * json_array_at(struct JSON const * value, uint32_t index);
uint32_t json_array_count(struct JSON const * value);

// -- JSON try get data
uint32_t json_as_string_id(struct JSON const * value);
char const * json_as_string(struct JSON const * value, struct Strings const * strings, char const * default_value);
float json_as_number(struct JSON const * value, float default_value);
bool json_as_boolean(struct JSON const * value, bool default_value);

//
extern struct JSON const json_true;
extern struct JSON const json_false;
extern struct JSON const json_null;
extern struct JSON const json_error;

#endif
