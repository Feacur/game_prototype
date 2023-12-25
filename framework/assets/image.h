#if !defined(FRAMEWORK_ASSETS_IMAGE)
#define FRAMEWORK_ASSETS_IMAGE

// @note: image layout
// +----------+
// |image  1,1|
// |          |
// |0,0       |
// +----------+

#include "framework/common.h"
#include "framework/maths_types.h"
#include "framework/graphics/gfx_types.h"

struct Buffer;

struct Image {
	uint32_t capacity;
	struct uvec2 size;
	void * data;
	struct Texture_Format format;
	struct Texture_Settings settings;
};

struct Image image_init(struct Buffer const * buffer);
void image_free(struct Image * image);

void image_ensure(struct Image * image, struct uvec2 size);

#endif
