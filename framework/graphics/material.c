#include "framework/logger.h"
#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"

#include <malloc.h>

//
#include "material.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

void gfx_uniforms_init(struct Gfx_Uniforms * uniforms) {
	array_any_init(&uniforms->headers, sizeof(struct Gfx_Uniforms_Entry));
	buffer_init(&uniforms->payload);
}

void gfx_uniforms_free(struct Gfx_Uniforms * uniforms) {
	array_any_free(&uniforms->headers);
	buffer_free(&uniforms->payload);
}

void gfx_uniforms_clear(struct Gfx_Uniforms * uniforms) {
	array_any_clear(&uniforms->headers);
	buffer_clear(&uniforms->payload);
}

struct Gfx_Uniform_Out gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, uint32_t uniform_id) {
	for (uint32_t i = 0; i < uniforms->headers.count; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_any_at(&uniforms->headers, i);
		if (entry->id != uniform_id) { continue; }
		return (struct Gfx_Uniform_Out){
			.size = entry->size,
			.data = uniforms->payload.data + entry->offset,
		};
	}
	logger_to_console("material doesn't have such a property\n"); DEBUG_BREAK();
	return (struct Gfx_Uniform_Out){0};
}

void gfx_uniforms_set(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct Gfx_Uniform_In value) {
	struct Gfx_Uniform_Out field = gfx_uniforms_get(uniforms, uniform_id);
	if (field.size >= value.size) {
		common_memcpy(field.data, value.data, value.size);
		return;
	}
	logger_to_console("data is too large\n"); DEBUG_BREAK();
}

void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct Gfx_Uniform_In value) {
	array_any_push(&uniforms->headers, &(struct Gfx_Uniforms_Entry){
		.id = uniform_id,
		.size = value.size,
		.offset = (uint32_t)uniforms->payload.count,
		// .type = uniform->type,
		// .array_size = uniform->array_size,
	});
	buffer_push_many(&uniforms->payload, value.size, value.data);
}

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

void gfx_material_init(
	struct Gfx_Material * material,
	struct Ref gpu_program_ref,
	struct Blend_Mode const * blend_mode,
	struct Depth_Mode const * depth_mode
) {
	gfx_uniforms_init(&material->uniforms);

	material->gpu_program_ref = gpu_program_ref;
	material->blend_mode = (blend_mode != NULL) ? *blend_mode : c_blend_mode_opaque;
	material->depth_mode = (blend_mode != NULL) ? *depth_mode : (struct Depth_Mode){.enabled = true, .mask = true};

	uint32_t uniforms_count;
	struct Gpu_Program_Field const * uniforms;
	gpu_program_get_uniforms(gpu_program_ref, &uniforms_count, &uniforms);

	uint32_t payload_bytes = 0, properties_count = 0;
	for (uint32_t i = 0; i < uniforms_count; i++) {
		struct Gpu_Program_Field const * uniform = uniforms + i;
		if (!uniform->is_property) { continue; }
		payload_bytes += data_type_get_size(uniform->type) * uniform->array_size;
		properties_count++;
	}

	array_any_resize(&material->uniforms.headers, sizeof(struct Gfx_Uniforms_Entry) * properties_count);
	buffer_resize(&material->uniforms.payload, payload_bytes);

	for (uint32_t i = 0; i < uniforms_count; i++) {
		struct Gpu_Program_Field const * uniform = uniforms + i;
		if (!uniform->is_property) { continue; }
		gfx_uniforms_push(&material->uniforms, uniform->id, (struct Gfx_Uniform_In){
			.size = data_type_get_size(uniform->type) * uniform->array_size,
		});
	}

	common_memset(material->uniforms.payload.data, 0, payload_bytes);
}

void gfx_material_free(struct Gfx_Material * material) {
	gfx_uniforms_free(&material->uniforms);
	common_memset(material, 0, sizeof(*material));
}
