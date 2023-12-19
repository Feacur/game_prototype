#include "framework/formatter.h"

#include "framework/platform/file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/array.h"
#include "framework/containers/array.h"
#include "framework/systems/memory_system.h"

#include "framework/graphics/gfx_types.h"

#include "internal/wfobj.h"


//
#include "mesh.h"

static struct Mesh mesh_init_wfobj(struct Buffer const * source);

struct Mesh mesh_init(struct Buffer const * source) {
	return mesh_init_wfobj(source);
}

void mesh_free(struct Mesh * mesh) {
	FOR_ARRAY(&mesh->buffers, it) {
		struct Mesh_Buffer * mesh_buffer = it.value;
		buffer_free(&mesh_buffer->buffer);
	}
	array_free(&mesh->buffers);
	common_memset(mesh, 0, sizeof(*mesh));
}

//

static struct Mesh mesh_init_wfobj(struct Buffer const * source) {
	struct WFObj wfobj = wfobj_parse((struct CString){
		.length = (uint32_t)source->size,
		.data = source->data,
	});

	// repack
	struct Array vertices = array_init(sizeof(float));
	struct Array indices  = array_init(sizeof(uint32_t));

	uint32_t attributes_count = 0;
	struct Mesh_Attributes attributes = {0};
	if (wfobj.positions.count > 0) { attributes.data[attributes_count++] = SHADER_ATTRIBUTE_POSITION; attributes.data[attributes_count++] = 3; }
	if (wfobj.texcoords.count > 0) { attributes.data[attributes_count++] = SHADER_ATTRIBUTE_TEXCOORD; attributes.data[attributes_count++] = 2; }
	if (wfobj.normals.count   > 0) { attributes.data[attributes_count++] = SHADER_ATTRIBUTE_NORMAL;   attributes.data[attributes_count++] = 3; }

	uint32_t const indices_count = wfobj.triangles.count / 3;
	for (uint32_t i = 0, vertex_id = 0; i < indices_count; i++) {
		uint32_t const * data = array_at(&wfobj.triangles, i * 3);
		uint32_t const vertex_index[] = {data[0], data[1], data[2]};

		// @todo: reuse matching vertices instead of copying them
		//        naive linear search would be quadratically slow,
		//        so, a hash_table it is

		uint32_t attribute_index = 1;
		if (wfobj.positions.count > 0) { array_push_many(&vertices, 3, array_at(&wfobj.positions, vertex_index[0] * attributes.data[attribute_index])); attribute_index += 2; }
		if (wfobj.texcoords.count > 0) { array_push_many(&vertices, 2, array_at(&wfobj.texcoords, vertex_index[1] * attributes.data[attribute_index])); attribute_index += 2; }
		if (wfobj.normals.count   > 0) { array_push_many(&vertices, 3, array_at(&wfobj.normals,   vertex_index[2] * attributes.data[attribute_index])); attribute_index += 2; }

		array_push_many(&indices, 1, &vertex_id);
		vertex_id++;
	}

	wfobj_free(&wfobj);

	// construct
	struct Mesh mesh = {
		.buffers= array_init(sizeof(struct Mesh_Buffer)),
	};
	array_resize(&mesh.buffers, 2);
	array_push_many(&mesh.buffers, mesh.buffers.capacity, (struct Mesh_Buffer[]){
		{
			.buffer = {
				.allocate = vertices.allocate,
				.data     = vertices.data,
				.capacity = vertices.capacity * vertices.value_size,
				.size     = vertices.count * vertices.value_size,
			},
			.format = {
				.type = DATA_TYPE_R32_F,
			},
			.attributes = attributes,
		},
		{
			.buffer = {
				.allocate = indices.allocate,
				.data     = indices.data,
				.capacity = indices.capacity * indices.value_size,
				.size     = indices.count * indices.value_size,
			},
			.format = {
				.mode = MESH_MODE_TRIANGLES,
				.type = DATA_TYPE_R32_U,
			},
			.is_index = true,
		},
	});

	vertices = (struct Array){0};
	indices = (struct Array){0};

	return mesh;
}
