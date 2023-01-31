#include "framework/memory.h"
#include "framework/logger.h"
#include "framework/maths.h"
#include "framework/json_read.h"
#include "framework/platform_file.h"
#include "framework/containers/buffer.h"
#include "framework/containers/strings.h"
#include "framework/systems/asset_system.h"
#include "framework/graphics/gpu_objects.h"
#include "framework/graphics/gpu_misc.h"

#include "framework/assets/mesh.h"
#include "framework/assets/image.h"
#include "framework/assets/json.h"
#include "framework/assets/typeface.h"
#include "framework/assets/font_atlas.h"

#include "json_load.h"

//
#include "asset_types.h"

// ----- ----- ----- ----- -----
//     Asset bytes part
// ----- ----- ----- ----- -----

static void asset_bytes_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Bytes * asset = instance;
	(void)system;

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

static void asset_bytes_free(struct Asset_System * system, void * instance) {
	struct Asset_Bytes * asset = instance;
	(void)system;
	MEMORY_FREE(asset->data);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset json part
// ----- ----- ----- ----- -----

static struct Strings gs_asset_json_strings;

static void asset_json_type_init(void) {
	gs_asset_json_strings = strings_init();
}

static void asset_json_type_free(void) {
	strings_free(&gs_asset_json_strings);
}

static void asset_json_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_JSON * asset = instance;
	(void)system;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}

	*asset = (struct Asset_JSON){
		.value = json_init(&gs_asset_json_strings, (char const *)file_buffer.data),
	};
	buffer_free(&file_buffer);
}

