#include "framework/formatter.h"
#include "framework/maths.h"
#include "framework/json_read.h"

#include "framework/platform/file.h"
#include "framework/containers/buffer.h"

#include "framework/systems/memory.h"
#include "framework/systems/assets.h"
#include "framework/systems/materials.h"

#include "framework/graphics/gfx_objects.h"
#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/json.h"
#include "framework/assets/typeface.h"
#include "framework/assets/font.h"

#include "json_load.h"


//
#include "asset_types.h"

// ----- ----- ----- ----- -----
//     Asset bytes part
// ----- ----- ----- ----- -----

static HANDLE_ACTION(asset_bytes_load) {
	struct Asset_Bytes * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		zero_out(CBMP_(asset));
		return;
	}

	// @note: memory ownership transfer
	*asset = (struct Asset_Bytes){
		.data = file_buffer.data,
		.length = (uint32_t)file_buffer.size,
	};
}

static HANDLE_ACTION(asset_bytes_drop) {
	struct Asset_Bytes * asset = system_assets_get(handle);
	FREE(asset->data);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset json part
// ----- ----- ----- ----- -----

static HANDLE_ACTION(asset_json_load) {
	struct Asset_JSON * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		zero_out(CBMP_(asset));
		return;
	}

	*asset = (struct Asset_JSON){
		.value = json_parse((struct CString){
			.length = (uint32_t)file_buffer.size,
			.data = file_buffer.data,
		}),
	};
	buffer_free(&file_buffer);
}

static HANDLE_ACTION(asset_json_drop) {
	struct Asset_JSON * asset = system_assets_get(handle);
	json_free(&asset->value);
	// zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----

static HANDLE_ACTION(asset_shader_load) {
	struct Asset_Shader * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		zero_out(CBMP_(asset));
		return;
	}
	// @todo: return error shader?

	*asset = (struct Asset_Shader){
		.gh_program = gpu_program_init(&file_buffer),
	};
	buffer_free(&file_buffer);
}

static HANDLE_ACTION(asset_shader_drop) {
	struct Asset_Shader * asset = system_assets_get(handle);
	gpu_program_free(asset->gh_program);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

static JSON_PROCESSOR(asset_sampler_fill) {
	struct Asset_Sampler * context = data;
	if (json->type == JSON_OBJECT) {
		struct Gfx_Sampler const settings = json_read_sampler(json);
		context->gh_sampler = gpu_sampler_init(&settings);
	}
}

static HANDLE_ACTION(asset_sampler_load) {
	struct Asset_Sampler * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);
	process_json(name, asset, asset_sampler_fill);
}

static HANDLE_ACTION(asset_sampler_drop) {
	struct Asset_Sampler * asset = system_assets_get(handle);
	gpu_sampler_free(asset->gh_sampler);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

static JSON_PROCESSOR(asset_image_meta_fill) {
	struct Image * context = data;
	if (json->type == JSON_OBJECT) {
		context->settings = json_read_texture_settings(json);

		context->settings.sublevels = min_u32(
			context->settings.sublevels,
			(uint32_t)r32_log2((float)max_u32(
				context->size.x, context->size.y
			))
		);

	}
}

static HANDLE_ACTION(asset_image_load) {
	struct Asset_Image * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);

	// binary
	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		zero_out(CBMP_(asset));
		return;
	}

	struct Image image = image_init(&file_buffer);
	buffer_free(&file_buffer); // @todo: optional arena allocator?

	// meta
	struct CString const meta_suffix = S_(".meta");
	char * meta_name_data = ARENA_ALLOCATE_ARRAY(char, name.length + meta_suffix.length + 1);
	common_memcpy(meta_name_data, name.data, name.length);
	common_memcpy(meta_name_data + name.length, meta_suffix.data, meta_suffix.length);
	meta_name_data[name.length + meta_suffix.length] = '\0';
	struct CString const meta_name = {
		.length = name.length + meta_suffix.length,
		.data = meta_name_data,
	};

	process_json(meta_name, &image, asset_image_meta_fill);
	ARENA_FREE(meta_name_data);

	// upload
	*asset = (struct Asset_Image){
		.gh_texture = gpu_texture_init(&image),
	};
	image_free(&image); // @todo: optional arena allocator?
}

static HANDLE_ACTION(asset_image_drop) {
	struct Asset_Image * asset = system_assets_get(handle);
	gpu_texture_free(asset->gh_texture);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset typeface part
// ----- ----- ----- ----- -----

static HANDLE_ACTION(asset_typeface_load) {
	struct Asset_Typeface * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		zero_out(CBMP_(asset));
		return;
	}

	struct Typeface * typeface = typeface_init(&file_buffer);

	*asset = (struct Asset_Typeface){
		.typeface = typeface,
	};
}

