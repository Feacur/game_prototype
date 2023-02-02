#if !defined(PROTOTYPE_UI)
#define PROTOTYPE_UI

#include "application/components.h"

struct Asset_System;

void ui_init(void);
void ui_free(void);

void ui_start_frame(void);
void ui_end_frame(void);

void ui_set_transform(struct Transform_Rect transform_rect);
void ui_set_color(struct vec4 color);

void ui_set_shader(struct CString name);
void ui_set_image(struct CString name);
void ui_set_glyph_atlas(struct CString name);

void ui_quad(struct rect uv);
void ui_text(struct CString value, struct vec2 alignment, bool wrap, float size);

#endif
