#if !defined(GAME_APPLICATION_INPUT)
#define GAME_APPLICATION_INPUT

#include "framework/common.h"
#include "framework/input_keys.h"

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