static HANDLE_ACTION(asset_typeface_drop) {
	struct Asset_Typeface * asset = system_assets_get(handle);
	typeface_free(asset->typeface);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset glyph atlas part
// ----- ----- ----- ----- -----

static JSON_PROCESSOR(asset_font_fill) {
	struct Handle const * context = data;
	struct Asset_Font * asset = system_assets_get(*context);
	if (json->type == JSON_ERROR) {
		zero_out(CBMP_(asset));
		return;
	}

	struct Font * font = font_init();

	// @todo: track handles
	struct JSON const * ranges = json_get(json, S_("ranges"));
	if (ranges->type == JSON_ARRAY) {
		uint32_t const count = json_count(ranges);
		for (uint32_t i = 0; i < count; i++) {
			struct JSON const * range = json_at(ranges, i);

			uint32_t from, to;
			struct Handle const ah_typeface = json_load_font_range(range, &from, &to);

			struct Asset_Typeface const * typeface = system_assets_get(ah_typeface);
			if (typeface == NULL) { continue; }
			font_set_typeface(font, typeface->typeface, from, to);
		}
	}

	*asset = (struct Asset_Font){
		.font = font,
		.gh_texture = gpu_texture_init(font_get_asset(font)),
	};
	
}

static HANDLE_ACTION(asset_font_load) {
	struct CString const name = system_assets_get_name(handle);
	process_json(name, &handle, asset_font_fill);
}

static HANDLE_ACTION(asset_font_drop) {
	struct Asset_Font * asset = system_assets_get(handle);
	font_free(asset->font);
	gpu_texture_free(asset->gh_texture);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
// -- Asset target part
// ----- ----- ----- ----- -----

static JSON_PROCESSOR(asset_target_fill) {
	struct Asset_Target * context = data;
	if (json->type == JSON_ERROR) {
		zero_out(CBMP_(context));
		return;
	}

	*context = (struct Asset_Target){
		.gh_target = json_read_target(json),
	};
}

static HANDLE_ACTION(asset_target_load) {
	struct Asset_Target * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);
	process_json(name, asset, asset_target_fill);
}

static HANDLE_ACTION(asset_target_drop) {
	struct Asset_Target * asset = system_assets_get(handle);
	gpu_target_free(asset->gh_target);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----

static HANDLE_ACTION(asset_model_load) {
	struct Asset_Model * asset = system_assets_get(handle);
	struct CString const name = system_assets_get_name(handle);

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		zero_out(CBMP_(asset));
		return;
	}

	struct Mesh mesh = mesh_init(&file_buffer);
	buffer_free(&file_buffer);

	*asset = (struct Asset_Model){
		.gh_mesh = gpu_mesh_init(&mesh),
	};
	mesh_free(&mesh);
}

static HANDLE_ACTION(asset_model_drop) {
	struct Asset_Model * asset = system_assets_get(handle);
	gpu_mesh_free(asset->gh_mesh);
	zero_out(CBMP_(asset));
}

// ----- ----- ----- ----- -----
// -- Asset material part
// ----- ----- ----- ----- -----

static JSON_PROCESSOR(asset_material_fill) {
	struct Handle const * context = data;
	struct Asset_Material * asset = system_assets_get(*context);
	if (json->type == JSON_ERROR) {
		zero_out(CBMP_(asset));
		return;
	}

	*asset = (struct Asset_Material){
		.mh_mat = json_load_gfx_material(json),
	};
}

static HANDLE_ACTION(asset_material_load) {
	struct CString const name = system_assets_get_name(handle);
	process_json(name, &handle, asset_material_fill);
}

static HANDLE_ACTION(asset_material_drop) {
	struct Asset_Material * asset = system_assets_get(handle);
	system_materials_discard(asset->mh_mat);
	zero_out(CBMP_(asset));
}

//

void asset_types_map(void) {
	system_assets_type_map(S_("bytes"),    S_("txt"));
	system_assets_type_map(S_("shader"),   S_("glsl"));
	system_assets_type_map(S_("image"),    S_("png"));
	system_assets_type_map(S_("typeface"), S_("ttf"));
	system_assets_type_map(S_("typeface"), S_("otf"));
	system_assets_type_map(S_("model"),    S_("obj"));
}

void asset_types_set(void) {
	system_assets_type_set(S_("bytes"), (struct Asset_Info){
		.size = sizeof(struct Asset_Bytes),
		.load = asset_bytes_load,
		.drop = asset_bytes_drop,
	});

	system_assets_type_set(S_("json"), (struct Asset_Info){
		.size = sizeof(struct Asset_JSON),
		.load = asset_json_load,
		.drop = asset_json_drop,
	});

	system_assets_type_set(S_("shader"), (struct Asset_Info){
		.size = sizeof(struct Asset_Shader),
		.load = asset_shader_load,
		.drop = asset_shader_drop,
	});

	system_assets_type_set(S_("sampler"), (struct Asset_Info){
		.size = sizeof(struct Asset_Sampler),
		.load = asset_sampler_load,
		.drop = asset_sampler_drop,
	});

	system_assets_type_set(S_("image"), (struct Asset_Info){
		.size = sizeof(struct Asset_Image),
		.load = asset_image_load,
		.drop = asset_image_drop,
	});

	system_assets_type_set(S_("typeface"), (struct Asset_Info){
		.size = sizeof(struct Asset_Typeface),
		.load = asset_typeface_load,
		.drop = asset_typeface_drop,
	});

	system_assets_type_set(S_("font"), (struct Asset_Info){
		.size = sizeof(struct Asset_Font),
		.load = asset_font_load,
		.drop = asset_font_drop,
	});

	system_assets_type_set(S_("target"), (struct Asset_Info){
		.size = sizeof(struct Asset_Target),
		.load = asset_target_load,
		.drop = asset_target_drop,
	});

	system_assets_type_set(S_("model"), (struct Asset_Info){
		.size = sizeof(struct Asset_Model),
		.load = asset_model_load,
		.drop = asset_model_drop,
	});

	system_assets_type_set(S_("material"), (struct Asset_Info){
		.size = sizeof(struct Asset_Material),
		.load = asset_material_load,
		.drop = asset_material_drop,
	});
}
