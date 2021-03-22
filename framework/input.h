#if !defined(GAME_FRAMEWORK_INPUT)
#define GAME_FRAMEWORK_INPUT

#include "common.h"
#include "input_keys.h"

// @todo: recieve utf-8 input, too?

void input_reset_delta(void);
void input_update(void);

bool input_key(enum Key_Code key);
bool input_key_transition(enum Key_Code key, bool state);

void input_mouse_position_window(uint32_t * x, uint32_t * y);
void input_mouse_position_display(uint32_t * x, uint32_t * y);
void input_mouse_delta(int32_t * x, int32_t * y);
bool input_mouse(enum Mouse_Code key);
bool input_mouse_transition(enum Mouse_Code key, bool state);

#endif
