#include "framework/logger.h"
#include "framework/containers/hash_table_u32.h"
#include "framework/graphics/types.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"

#include <malloc.h>

//
#include "material.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

struct Gfx_Uniforms gfx_uniforms_init(void) {
	return (struct Gfx_Uniforms){
		.headers = array_any_init(sizeof(struct Gfx_Uniforms_Entry)),
		.payload = buffer_init(NULL),
	};
}

void gfx_uniforms_free(struct Gfx_Uniforms * uniforms) {
	array_any_free(&uniforms->headers);
	buffer_free(&uniforms->payload);
	// common_memset(uniforms, 0, sizeof(*uniforms));
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
			.data = (uint8_t *)uniforms->payload.data + entry->offset,
		};
	}
	struct CString const uniform_name = graphics_get_uniform_value(uniform_id);
	logger_to_console("material doesn't have uniform '%.*s'\n", uniform_name.length, uniform_name.data);
	return (struct Gfx_Uniform_Out){0};
}

void gfx_uniforms_set(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct Gfx_Uniform_In value) {
	struct Gfx_Uniform_Out field = gfx_uniforms_get(uniforms, uniform_id);
	if (field.size >= value.size) {
		common_memcpy(field.data, value.data, value.size);
		return;
	}
	struct CString const uniform_name = graphics_get_uniform_value(uniform_id);
	logger_to_console("data is too large for uniform '%.*s'\n", uniform_name.length, uniform_name.data);
}

void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct Gfx_Uniform_In value) {
	array_any_push_many(&uniforms->headers, 1, &(struct Gfx_Uniforms_Entry){
		.id = uniform_id,
		.size = value.size,
		.offset = (uint32_t)uniforms->payload.count,
	});
	buffer_push_many(&uniforms->payload, value.size, value.data);
}

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

struct Gfx_Material gfx_material_init(
	struct Ref gpu_program_ref,
	enum Blend_Mode blend_mode,
	enum Depth_Mode depth_mode
) {
	struct CString const property_prefix = S_("p_");

	struct Gfx_Material material = {
		.uniforms = gfx_uniforms_init(),
		.gpu_program_ref = gpu_program_ref,
		.blend_mode = blend_mode,
		.depth_mode = depth_mode,
	};

	struct Hash_Table_U32 const * uniforms = gpu_program_get_uniforms(gpu_program_ref);
	if (uniforms == NULL) { goto fail; }

	uint32_t payload_bytes = 0, properties_count = 0;
	FOR_HASH_TABLE_U32(uniforms, it) {
		struct CString const uniform_name = graphics_get_uniform_value(it.key_hash);
		if (!cstring_starts(uniform_name, property_prefix)) { continue; }

		struct Gpu_Uniform const * uniform = it.value;
		payload_bytes += data_type_get_size(uniform->type) * uniform->array_size;
		properties_count++;
	}

	array_any_resize(&material.uniforms.headers, sizeof(struct Gfx_Uniforms_Entry) * properties_count);
	buffer_resize(&material.uniforms.payload, payload_bytes);
	common_memset(material.uniforms.payload.data, 0, payload_bytes);

	FOR_HASH_TABLE_U32(uniforms, it) {
		struct CString const uniform_name = graphics_get_uniform_value(it.key_hash);
		if (!cstring_starts(uniform_name, property_prefix)) { continue; }

		struct Gpu_Uniform const * uniform = it.value;
		gfx_uniforms_push(&material.uniforms, it.key_hash, (struct Gfx_Uniform_In){
			.size = data_type_get_size(uniform->type) * uniform->array_size,
		});
	}

	finalize:
	return material;

	// process errors
	fail: logger_to_console("failed to initialize material\n");
	goto finalize;
}

void gfx_material_free(struct Gfx_Material * material) {
	gfx_uniforms_free(&material->uniforms);
	common_memset(material, 0, sizeof(*material));
}
