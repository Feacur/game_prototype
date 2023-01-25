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

struct CArray_Mut gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, uint32_t uniform_id, uint32_t offset) {
	for (uint32_t i = offset; i < uniforms->headers.count; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_any_at(&uniforms->headers, i);
		if (entry->id != uniform_id) { continue; }
		return (struct CArray_Mut){
			.data = (uint8_t *)uniforms->payload.data + entry->offset,
			.size = entry->size,
		};
	}
	return (struct CArray_Mut){0};
}

void gfx_uniforms_set(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct CArray value) {
	struct CArray_Mut field = gfx_uniforms_get(uniforms, uniform_id, 0);
	if (field.data == NULL) {
		struct CString const uniform_name = graphics_get_uniform_value(uniform_id);
		logger_to_console("creating uniform '%.*s'\n", uniform_name.length, uniform_name.data);
		gfx_uniforms_push(uniforms, uniform_id, value);
		return;
	}
	if (field.size >= value.size) {
		common_memcpy(field.data, value.data, value.size);
		return;
	}
	struct CString const uniform_name = graphics_get_uniform_value(uniform_id);
	logger_to_console("data is too large for uniform '%.*s'\n", uniform_name.length, uniform_name.data);
}

void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, uint32_t uniform_id, struct CArray value) {
	array_any_push_many(&uniforms->headers, 1, &(struct Gfx_Uniforms_Entry){
		.id = uniform_id,
		.size = (uint32_t)value.size,
		.offset = (uint32_t)uniforms->payload.size,
	});
	buffer_push_many(&uniforms->payload, value.size, value.data);
}

// ----- ----- ----- ----- -----
//     material
// ----- ----- ----- ----- -----

struct Gfx_Material gfx_material_init(void) {
	return (struct Gfx_Material){
		.uniforms = gfx_uniforms_init(),
	};
}

void gfx_material_free(struct Gfx_Material * material) {
	gfx_uniforms_free(&material->uniforms);
	common_memset(material, 0, sizeof(*material));
}

void gfx_material_set_shader(struct Gfx_Material * material, struct Handle gpu_handle) {
	struct CString const property_prefix = S_("p_");

	gfx_uniforms_clear(&material->uniforms);

	material->gpu_program_handle = gpu_handle;
	struct Hash_Table_U32 const * uniforms = gpu_program_get_uniforms(gpu_handle);
	if (uniforms == NULL) { return; }

	uint32_t payload_bytes = 0, properties_count = 0;
	FOR_HASH_TABLE_U32(uniforms, it) {
		struct CString const uniform_name = graphics_get_uniform_value(it.key_hash);
		if (!cstring_starts(uniform_name, property_prefix)) { continue; }

		struct Gpu_Uniform const * uniform = it.value;
		payload_bytes += data_type_get_size(uniform->type) * uniform->array_size;
		properties_count++;
	}

	array_any_resize(&material->uniforms.headers, properties_count);
	buffer_resize(&material->uniforms.payload, payload_bytes);
	common_memset(material->uniforms.payload.data, 0, payload_bytes);

	FOR_HASH_TABLE_U32(uniforms, it) {
		struct CString const uniform_name = graphics_get_uniform_value(it.key_hash);
		if (!cstring_starts(uniform_name, property_prefix)) { continue; }

		struct Gpu_Uniform const * uniform = it.value;
		gfx_uniforms_push(&material->uniforms, it.key_hash, (struct CArray){
			.size = data_type_get_size(uniform->type) * uniform->array_size,
		});
	}
}
