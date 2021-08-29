#if !defined(GAME_ASSETS_JSON)
#define GAME_ASSETS_JSON

#include "framework/common.h"

struct JSON;

struct JSON * json_init(char const * data);
void json_free(struct JSON * value);

bool json_is_null(struct JSON const * value);
bool json_is_object(struct JSON const * value);
bool json_is_array(struct JSON const * value);
bool json_is_string(struct JSON const * value);
bool json_is_number(struct JSON const * value);
bool json_is_boolean(struct JSON const * value);

struct JSON const * json_object_get(struct JSON const * value, char const * key);
struct JSON const * json_array_at(struct JSON const * value, uint32_t index);

char const * json_as_string(struct JSON const * value, char const * default_value);
float json_as_number(struct JSON const * value, float default_value);
bool json_as_boolean(struct JSON const * value, bool default_value);

char const * json_get_string(struct JSON const * value, char const * key, char const * default_value);
float json_get_number(struct JSON const * value, char const * key, float default_value);
bool json_get_boolean(struct JSON const * value, char const * key, bool default_value);

#endif
