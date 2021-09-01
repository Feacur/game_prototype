#if !defined(GAME_ASSETS_JSON)
#define GAME_ASSETS_JSON

#include "framework/containers/array_any.h"
#include "framework/containers/hash_table_u32.h"

// -- JSON system part
uint32_t json_system_add_string_id(char const * value);
uint32_t json_system_find_string_id(char const * value);
char const * json_system_get_string_value(uint32_t value);

// -- JSON value part
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

void json_init(struct JSON * value, char const * data);
void json_free(struct JSON * value);

// -- JSON complex
struct JSON const * json_object_get(struct JSON const * value, uint32_t key_id);
struct JSON const * json_array_at(struct JSON const * value, uint32_t index);

// -- JSON value as data
struct Hash_Table_U32 const * json_as_table(struct JSON const * value, struct Hash_Table_U32 const * default_value);
struct Array_Any const * json_as_array(struct JSON const * value, struct Array_Any const * default_value);
uint32_t json_as_string_id(struct JSON const * value, uint32_t default_value);
char const * json_as_string(struct JSON const * value, char const * default_value);
float json_as_number(struct JSON const * value, float default_value);
bool json_as_boolean(struct JSON const * value, bool default_value);

// -- JSON object get data
struct Hash_Table_U32 const * json_get_table(struct JSON const * value, uint32_t key, struct Hash_Table_U32 const * default_value);
struct Array_Any const * json_get_array(struct JSON const * value, uint32_t key, struct Array_Any const * default_value);
uint32_t json_get_string_id(struct JSON const * value, uint32_t key, uint32_t default_value);
char const * json_get_string(struct JSON const * value, uint32_t key, char const * default_value);
float json_get_number(struct JSON const * value, uint32_t key, float default_value);
bool json_get_boolean(struct JSON const * value, uint32_t key, bool default_value);

// -- JSON object find data
struct Hash_Table_U32 const * json_find_table(struct JSON const * value, char const * key, struct Hash_Table_U32 const * default_value);
struct Array_Any const * json_find_array(struct JSON const * value, char const * key, struct Array_Any const * default_value);
uint32_t json_find_string_id(struct JSON const * value, char const * key, uint32_t default_value);
char const * json_find_string(struct JSON const * value, char const * key, char const * default_value);
float json_find_number(struct JSON const * value, char const * key, float default_value);
bool json_find_boolean(struct JSON const * value, char const * key, bool default_value);

#endif
