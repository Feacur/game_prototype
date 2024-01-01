#if !defined(FRAMEWORK_system_assets_JSON)
#define FRAMEWORK_system_assets_JSON

#include "framework/containers/array.h"
#include "framework/containers/hashmap.h"

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
		struct Hashmap table;    // key `struct Handle` : `struct JSON`
		struct Array array;      // `struct JSON`
		struct Handle sh_string; // get `struct CString` via `system_strings_get`
		double number;
		bool boolean;
	} as;
};

struct JSON json_init(void);
void json_free(struct JSON * value);

// -- JSON get/at element
struct JSON const * json_get(struct JSON const * value, struct CString key);
struct JSON const * json_at(struct JSON const * value, uint32_t index);
uint32_t json_count(struct JSON const * value);

// -- JSON as data
struct CString json_as_string(struct JSON const * value);
double json_as_number(struct JSON const * value);
bool json_as_boolean(struct JSON const * value);

// -- JSON get data
struct CString json_get_string(struct JSON const * value, struct CString key);
double json_get_number(struct JSON const * value, struct CString key);
bool json_get_boolean(struct JSON const * value, struct CString key);

// -- JSON at data
struct CString json_at_string(struct JSON const * value, uint32_t index);
double json_at_number(struct JSON const * value, uint32_t index);
bool json_at_boolean(struct JSON const * value, uint32_t index);

// ----- ----- ----- ----- -----
//     parsing
// ----- ----- ----- ----- -----

struct JSON json_parse(struct CString text);

// ----- ----- ----- ----- -----
//     constants
// ----- ----- ----- ----- -----

extern struct JSON const c_json_true;
extern struct JSON const c_json_false;
extern struct JSON const c_json_null;
extern struct JSON const c_json_error;

#endif
