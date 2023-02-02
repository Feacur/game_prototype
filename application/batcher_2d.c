#include "framework/common.h"
#include "framework/memory.h"
#include "framework/unicode.h"
#include "framework/logger.h"

#include "framework/containers/hash_set_u64.h"
#include "framework/containers/array_any.h"
#include "framework/containers/array_flt.h"
#include "framework/containers/array_u32.h"
#include "framework/containers/handle.h"

#include "framework/assets/mesh.h"
#include "framework/assets/glyph_atlas.h"

#include "framework/graphics/material.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"
#include "framework/graphics/gpu_command.h"
#include "framework/maths.h"

#include "asset_types.h"

// @todo: UTF-8 edge cases, different languages, LTR/RTL, ligatures, etc.
// @idea: point to a `Gfx_Material` with an `Asset_Ref` instead
// @idea: static batching option; cache vertices and stuff until changed
// @idea: add a 3d batcher

// quad layout
// 3-----------2
// |         / |
// |       /   |
// |     /     |
// |   /       |
// | /         |
// 0-----------1

#define BATCHER_2D_BUFFERS_COUNT 2

struct Batcher_2D_Word {
	uint32_t codepoints_offset, codepoints_end;
	uint32_t buffer_vertices_offset;
	// @idea: use `Asset_Ref` instead
	struct Asset_Glyph_Atlas const * cached_glyph_atlas; float size;
	// @note: local to `batcher_2d_add_text`
	uint32_t breaker_codepoint;
	struct vec2 position;
	float full_size_x;
};

struct Batcher_2D_Batch {
	struct Gfx_Material const * material; // @idea: internalize materials if async
	uint32_t indices_offset, indices_length;
	uint32_t uniform_offset, uniform_length;
};

struct Batcher_2D_Vertex {
	struct vec2 position;
	struct vec2 tex_coord;
	struct vec4 color;
};

struct Batcher_2D {
	struct Gfx_Uniforms uniforms;
	//
	struct Batcher_2D_Batch batch;
	struct Array_U32 codepoints;
	struct Array_Any batches;                 // `struct Batcher_2D_Batch`
	struct Array_Any words;                   // `struct Batcher_2D_Word`
	struct Hash_Set_U64 cached_glyph_atlases; // `struct Asset_Glyph_Atlas const *`
	//
	struct vec4 color;
	struct mat4 matrix;
	//
	struct Array_Any buffer_vertices; // `struct Batcher_2D_Vertex`
	struct Array_U32 buffer_indices;
	//
	struct Mesh_Parameters mesh_parameters[BATCHER_2D_BUFFERS_COUNT];
	struct Handle gpu_mesh_handle;
};

static void batcher_2d_bake_pass(struct Batcher_2D * batcher);

//
#include "batcher_2d.h"

struct Batcher_2D * batcher_2d_init(void) {
	struct Batcher_2D * batcher = MEMORY_ALLOCATE(struct Batcher_2D);
	*batcher = (struct Batcher_2D){
		.uniforms = gfx_uniforms_init(),
		.color = (struct vec4){1, 1, 1, 1},
		.matrix = c_mat4_identity,
		.mesh_parameters = {
			(struct Mesh_Parameters){
				.type = DATA_TYPE_R32_F,
				.flags = MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
				.attributes = {
					ATTRIBUTE_TYPE_POSITION,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, position) / sizeof(float),
					ATTRIBUTE_TYPE_TEXCOORD,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, tex_coord) / sizeof(float),
					ATTRIBUTE_TYPE_COLOR,
					SIZE_OF_MEMBER(struct Batcher_2D_Vertex, color) / sizeof(float),
				},
			},
			(struct Mesh_Parameters){
				.type = DATA_TYPE_R32_U,
				.flags = MESH_FLAG_INDEX | MESH_FLAG_MUTABLE | MESH_FLAG_WRITE | MESH_FLAG_FREQUENT,
			},
		},
		.codepoints      = array_u32_init(),
		.batches         = array_any_init(sizeof(struct Batcher_2D_Batch)),
		.words           = array_any_init(sizeof(struct Batcher_2D_Word)),
		.buffer_vertices = array_any_init(sizeof(struct Batcher_2D_Vertex)),
		.buffer_indices  = array_u32_init(),
	};

	//
	batcher->gpu_mesh_handle = gpu_mesh_init(&(struct Mesh){
		.count      = BATCHER_2D_BUFFERS_COUNT,
		.buffers    = (struct Buffer[BATCHER_2D_BUFFERS_COUNT]){0},
		.parameters = batcher->mesh_parameters,
	});

	return batcher;
}

