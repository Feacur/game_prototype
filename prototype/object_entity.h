#if !defined(PROTOTYPE_OBJECT_ENTITY)
#define PROTOTYPE_OBJECT_ENTITY

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
	struct Handle ah_mesh;
};

struct Entity_Quad {
	struct rect view;
	uint32_t uniform_id;
	enum Entity_Quad_Mode mode;
};

struct Entity_Text {
	struct Handle ah_font;
	struct Handle ah_text;
	float size;
	struct vec2 alignment;
};

struct Entity {
	uint32_t camera;
	struct Handle ah_material;
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

struct Entity entity_init(void);
void entity_free(struct Entity * entity);

struct uvec2 entity_get_content_size(
	struct Entity const * entity, struct Handle material_handle,
	uint32_t parent_size_x, uint32_t parent_size_y
);

#endif
