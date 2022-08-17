#if !defined(PROTOTYPE_OBJECT_ENTITY)
#define PROTOTYPE_OBJECT_ENTITY

#include "framework/systems/asset_ref.h"
#include "framework/graphics/material.h"
#include "framework/graphics/types.h"

#include "application/components.h"

#include "components.h"

enum Entity_Type {
	ENTITY_TYPE_NONE,
	ENTITY_TYPE_MESH,
	ENTITY_TYPE_QUAD_2D,
	ENTITY_TYPE_TEXT_2D,
};

enum Entity_Quad_Mode {
	ENTITY_QUAD_MODE_NONE,
	ENTITY_QUAD_MODE_FIT,
	ENTITY_QUAD_MODE_SIZE,
};

enum Entity_Rotation_Mode {
	ENTITY_ROTATION_MODE_NONE,
	ENTITY_ROTATION_MODE_X,
	ENTITY_ROTATION_MODE_Y,
	ENTITY_ROTATION_MODE_Z,
};

struct Entity_Mesh {
	struct Asset_Ref mesh;
};

struct Entity_Quad {
	uint32_t texture_uniform;
	enum Entity_Quad_Mode mode;
};

struct Entity_Text {
	struct Asset_Ref font;
	struct Asset_Ref message;
	uint32_t visible_length;
	float size;
};

struct Entity {
	uint32_t camera;
	struct Asset_Ref material;
	//
	struct Transform_3D transform;
	struct Transform_Rect rect; // @todo: make it completely optional?
	enum Entity_Rotation_Mode rotation_mode;
	//
	enum Entity_Type type;
	union {
		struct Entity_Mesh mesh;
		struct Entity_Quad quad;
		struct Entity_Text text;
	} as;
};

//

bool entity_get_is_screen_space(struct Entity const * entity);
bool entity_get_is_batched(struct Entity const * entity);

void entity_get_rect(
	struct Entity const * entity,
	uint32_t viewport_size_x, uint32_t viewport_size_y,
	struct vec2 * min, struct vec2 * max, struct vec2 * pivot
);

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Gfx_Material const * material,
	uint32_t viewport_size_x, uint32_t viewport_size_y
);

#endif