void batcher_2d_free(struct Batcher_2D * batcher) {
	gpu_mesh_free(batcher->gpu_mesh_handle);
	//
	array_u32_free(&batcher->codepoints);
	array_any_free(&batcher->batches);
	array_any_free(&batcher->words);
	hash_set_u64_free(&batcher->cached_glyph_atlases);
	array_any_free(&batcher->buffer_vertices);
	array_u32_free(&batcher->buffer_indices);
	//
	gfx_uniforms_free(&batcher->uniforms);
	//
	common_memset(batcher, 0, sizeof(*batcher));
	MEMORY_FREE(batcher);
}

void batcher_2d_set_color(struct Batcher_2D * batcher, struct vec4 value) {
	batcher->color = value;
}

void batcher_2d_set_matrix(struct Batcher_2D * batcher, struct mat4 const value) {
	batcher->matrix = value;
}

void batcher_2d_set_material(struct Batcher_2D * batcher, struct Gfx_Material const * material) {
	if (batcher->batch.material != material) {
		batcher_2d_bake_pass(batcher);
	}
	batcher->batch.material = material;
}

void batcher_2d_uniforms_push(struct Batcher_2D * batcher, uint32_t uniform_id, struct CArray value) {
	struct CArray_Mut const field = gfx_uniforms_get(&batcher->uniforms, uniform_id, batcher->batch.uniform_offset);
	if (field.data != NULL) {
		batcher_2d_bake_pass(batcher);
	}
	gfx_uniforms_push(&batcher->uniforms, uniform_id, value);
}

inline static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord, struct vec4 color);
void batcher_2d_add_quad(
	struct Batcher_2D * batcher,
	struct rect rect,
	struct rect uv
) {
	uint32_t const vertex_offset = batcher->buffer_vertices.count;
	struct vec4 const color = batcher->color;
	array_any_push_many(&batcher->buffer_vertices, 4, (struct Batcher_2D_Vertex[]){
		batcher_2d_make_vertex(batcher->matrix, rect.min, uv.min, color),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect.max.x, rect.min.y}, (struct vec2){uv.max.x, uv.min.y}, color),
		batcher_2d_make_vertex(batcher->matrix, rect.max, uv.max, color),
		batcher_2d_make_vertex(batcher->matrix, (struct vec2){rect.min.x, rect.max.y}, (struct vec2){uv.min.x, uv.max.y}, color),
	});
	array_u32_push_many(&batcher->buffer_indices, 3 * 2, (uint32_t[]){
		vertex_offset + 0, vertex_offset + 1, vertex_offset + 2,
		vertex_offset + 2, vertex_offset + 3, vertex_offset + 0,
	});
}

