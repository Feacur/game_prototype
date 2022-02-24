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
	// @note: the same `strings` pointer is meant to be shared throughout the entire JSON tree
	struct Strings const * strings;
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

// -- JSON find id
uint32_t json_find_id(struct JSON const * value, struct CString text);

// -- JSON get/at element
struct JSON const * json_get(struct JSON const * value, struct CString key);
struct JSON const * json_at(struct JSON const * value, uint32_t index);
uint32_t json_count(struct JSON const * value);

// -- JSON as data
uint32_t json_as_id(struct JSON const * value);
struct CString json_as_string(struct JSON const * value, struct CString default_value);
float json_as_number(struct JSON const * value, float default_value);
bool json_as_boolean(struct JSON const * value, bool default_value);

// -- JSON get data
uint32_t json_get_id(struct JSON const * value, struct CString key);
struct CString json_get_string(struct JSON const * value, struct CString key, struct CString default_value);
float json_get_number(struct JSON const * value, struct CString key, float default_value);
bool json_get_boolean(struct JSON const * value, struct CString key, bool default_value);

// -- JSON at data
uint32_t json_at_id(struct JSON const * value, uint32_t index);
struct CString json_at_string(struct JSON const * value, uint32_t index, struct CString default_value);
float json_at_number(struct JSON const * value, uint32_t index, float default_value);
bool json_at_boolean(struct JSON const * value, uint32_t index, bool default_value);

//
extern struct JSON const c_json_true;
extern struct JSON const c_json_false;
extern struct JSON const c_json_null;
extern struct JSON const c_json_error;

#endif
