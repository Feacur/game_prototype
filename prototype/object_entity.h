#if !defined(PROTOTYPE_OBJECT_ENTITY)
#define PROTOTYPE_OBJECT_ENTITY

#include "framework/systems/asset_handle.h"
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
	struct Asset_Handle asset_handle;
};

struct Entity_Quad {
	struct rect view;
	uint32_t texture_uniform;
	enum Entity_Quad_Mode mode;
};

struct Entity_Text {
	struct Asset_Handle font_asset_handle;
	struct Asset_Handle message_asset_handle;
	float size;
	struct vec2 alignment;
};

struct Entity {
	uint32_t camera;
	struct Asset_Handle material_asset_handle;
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
	//
	struct rect cached_rect;
};

//

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Gfx_Material const * material,
	uint32_t parent_size_x, uint32_t parent_size_y
);

#endif