void batcher_2d_add_text(
	struct Batcher_2D * batcher,
	struct rect rect, struct vec2 alignment, bool wrap,
	struct Asset_Glyph_Atlas const * glyph_atlases, struct CString value, float size
) {
	if (glyph_atlases == NULL) { return; }
	if (glyph_atlases->glyph_atlas == NULL) { return; }

	float const scale       = glyph_atlas_get_scale(glyph_atlases->glyph_atlas, size);
	float const ascent      = glyph_atlas_get_ascent(glyph_atlases->glyph_atlas, scale);
	float const descent     = glyph_atlas_get_descent(glyph_atlases->glyph_atlas, scale);
	float const line_gap    = glyph_atlas_get_gap(glyph_atlases->glyph_atlas, scale);
	float const line_height = ascent - descent + line_gap;

	uint32_t const words_offset = batcher->words.count;

	glyph_atlas_add_defaults(glyph_atlases->glyph_atlas, size);
	struct Glyph const * glyph_error = glyph_atlas_get_glyph(glyph_atlases->glyph_atlas, '\0', size);

	// break text into words
	{
		float word_width = 0;
		uint32_t codepoints_offset = batcher->codepoints.count;

		FOR_UTF8 (value.length, (uint8_t const *)value.data, it) {
			glyph_atlas_add_glyph(glyph_atlases->glyph_atlas, it.codepoint, size);

			struct Glyph const * glyph = glyph_atlas_get_glyph(glyph_atlases->glyph_atlas, it.codepoint, size);
			float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

			word_width += full_size_x;
			if (codepoint_is_word_break(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_any_push_many(&batcher->words, 1, &(struct Batcher_2D_Word) {
					.codepoints_offset = codepoints_offset, .codepoints_end = batcher->codepoints.count,
					.cached_glyph_atlas = glyph_atlases, .size = size,
					.breaker_codepoint = it.codepoint,
					.full_size_x = word_width,
				});

				//
				word_width = 0;
				codepoints_offset = batcher->codepoints.count;
			}

			if (!codepoint_is_invisible(it.codepoint)) {
				// @todo: (?) arena/stack allocator
				array_u32_push_many(&batcher->codepoints, 1, &it.codepoint);

				//
				float const kerning = glyph_atlas_get_kerning(glyph_atlases->glyph_atlas, it.previous, it.codepoint, scale);
				word_width += kerning;
			}
		}

		if (batcher->codepoints.count > codepoints_offset) {
			// @todo: (?) arena/stack allocator
			array_any_push_many(&batcher->words, 1, &(struct Batcher_2D_Word) {
				.codepoints_offset = codepoints_offset, .codepoints_end = batcher->codepoints.count,
				.cached_glyph_atlas = glyph_atlases, .size = size,
				.full_size_x = word_width,
			});
		}
	}

	// position words as for alignment {0, 1}
	{
		struct vec2 offset = {rect.min.x, rect.max.y};
		offset.y -= ascent;

		for (uint32_t word_i = words_offset; word_i < batcher->words.count; word_i++) {
			struct Batcher_2D_Word * word = array_any_at(&batcher->words, word_i);

			// break line if word overflows
			if (wrap && (offset.x + word->full_size_x > rect.max.x)) {
				offset.x = rect.min.x; // @note: auto `\r`
				offset.y -= line_height;
			}
			word->position = offset;

			// process word
			for (uint32_t strings_i = word->codepoints_offset; strings_i < word->codepoints_end; strings_i++) {
				uint32_t const codepoint = array_u32_at(&batcher->codepoints, strings_i);
				uint32_t const previous = (strings_i > word->codepoints_offset) ? array_u32_at(&batcher->codepoints, strings_i - 1) : '\0';

				struct Glyph const * glyph = glyph_atlas_get_glyph(word->cached_glyph_atlas->glyph_atlas, codepoint, word->size);
				float const full_size_x = (glyph != NULL) ? glyph->params.full_size_x : glyph_error->params.full_size_x;

				float const kerning = glyph_atlas_get_kerning(glyph_atlases->glyph_atlas, previous, codepoint, scale);
				offset.x += full_size_x + kerning;
			}

			// process breaker
			{
				struct Glyph const * glyph = glyph_atlas_get_glyph(word->cached_glyph_atlas->glyph_atlas, word->breaker_codepoint, word->size);
				offset.x += (glyph != NULL) ? glyph->params.full_size_x : 0;
				if (word->breaker_codepoint == '\n') {
					offset.x = rect.min.x; // @note: auto `\r`
					offset.y -= line_height;
				}
			}
		}
	}

	// align words
	{
		struct vec2 const error_margins = {
			0.0001f * (1 - 2*alignment.x),
			0.0001f * (1 - 2*alignment.y),
		};
		struct vec2 const rect_size = {
			rect.max.x - rect.min.x,
			rect.max.y - rect.min.y,
		};

		uint32_t line_offset = words_offset;
		uint32_t lines_count = 1;
		float line_position_y = 0;
		float line_width = 0;

		for (uint32_t word_i = words_offset; word_i < batcher->words.count; word_i++) {
			struct Batcher_2D_Word const * word = array_any_at(&batcher->words, word_i);

			if (line_position_y != word->position.y) {
				float const offset = lerp(0, rect_size.x - line_width, alignment.x) + error_margins.x;
				for (uint32_t i = line_offset; i < word_i; i++) {
					struct Batcher_2D_Word * aligned_text = array_any_at(&batcher->words, i);
					aligned_text->position.x += offset;
				}
				line_offset = word_i;
				lines_count++;

				//
				line_position_y = word->position.y;
				line_width = 0;
			}

			line_width += word->full_size_x;
		}

		{
			float const offset = lerp(0, rect_size.x - line_width, alignment.x) + error_margins.x;
			for (uint32_t i = line_offset; i < batcher->words.count; i++) {
				struct Batcher_2D_Word * word = array_any_at(&batcher->words, i);
				word->position.x += offset;
			}
		}

		{
			float const height = (float)lines_count * line_height;
			float const offset = lerp((height - rect_size.y) - line_height, 0, alignment.y) + error_margins.y;
			for (uint32_t i = words_offset; i < batcher->words.count; i++) {
				struct Batcher_2D_Word * word = array_any_at(&batcher->words, i);
				word->position.y += offset;
			}
		}
	}

	// translate words into vertices
	for (uint32_t word_i = words_offset; word_i < batcher->words.count; word_i++) {
		struct Batcher_2D_Word * word = array_any_at(&batcher->words, word_i);
		word->buffer_vertices_offset = batcher->buffer_vertices.count;

		struct vec2 offset = word->position;
		if (offset.y > rect.max.y) { word->codepoints_end = word->codepoints_offset; continue; } // void entry
		if (offset.y < rect.min.y) { batcher->words.count = word_i;                  break; }    // drop rest

		for (uint32_t strings_i = word->codepoints_offset; strings_i < word->codepoints_end; strings_i++) {
			uint32_t const codepoint = array_u32_at(&batcher->codepoints, strings_i);
				uint32_t const previous = (strings_i > word->codepoints_offset) ? array_u32_at(&batcher->codepoints, strings_i - 1) : '\0';

			struct Glyph const * glyph = glyph_atlas_get_glyph(word->cached_glyph_atlas->glyph_atlas, codepoint, word->size);
			struct Glyph_Params const params = (glyph != NULL) ? glyph->params : glyph_error->params;

			float const kerning = glyph_atlas_get_kerning(glyph_atlases->glyph_atlas, previous, codepoint, scale);
			float const offset_x = offset.x + kerning;
			offset.x += params.full_size_x + kerning;

			if (offset_x < rect.min.x) { word->codepoints_offset = strings_i + 1; continue; } // skip glyph
			if (offset.x > rect.max.x) { word->codepoints_end    = strings_i;     break; }    // drop rest

			if (!codepoint_is_invisible(codepoint)) {
				batcher_2d_add_quad(
					batcher,
					(struct rect){
						.min = {
							((float)params.rect.min.x) + offset_x,
							((float)params.rect.min.y) + offset.y,
						},
						.max = {
							((float)params.rect.max.x) + offset_x,
							((float)params.rect.max.y) + offset.y,
						},
					},
					(struct rect){0} // @note: UVs are delayed, see `batcher_2d_bake_texts`
				);
			}
		}
	}
}

static void batcher_2d_bake_words(struct Batcher_2D * batcher) {
	if (batcher->words.count == 0) { return; }

	// render and upload the atlases
	{
		// @todo: (?) arena/stack allocator
		hash_set_u64_clear(&batcher->cached_glyph_atlases);

		for (uint32_t i = 0; i < batcher->words.count; i++) {
			struct Batcher_2D_Word const * word = array_any_at(&batcher->words, i);
			hash_set_u64_set(&batcher->cached_glyph_atlases, (uint64_t)word->cached_glyph_atlas);
		}

		FOR_HASH_SET_U64 (&batcher->cached_glyph_atlases, it) {
			struct Asset_Glyph_Atlas const * glyph_atlas = (void *)it.key_hash;
			glyph_atlas_render(glyph_atlas->glyph_atlas);
		}

		FOR_HASH_SET_U64 (&batcher->cached_glyph_atlases, it) {
			struct Asset_Glyph_Atlas const * glyph_atlas = (void *)it.key_hash;
			gpu_texture_update(glyph_atlas->gpu_handle, glyph_atlas_get_asset(glyph_atlas->glyph_atlas));
		}
	}

	// fill quads UVs
	for (uint32_t word_i = 0; word_i < batcher->words.count; word_i++) {
		struct Batcher_2D_Word const * word = array_any_at(&batcher->words, word_i);
		uint32_t vertices_offset = word->buffer_vertices_offset;

		struct Glyph const * glyph_error = glyph_atlas_get_glyph(word->cached_glyph_atlas->glyph_atlas, '\0', word->size);
		struct rect const glyph_error_uv = glyph_error->uv;

		for (uint32_t strings_i = word->codepoints_offset; strings_i < word->codepoints_end; strings_i++) {
			uint32_t const codepoint = array_u32_at(&batcher->codepoints, strings_i);

			if (codepoint_is_invisible(codepoint)) { continue; }

			struct Glyph const * glyph = glyph_atlas_get_glyph(word->cached_glyph_atlas->glyph_atlas, codepoint, word->size);
			struct rect const uv = (glyph != NULL) ? glyph->uv : glyph_error_uv;

			struct Batcher_2D_Vertex * vertices = array_any_at(&batcher->buffer_vertices, vertices_offset);
			vertices[0].tex_coord = uv.min;
			vertices[1].tex_coord = (struct vec2){uv.max.x, uv.min.y};
			vertices[2].tex_coord = uv.max;
			vertices[3].tex_coord = (struct vec2){uv.min.x, uv.max.y};

			vertices_offset += 4;
		}
	}
}

void batcher_2d_clear(struct Batcher_2D * batcher) {
	common_memset(&batcher->batch, 0, sizeof(batcher->batch));
	array_u32_clear(&batcher->codepoints);
	array_any_clear(&batcher->batches);
	array_any_clear(&batcher->words);
	array_any_clear(&batcher->buffer_vertices);
	array_u32_clear(&batcher->buffer_indices);
	gfx_uniforms_clear(&batcher->uniforms);
}

void batcher_2d_issue_commands(struct Batcher_2D * batcher, struct Array_Any * gpu_commands) {
	batcher_2d_bake_pass(batcher);
	for (uint32_t i = 0; i < batcher->batches.count; i++) {
		struct Batcher_2D_Batch const * batch = array_any_at(&batcher->batches, i);

		array_any_push_many(gpu_commands, 1, &(struct GPU_Command){
			.type = GPU_COMMAND_TYPE_MATERIAL,
			.as.material = {
				.material = batch->material,
			},
		});

		if (batch->uniform_length > 0) {
			array_any_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_UNIFORM,
				.as.uniform = {
					.program_handle = batch->material->gpu_program_handle,
					.override = {
						.uniforms = &batcher->uniforms,
						.offset = batch->uniform_offset,
						.count = batch->uniform_length,
					},
				},
			});
		}

		if (batch->indices_length > 0) {
			array_any_push_many(gpu_commands, 1, &(struct GPU_Command){
				.type = GPU_COMMAND_TYPE_DRAW,
				.as.draw = {
					.mesh_handle = batcher->gpu_mesh_handle,
					.offset = batch->indices_offset, .length = batch->indices_length,
				},
			});
		}
	}
	array_any_clear(&batcher->batches);
}

