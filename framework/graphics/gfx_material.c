#include "framework/containers/hashmap.h"
#include "framework/graphics/gfx_types.h"
#include "framework/graphics/objects.h"
#include "framework/systems/string_system.h"

//
#include "gfx_material.h"

// ----- ----- ----- ----- -----
//     uniforms
// ----- ----- ----- ----- -----

struct Gfx_Uniforms gfx_uniforms_init(void) {
	return (struct Gfx_Uniforms){
		.headers = array_init(sizeof(struct Gfx_Uniforms_Entry)),
		.payload = buffer_init(NULL),
	};
}

void gfx_uniforms_free(struct Gfx_Uniforms * uniforms) {
	array_free(&uniforms->headers);
	buffer_free(&uniforms->payload);
	// common_memset(uniforms, 0, sizeof(*uniforms));
}

void gfx_uniforms_clear(struct Gfx_Uniforms * uniforms) {
	array_clear(&uniforms->headers);
	buffer_clear(&uniforms->payload);
}

struct CArray_Mut gfx_uniforms_get(struct Gfx_Uniforms const * uniforms, struct CString name, uint32_t offset) {
	uint32_t const id = string_system_find(name);
	return gfx_uniforms_id_get(uniforms, id, offset);
}

void gfx_uniforms_push(struct Gfx_Uniforms * uniforms, struct CString name, struct CArray value) {
	uint32_t const id = string_system_add(name);
	gfx_uniforms_id_push(uniforms, id, value);
}

struct CArray_Mut gfx_uniforms_id_get(struct Gfx_Uniforms const * uniforms, uint32_t id, uint32_t offset) {
	if (id == 0) { goto null; }
	for (uint32_t i = offset; i < uniforms->headers.count; i++) {
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, i);
		if (entry->id != id) { continue; }
		return (struct CArray_Mut){
			.data = (uint8_t *)uniforms->payload.data + entry->offset,
			.size = entry->size,
		};
	}
	null: return (struct CArray_Mut){0};
}

void gfx_uniforms_id_push(struct Gfx_Uniforms * uniforms, uint32_t id, struct CArray value) {
	if (id == 0) { return; }
	array_push_many(&uniforms->headers, 1, &(struct Gfx_Uniforms_Entry){
		.id = id,
		.size = (uint32_t)value.size,
		.offset = (uint32_t)uniforms->payload.size,
	});
	buffer_push_many(&uniforms->payload, value.size, value.data);
}

bool gfx_uniforms_iterate(struct Gfx_Uniforms const * uniforms, struct Gfx_Uniforms_Iterator * iterator) {
	while (iterator->next < uniforms->headers.count) {
		uint32_t const index = iterator->next++;
		iterator->current = index;
		//
		struct Gfx_Uniforms_Entry const * entry = array_at(&uniforms->headers, index);
		iterator->id = entry->id;
		iterator->size = entry->size;
		iterator->value = (uint8_t *)uniforms->payload.data + entry->offset;
		return true;
	}
	return false;
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

void gfx_material_set_shader(struct Gfx_Material * material, struct Handle gh_program) {
	struct CString const property_prefix = S_("p_");

	material->gh_program = gh_program;
	struct Hashmap const * uniforms = gpu_program_get_uniforms(gh_program);
	if (uniforms == NULL) { return; }

	uint32_t payload_bytes = 0, properties_count = 0;
	FOR_HASHMAP(uniforms, it) {
		struct CString const uniform_name = string_system_get(it.hash);
		if (!cstring_starts(uniform_name, property_prefix)) { continue; }

		struct Gpu_Uniform const * uniform = it.value;
		payload_bytes += data_type_get_size(uniform->type) * uniform->array_size;
		properties_count++;
	}

	gfx_uniforms_clear(&material->uniforms);
	array_resize(&material->uniforms.headers, properties_count);
	buffer_resize(&material->uniforms.payload, payload_bytes);

	FOR_HASHMAP(uniforms, it) {
		struct CString const uniform_name = string_system_get(it.hash);
		if (!cstring_starts(uniform_name, property_prefix)) { continue; }

		struct Gpu_Uniform const * uniform = it.value;
		gfx_uniforms_id_push(&material->uniforms, it.hash, (struct CArray){
			.size = data_type_get_size(uniform->type) * uniform->array_size,
		});
	}
	common_memset(material->uniforms.payload.data, 0, payload_bytes);
}
