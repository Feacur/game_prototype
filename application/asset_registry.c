#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/json_read.h"
#include "framework/platform_file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/systems/asset_system.h"
#include "framework/systems/material_system.h"
#include "framework/graphics/gpu_objects.h"

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

static void asset_bytes_load(void * instance, struct CString name) {
	struct Asset_Bytes * asset = instance;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}

	// @note: memory ownership transfer
	*asset = (struct Asset_Bytes){
		.data = file_buffer.data,
		.length = (uint32_t)file_buffer.size,
	};
}

static void asset_bytes_drop(void * instance) {
	struct Asset_Bytes * asset = instance;
	MEMORY_FREE(asset->data);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset json part
// ----- ----- ----- ----- -----

static void asset_json_load(void * instance, struct CString name) {
	struct Asset_JSON * asset = instance;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}

	*asset = (struct Asset_JSON){
		.value = json_parse((char const *)file_buffer.data),
	};
	buffer_free(&file_buffer);
}

static void asset_json_drop(void * instance) {
	struct Asset_JSON * asset = instance;
	json_free(&asset->value);
	// common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----

static void asset_shader_load(void * instance, struct CString name) {
	struct Asset_Shader * asset = instance;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}
	// @todo: return error shader?

	*asset = (struct Asset_Shader){
		.gpu_handle = gpu_program_init(&file_buffer),
	};
	buffer_free(&file_buffer);
}

