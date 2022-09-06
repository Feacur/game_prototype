#if !defined(PROTOTYPE_UI)
#define PROTOTYPE_UI

#include "framework/vector_types.h"

struct Asset_System;

void ui_init(struct CString shader_name);
void ui_free(void);

void ui_start_frame(void);
void ui_end_frame(void);

void ui_set_font(struct CString name);
void ui_text(struct rect rect, struct CString value, struct vec2 alignment, bool wrap, float size);

#endif