void batcher_2d_bake(struct Batcher_2D * batcher) {
	if (batcher->batches.count > 0) {
		logger_to_console("unissued batches\n"); DEBUG_BREAK();
	}
	if (batcher->batch.indices_offset < batcher->buffer_indices.count) {
		logger_to_console("unissued indices\n"); DEBUG_BREAK();
	}

	batcher_2d_bake_words(batcher);
	gpu_mesh_update(batcher->gpu_mesh_handle, &(struct Mesh){
		.count = BATCHER_2D_BUFFERS_COUNT,
		.buffers = (struct Buffer[BATCHER_2D_BUFFERS_COUNT]){
			(struct Buffer){
				.data = batcher->buffer_vertices.data,
				.size = sizeof(struct Batcher_2D_Vertex) * batcher->buffer_vertices.count,
			},
			(struct Buffer){
				.data = batcher->buffer_indices.data,
				.size = sizeof(*batcher->buffer_indices.data) * batcher->buffer_indices.count,
			},
		},
		.parameters = batcher->mesh_parameters,
	});
}

//

inline static struct Batcher_2D_Vertex batcher_2d_make_vertex(struct mat4 m, struct vec2 position, struct vec2 tex_coord, struct vec4 color) {
	return (struct Batcher_2D_Vertex){
		.position = {
			m.x.x * position.x + m.y.x * position.y + m.w.x,
			m.x.y * position.x + m.y.y * position.y + m.w.y,
		},
		.tex_coord = tex_coord,
		.color = color,
	};

/*
	.position = mat4_mul_vec(m, position as vec4);
*/
}

static void batcher_2d_bake_pass(struct Batcher_2D * batcher) {
	uint32_t const offset = batcher->buffer_indices.count;
	uint32_t const u_offset = batcher->uniforms.headers.count;
	if (batcher->batch.indices_offset < offset || batcher->batch.uniform_offset < offset) {
		batcher->batch.indices_length = offset - batcher->batch.indices_offset;
		batcher->batch.uniform_length = u_offset - batcher->batch.uniform_offset;
		array_any_push_many(&batcher->batches, 1, &batcher->batch);

		batcher->batch.indices_offset = offset;
		batcher->batch.uniform_offset = u_offset;

		batcher->batch.indices_length = 0;
		batcher->batch.uniform_length = 0;
	}
}
