#include "framework/logger.h"

//
#include "asset_system.h"

struct Asset_Type {
	struct Asset_Callbacks callbacks;
	struct Handle_Table instances;
};

struct Asset_Entry {
	struct {
		uint32_t name_id;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

static void asset_system_del_type_internal(struct Asset_System * system, struct Asset_Type * asset_type);
static uint32_t asset_system_get_extension_from_name(struct CString name);

struct Asset_System asset_system_init(void) {
	return (struct Asset_System){
		.strings = strings_init(),
		.types = hash_table_u32_init(sizeof(struct Asset_Type)),
		.handles = hash_table_u32_init(sizeof(struct Handle)),
		.map = hash_table_u32_init(sizeof(uint32_t)),
	};
}

void asset_system_free(struct Asset_System * system) {
	FOR_HASH_TABLE_U32 (&system->types, it) {
		asset_system_del_type_internal(system, it.value);
	}
	strings_free(&system->strings);
	hash_table_u32_free(&system->types);
	hash_table_u32_free(&system->handles);
	hash_table_u32_free(&system->map);
	// common_memset(system, 0, sizeof(*system));
}

void asset_system_map_extension(struct Asset_System * system, struct CString type, struct CString extension) {
	if (type.data == NULL) { logger_to_console("empty type\n"); DEBUG_BREAK(); return; }

	if (extension.length == 0) { logger_to_console("empty extension\n"); DEBUG_BREAK(); return; }

	uint32_t const type_id = strings_add(&system->strings, type);
	uint32_t const extension_id = strings_add(&system->strings, extension);

	hash_table_u32_set(&system->map, extension_id, &type_id);
}

bool asset_system_match_type(struct Asset_System * system, struct Asset_Handle handle, struct CString type) {
	struct CString const asset_type = strings_get(&system->strings, handle.type_id);
	return cstring_equals(asset_type, type);
}

void asset_system_set_type(struct Asset_System * system, struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size) {
	if (type.data == NULL) { logger_to_console("empty type\n"); DEBUG_BREAK(); return; }

	if (callbacks.type_init != NULL) {
		callbacks.type_init();
	}

	struct Asset_Type asset_type = {
		.callbacks = callbacks,
		.instances = handle_table_init(SIZE_OF_MEMBER(struct Asset_Entry, header) + value_size),
	};

	uint32_t const type_id = strings_add(&system->strings, type);

	hash_table_u32_set(&system->types, type_id, &asset_type);
}

void asset_system_del_type(struct Asset_System * system, struct CString type) {
	if (type.data == NULL) { logger_to_console("empty type\n"); DEBUG_BREAK(); return; }

	uint32_t const type_id = strings_find(&system->strings, type);
	if (type_id == 0) { logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK(); return; }

	{
		struct Asset_Type * asset_type = hash_table_u32_get(&system->types, type_id);
		asset_system_del_type_internal(system, asset_type);
	}

	hash_table_u32_del(&system->types, type_id);
}

struct Asset_Handle asset_system_aquire(struct Asset_System * system, struct CString name) {
	if (name.data == NULL) { logger_to_console("empty name\n"); goto fail; }

	//
	uint32_t const extension_length = asset_system_get_extension_from_name(name);
	if (extension_length == 0) { logger_to_console("empty extension\n"); goto fail; }

	char const * extension_name = name.data + (name.length - extension_length);

	//
	uint32_t const extension_id = strings_find(&system->strings, (struct CString){
		.length = extension_length,
		.data = extension_name,
	});
	if (extension_id == 0) {
		logger_to_console("unknown extension: %.*s\n", extension_length, extension_name);
		goto fail;
	}

	uint32_t const * type_id = hash_table_u32_get(&system->map, extension_id);
	if (type_id == NULL) {
		logger_to_console("can't infer type from extension: %.*s\n", extension_length, extension_name);
		goto fail;
	}

	//
	uint32_t name_id = strings_add(&system->strings, name);
	struct Handle const * instance_handle_ptr = hash_table_u32_get(&system->handles, name_id);
	if (instance_handle_ptr != NULL) {
		return (struct Asset_Handle){
			.instance_handle = *instance_handle_ptr,
			.type_id = *type_id,
			.name_id = name_id,
		};
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, *type_id);
	if (asset_type == NULL) {
		struct CString const type = strings_get(&system->strings, *type_id);
		logger_to_console("unknown type: %.*s\n", type.length, type.data);
		goto fail;
	}

	//
	struct Handle const instance_handle = handle_table_aquire(&asset_type->instances, NULL);
	hash_table_u32_set(&system->handles, name_id, &instance_handle);

	struct Asset_Entry * entry = handle_table_get(&asset_type->instances, instance_handle);
	entry->header.name_id = name_id;

	if (asset_type->callbacks.init != NULL) {
		asset_type->callbacks.init(system, entry->payload, name);
	}

	return (struct Asset_Handle){
		.instance_handle = instance_handle,
		.type_id = *type_id,
		.name_id = name_id,
	};

	// process errors
	fail: DEBUG_BREAK();
	return (struct Asset_Handle){0};
}

void asset_system_discard(struct Asset_System * system, struct Asset_Handle handle) {
	if (asset_handle_is_null(handle)) {
		logger_to_console("null asset handle\n"); DEBUG_BREAK();
		return;
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, handle.type_id);
	if (asset_type == NULL) {
		struct CString const type = strings_get(&system->strings, handle.type_id);
		logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK();
	}

	//
	hash_table_u32_del(&system->handles, handle.name_id);
	if (asset_type->callbacks.free != NULL) {
		struct Asset_Entry * entry = handle_table_get(&asset_type->instances, handle.instance_handle);
		asset_type->callbacks.free(system, entry->payload);
		handle_table_discard(&asset_type->instances, handle.instance_handle);
	}
}

void * asset_system_find_instance(struct Asset_System * system, struct Asset_Handle handle) {
	if (asset_handle_is_null(handle)) {
		logger_to_console("null asset handle\n"); DEBUG_BREAK();
		return NULL;
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, handle.type_id);
	if (asset_type == NULL) {
		struct CString const type = strings_get(&system->strings, handle.type_id);
		logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK();
	}

	//
	struct Asset_Entry * entry = handle_table_get(&asset_type->instances, handle.instance_handle);
	return entry->payload;
}

void * asset_system_aquire_instance(struct Asset_System * system, struct CString name) {
	struct Asset_Handle const handle = asset_system_aquire(system, name);
	return asset_system_find_instance(system, handle);
}

struct CString asset_system_get_name(struct Asset_System * system, struct Asset_Handle handle) {
	return strings_get(&system->strings, handle.name_id);
}

//

static void asset_system_del_type_internal(struct Asset_System * system, struct Asset_Type * asset_type) {
	FOR_HANDLE_TABLE (&asset_type->instances, it) {
		struct Asset_Entry * entry = it.value;
		hash_table_u32_del(&system->handles, entry->header.name_id);
		if (asset_type->callbacks.free != NULL) {
			asset_type->callbacks.free(system, entry->payload);
		}
	}

	handle_table_free(&asset_type->instances);
	if (asset_type->callbacks.type_free != NULL) {
		asset_type->callbacks.type_free();
	}
}

static uint32_t asset_system_get_extension_from_name(struct CString name) {
	for (uint32_t extension_length = 0; extension_length < name.length; extension_length++) {
		// @todo: make it unicode-aware?
		char const symbol = name.data[name.length - extension_length - 1];
		if (symbol == '.') { return extension_length; }
		if (symbol == '/') { break; }
	}
	return 0;
}