static void asset_shader_drop(void * instance) {
	struct Asset_Shader * asset = instance;
	gpu_program_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

struct Asset_Image_Context {
	struct Asset_Image * result;
};

static void asset_image_fill(struct JSON const * json, void * data) {
	struct Asset_Image_Context * context = data;
	if (json->type == JSON_ERROR) {
		common_memset(context->result, 0, sizeof(*context->result));
		return;
	}

	struct CString const path = json_get_string(json, S_("path"));
	struct Buffer file_buffer = platform_file_read_entire(path);
	if (file_buffer.capacity == 0) {
		common_memset(context->result, 0, sizeof(*context->result));
		return;
	}

	struct Texture_Settings settings = json_read_texture_settings(json);

	struct Image image = image_init(settings, &file_buffer);
	buffer_free(&file_buffer);

	*context->result = (struct Asset_Image){
		.gpu_handle = gpu_texture_init(&image),
	};
	image_free(&image);

}

static void asset_image_load(void * instance, struct CString name) {
	struct Asset_Image * asset = instance;
	struct Asset_Image_Context context = { .result = asset };
	process_json(name, &context, asset_image_fill);
}

static void asset_image_drop(void * instance) {
	struct Asset_Image * asset = instance;
	gpu_texture_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset typeface part
// ----- ----- ----- ----- -----

static void asset_typeface_load(void * instance, struct CString name) {
	struct Asset_Typeface * asset = instance;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}

	struct Typeface * typeface = typeface_init(&file_buffer);

	*asset = (struct Asset_Typeface){
		.typeface = typeface,
	};
}

static void asset_typeface_drop(void * instance) {
	struct Asset_Typeface * asset = instance;
	typeface_free(asset->typeface);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset glyph atlas part
// ----- ----- ----- ----- -----

struct Asset_font_Context {
	struct Asset_Font * result;
};

static void asset_font_fill(struct JSON const * json, void * data) {
	struct Asset_font_Context * context = data;
	if (json->type == JSON_ERROR) {
		common_memset(context->result, 0, sizeof(*context->result));
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
			struct Handle const handle = json_load_font_range(range, &from, &to);
			if (handle_is_null(handle)) { continue; }
			if (from > to) { continue; }

			struct Asset_Typeface const * asset = asset_system_get(handle);
			font_set_typeface(font, asset->typeface, from, to);
		}
	}

	*context->result = (struct Asset_Font){
		.font = font,
		.gpu_handle = gpu_texture_init(font_get_asset(font)),
	};
	
}

static void asset_font_load(void * instance, struct CString name) {
	struct Asset_Font * asset = instance;
	struct Asset_font_Context context = { .result = asset };
	process_json(name, &context, asset_font_fill);
}

static void asset_font_drop(void * instance) {
	struct Asset_Font * asset = instance;
	font_free(asset->font);
	gpu_texture_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
// -- Asset target part
// ----- ----- ----- ----- -----

struct Asset_Target_Context {
	struct Asset_Target * result;
};

static void asset_target_fill(struct JSON const * json, void * data) {
	struct Asset_Target_Context * context = data;
	if (json->type == JSON_ERROR) {
		common_memset(context->result, 0, sizeof(*context->result));
		return;
	}

	*context->result = (struct Asset_Target){
		.gpu_handle = json_read_target(json),
	};
}

static void asset_target_load(void * instance, struct CString name) {
	struct Asset_Target * asset = instance;
	struct Asset_Target_Context context = { .result = asset };
	process_json(name, &context, asset_target_fill);
}

static void asset_target_drop(void * instance) {
	struct Asset_Target * asset = instance;
	gpu_target_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----

static void asset_model_load(void * instance, struct CString name) {
	struct Asset_Model * asset = instance;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}

	struct Mesh mesh = mesh_init(&file_buffer);
	buffer_free(&file_buffer);

	*asset = (struct Asset_Model){
		.gpu_handle = gpu_mesh_init(&mesh),
	};
	mesh_free(&mesh);
}

static void asset_model_drop(void * instance) {
	struct Asset_Model * asset = instance;
	gpu_mesh_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
// -- Asset material part
// ----- ----- ----- ----- -----

struct Asset_Material_Context {
	struct Asset_Material * result;
};

static void asset_material_fill(struct JSON const * json, void * data) {
	struct Asset_Material_Context * context = data;
	if (json->type == JSON_ERROR) {
		common_memset(context->result, 0, sizeof(*context->result));
		return;
	}

	struct Handle handle = json_load_gfx_material(json);

	*context->result = (struct Asset_Material){
		.ms_handle = handle,
	};
}

static void asset_material_load(void * instance, struct CString name) {
	struct Asset_Material * asset = instance;
	struct Asset_Material_Context context = { .result = asset };
	process_json(name, &context, asset_material_fill);
}

static void asset_material_drop(void * instance) {
	struct Asset_Material * asset = instance;
	material_system_discard(asset->ms_handle);
	common_memset(asset, 0, sizeof(*asset));
}

//
#include "asset_registry.h"

void asset_types_init(void) {
	asset_system_map_extension(S_("bytes"),    S_("txt"));
	asset_system_map_extension(S_("shader"),   S_("glsl"));
	asset_system_map_extension(S_("typeface"), S_("ttf"));
	asset_system_map_extension(S_("typeface"), S_("otf"));
	asset_system_map_extension(S_("model"),    S_("obj"));

	asset_system_set_type(S_("bytes"), (struct Asset_Info){
		.size = sizeof(struct Asset_Bytes),
		.load = asset_bytes_load,
		.drop = asset_bytes_drop,
	});

	asset_system_set_type(S_("json"), (struct Asset_Info){
		.size = sizeof(struct Asset_JSON),
		.load = asset_json_load,
		.drop = asset_json_drop,
	});

	asset_system_set_type(S_("shader"), (struct Asset_Info){
		.size = sizeof(struct Asset_Shader),
		.load = asset_shader_load,
		.drop = asset_shader_drop,
	});

	asset_system_set_type(S_("image"), (struct Asset_Info){
		.size = sizeof(struct Asset_Image),
		.load = asset_image_load,
		.drop = asset_image_drop,
	});

	asset_system_set_type(S_("typeface"), (struct Asset_Info){
		.size = sizeof(struct Asset_Typeface),
		.load = asset_typeface_load,
		.drop = asset_typeface_drop,
	});

	asset_system_set_type(S_("font"), (struct Asset_Info){
		.size = sizeof(struct Asset_Font),
		.load = asset_font_load,
		.drop = asset_font_drop,
	});

	asset_system_set_type(S_("target"), (struct Asset_Info){
		.size = sizeof(struct Asset_Target),
		.load = asset_target_load,
		.drop = asset_target_drop,
	});

	asset_system_set_type(S_("model"), (struct Asset_Info){
		.size = sizeof(struct Asset_Model),
		.load = asset_model_load,
		.drop = asset_model_drop,
	});

	asset_system_set_type(S_("material"), (struct Asset_Info){
		.size = sizeof(struct Asset_Material),
		.load = asset_material_load,
		.drop = asset_material_drop,
	});
}

void asset_types_free(void) {
	asset_system_del_type(S_("bytes"));
	asset_system_del_type(S_("json"));
	asset_system_del_type(S_("shader"));
	asset_system_del_type(S_("image"));
	asset_system_del_type(S_("typeface"));
	asset_system_del_type(S_("font"));
	asset_system_del_type(S_("target"));
	asset_system_del_type(S_("model"));
	asset_system_del_type(S_("material"));
}
