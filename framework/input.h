#if !defined(FRAMEWORK_INPUT)
#define FRAMEWORK_INPUT

#include "common.h"
#include "input_keys.h"

struct Array;

void input_reset_delta(void);
void input_update(void);

bool input_key(enum Key_Code key, enum Input_Type state);
void input_track_codepoints(bool state);
struct Array const * input_get_codepoints(void);

void input_mouse_position_window(uint32_t * x, uint32_t * y);
void input_mouse_position_display(uint32_t * x, uint32_t * y);
void input_mouse_delta(int32_t * x, int32_t * y);
bool input_mouse(enum Mouse_Code key, enum Input_Type state);

#endif