static void asset_json_free(struct Asset_System * system, void * instance) {
	struct Asset_JSON * asset = instance;
	(void)system;
	json_free(&asset->value);
	// common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset shader part
// ----- ----- ----- ----- -----

static void asset_shader_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Shader * asset = instance;
	(void)system;

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

static void asset_shader_free(struct Asset_System * system, void * instance) {
	struct Asset_Shader * asset = instance;
	(void)system;
	gpu_program_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset image part
// ----- ----- ----- ----- -----

struct Asset_Image_Context {
	struct Asset_System * system;
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

static void asset_image_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Image * asset = instance;
	struct Asset_Image_Context context = { .system = system, .result = asset };
	process_json(name, &context, asset_image_fill);
}

static void asset_image_free(struct Asset_System * system, void * instance) {
	struct Asset_Image * asset = instance;
	(void)system;
	gpu_texture_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset typeface part
// ----- ----- ----- ----- -----

static void asset_typeface_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Typeface * asset = instance;
	(void)system;

	struct Buffer file_buffer = platform_file_read_entire(name);
	if (file_buffer.capacity == 0) {
		common_memset(asset, 0, sizeof(*asset));
		return;
	}

	struct Font * font = font_init(&file_buffer);

	*asset = (struct Asset_Typeface){
		.typeface = font,
	};
}

static void asset_typeface_free(struct Asset_System * system, void * instance) {
	struct Asset_Typeface * asset = instance;
	(void)system;
	font_free(asset->typeface);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset font part
// ----- ----- ----- ----- -----

struct Asset_Fonts_Context {
	struct Asset_System * system;
	struct Asset_Fonts * result;
};

static void asset_fonts_fill(struct JSON const * json, void * data) {
	struct Asset_Fonts_Context * context = data;
	if (json->type == JSON_ERROR) {
		common_memset(context->result, 0, sizeof(*context->result));
		return;
	}

	struct Font_Atlas * font_atlas = font_atlas_init();

	// @todo: track handles
	struct CString const path = json_get_string(json, S_("path"));
	struct Asset_Handle const handle = asset_system_aquire(context->system, path);
	struct Asset_Typeface const * asset = asset_system_find_instance(context->system, handle);
	font_atlas_set_typeface(font_atlas, asset->typeface, 0, UINT32_MAX);

	*context->result = (struct Asset_Fonts){
		.font_atlas = font_atlas,
		.gpu_handle = gpu_texture_init(font_atlas_get_asset(font_atlas)),
	};
	
}

static void asset_fonts_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Fonts * asset = instance;
	struct Asset_Fonts_Context context = { .system = system, .result = asset };
	process_json(name, &context, asset_fonts_fill);
}

static void asset_fonts_free(struct Asset_System * system, void * instance) {
	struct Asset_Fonts * asset = instance;
	(void)system;
	font_atlas_free(asset->font_atlas);
	gpu_texture_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
// -- Asset target part
// ----- ----- ----- ----- -----

struct Asset_Target_Context {
	struct Asset_System * system;
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

static void asset_target_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Target * asset = instance;
	struct Asset_Target_Context context = { .system = system, .result = asset };
	process_json(name, &context, asset_target_fill);
}

static void asset_target_free(struct Asset_System * system, void * instance) {
	struct Asset_Target * asset = instance;
	(void)system;
	gpu_target_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
//     Asset model part
// ----- ----- ----- ----- -----

static void asset_model_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Model * asset = instance;
	(void)system;

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

static void asset_model_free(struct Asset_System * system, void * instance) {
	struct Asset_Model * asset = instance;
	(void)system;
	gpu_mesh_free(asset->gpu_handle);
	common_memset(asset, 0, sizeof(*asset));
}

// ----- ----- ----- ----- -----
// -- Asset material part
// ----- ----- ----- ----- -----

struct Asset_Material_Context {
	struct Asset_System * system;
	struct Asset_Material * result;
};

static void asset_material_fill(struct JSON const * json, void * data) {
	struct Asset_Material_Context * context = data;
	if (json->type == JSON_ERROR) {
		common_memset(context->result, 0, sizeof(*context->result));
		return;
	}

	struct Gfx_Material material;
	json_load_gfx_material(context->system, json, &material);

	*context->result = (struct Asset_Material){
		.value = material,
	};
}

static void asset_material_init(struct Asset_System * system, void * instance, struct CString name) {
	struct Asset_Material * asset = instance;
	struct Asset_Material_Context context = { .system = system, .result = asset };
	process_json(name, &context, asset_material_fill);
}

static void asset_material_free(struct Asset_System * system, void * instance) {
	struct Asset_Material * asset = instance;
	(void)system;
	gfx_material_free(&asset->value);
	common_memset(asset, 0, sizeof(*asset));
}

//
#include "asset_registry.h"

void asset_types_init(struct Asset_System * system) {
	asset_system_map_extension(system, S_("bytes"),    S_("txt"));
	asset_system_map_extension(system, S_("json"),     S_("json"));
	asset_system_map_extension(system, S_("shader"),   S_("glsl"));
	asset_system_map_extension(system, S_("image"),    S_("image"));
	asset_system_map_extension(system, S_("typeface"), S_("ttf"));
	asset_system_map_extension(system, S_("typeface"), S_("otf"));
	asset_system_map_extension(system, S_("fonts"),    S_("fonts"));
	asset_system_map_extension(system, S_("target"),   S_("target"));
	asset_system_map_extension(system, S_("model"),    S_("obj"));
	asset_system_map_extension(system, S_("material"), S_("material"));

	asset_system_set_type(system, S_("bytes"), (struct Asset_Callbacks){
		.init = asset_bytes_init,
		.free = asset_bytes_free,
	}, sizeof(struct Asset_Bytes));

	asset_system_set_type(system, S_("json"), (struct Asset_Callbacks){
		.type_init = asset_json_type_init,
		.type_free = asset_json_type_free,
		.init = asset_json_init,
		.free = asset_json_free,
	}, sizeof(struct Asset_JSON));

	asset_system_set_type(system, S_("shader"), (struct Asset_Callbacks){
		.init = asset_shader_init,
		.free = asset_shader_free,
	}, sizeof(struct Asset_Shader));

	asset_system_set_type(system, S_("image"), (struct Asset_Callbacks){
		.init = asset_image_init,
		.free = asset_image_free,
	}, sizeof(struct Asset_Image));

	asset_system_set_type(system, S_("typeface"), (struct Asset_Callbacks){
		.init = asset_typeface_init,
		.free = asset_typeface_free,
	}, sizeof(struct Asset_Typeface));

	asset_system_set_type(system, S_("fonts"), (struct Asset_Callbacks){
		.init = asset_fonts_init,
		.free = asset_fonts_free,
	}, sizeof(struct Asset_Fonts));

	asset_system_set_type(system, S_("target"), (struct Asset_Callbacks){
		.init = asset_target_init,
		.free = asset_target_free,
	}, sizeof(struct Asset_Target));

	asset_system_set_type(system, S_("model"), (struct Asset_Callbacks){
		.init = asset_model_init,
		.free = asset_model_free,
	}, sizeof(struct Asset_Model));

	asset_system_set_type(system, S_("material"), (struct Asset_Callbacks){
		.init = asset_material_init,
		.free = asset_material_free,
	}, sizeof(struct Asset_Material));
}

void asset_types_free(struct Asset_System * system) {
	asset_system_del_type(system, S_("bytes"));
	asset_system_del_type(system, S_("json"));
	asset_system_del_type(system, S_("shader"));
	asset_system_del_type(system, S_("image"));
	asset_system_del_type(system, S_("typeface"));
	asset_system_del_type(system, S_("fonts"));
	asset_system_del_type(system, S_("target"));
	asset_system_del_type(system, S_("model"));
	asset_system_del_type(system, S_("material"));
}
