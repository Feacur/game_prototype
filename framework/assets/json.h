#if !defined(GAME_ASSETS_JSON)
#define GAME_ASSETS_JSON

#include "framework/common.h"

enum JSON_Value_Type {
	JSON_VALUE_NONE,
	JSON_VALUE_OBJECT,
	JSON_VALUE_ARRAY,
	JSON_VALUE_STRING,
	JSON_VALUE_NUMBER,
};

struct JSON_Value_Object {
	uint8_t dummy;
};

struct JSON_Value_Array {
	uint8_t dummy;
};

struct JSON_Value_String {
	uint8_t dummy;
};

struct JSON_Value_Number {
	uint8_t dummy;
};

struct JSON_Value {
	enum JSON_Value_Type type;
	union {
		struct JSON_Value_Object object;
		struct JSON_Value_Array  array;
		struct JSON_Value_String string;
		struct JSON_Value_Number number;
	} as;
};

#endif
