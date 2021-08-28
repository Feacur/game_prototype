#if !defined(GAME_ASSETS_JSON)
#define GAME_ASSETS_JSON

#include "framework/common.h"

enum JSON_Value_Type {
	JSON_VALUE_NONE,
	JSON_VALUE_OBJECT,
	JSON_VALUE_ARRAY,
	JSON_VALUE_STRING,
	JSON_VALUE_NUMBER,
	JSON_VALUE_BOOLEAN,
};

struct JSON_Value_Object {
	uint8_t dummy;
};

struct JSON_Value_Array {
	uint8_t dummy;
};

struct JSON_Value_String {
	char const * value;
};

struct JSON_Value_Number {
	double value;
};

struct JSON_Value_Boolean {
	bool value;
};

struct JSON_Value {
	enum JSON_Value_Type type;
	union {
		struct JSON_Value_Object  object;
		struct JSON_Value_Array   array;
		struct JSON_Value_String  string;
		struct JSON_Value_Number  number;
		struct JSON_Value_Boolean boolean;
	} as;
};

struct JSON_Value json_read(char const * data);

#endif
