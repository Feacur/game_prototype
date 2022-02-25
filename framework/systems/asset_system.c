#include "framework/logger.h"

//
#include "asset_system.h"

struct Asset_Type {
	struct Asset_Callbacks callbacks;
	struct Ref_Table instances;
};

struct Asset_Entry {
	struct {
		uint32_t resource_id;
	} header;
	uint8_t payload[FLEXIBLE_ARRAY];
};

static void asset_system_del_type_internal(struct Asset_System * system, struct Asset_Type * asset_type);
static uint32_t asset_system_get_extension_from_name(struct CString name);

void asset_system_init(struct Asset_System * system) {
	strings_init(&system->strings);
	hash_table_u32_init(&system->types, sizeof(struct Asset_Type));
	hash_table_u32_init(&system->refs, sizeof(struct Ref));
	hash_table_u32_init(&system->map, sizeof(uint32_t));

	{
		// @note: consider `type_id == 0` and `resource_id == 0` empty
		uint32_t const id = strings_add(&system->strings, S_EMPTY);
		hash_table_u32_set(&system->types, id, &(struct Asset_Type){0});
		hash_table_u32_set(&system->refs, id, &(struct Ref){0});
	}
}

void asset_system_free(struct Asset_System * system) {
	for (struct Hash_Table_U32_Iterator it = {0}; hash_table_u32_iterate(&system->types, &it); /*empty*/) {
		struct Asset_Type * asset_type = it.value;
		asset_system_del_type_internal(system, asset_type);
	}

	strings_free(&system->strings);
	hash_table_u32_free(&system->types);
	hash_table_u32_free(&system->refs);
	hash_table_u32_free(&system->map);
}

void asset_system_map_extension(struct Asset_System * system, struct CString type, struct CString extension) {
	if (type.length == 0) { logger_to_console("empty type"); DEBUG_BREAK(); return; }

	if (extension.length == 0) { logger_to_console("empty extension"); DEBUG_BREAK(); return; }

	uint32_t const type_id = strings_add(&system->strings, type);
	uint32_t const extension_id = strings_add(&system->strings, extension);

	hash_table_u32_set(&system->map, extension_id, &type_id);
}

bool asset_system_match_type(struct Asset_System * system, struct Asset_Ref asset_ref, struct CString type_name) {
	return asset_ref.type_id == strings_find(&system->strings, type_name);
}

void asset_system_set_type(struct Asset_System * system, struct CString type, struct Asset_Callbacks callbacks, uint32_t value_size) {
	if (type.length == 0) { logger_to_console("empty type"); DEBUG_BREAK(); return; }

	if (callbacks.type_init != NULL) {
		callbacks.type_init();
	}

	struct Asset_Type asset_type = {
		.callbacks = callbacks,
	};

	ref_table_init(&asset_type.instances, SIZE_OF_MEMBER(struct Asset_Entry, header) + value_size);

	{
		// @note: consider `ref.id == 0` empty
		ref_table_aquire(&asset_type.instances, NULL);
	}

	uint32_t const type_id = strings_add(&system->strings, type);

	hash_table_u32_set(&system->types, type_id, &asset_type);
}

void asset_system_del_type(struct Asset_System * system, struct CString type) {
	if (type.length == 0) { logger_to_console("empty type"); DEBUG_BREAK(); return; }

	uint32_t const type_id = strings_find(&system->strings, type);
	if (type_id == INDEX_EMPTY) { logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK(); return; }

	{
		struct Asset_Type * asset_type = hash_table_u32_get(&system->types, type_id);
		asset_system_del_type_internal(system, asset_type);
	}

	hash_table_u32_del(&system->types, type_id);
}

struct Asset_Ref asset_system_aquire(struct Asset_System * system, struct CString name) {
	if (name.length == INDEX_EMPTY) { logger_to_console("empty name"); DEBUG_BREAK(); return с_asset_ref_empty; }

	//
	uint32_t const extension_length = asset_system_get_extension_from_name(name);
	if (extension_length == 0) { logger_to_console("empty extension"); DEBUG_BREAK(); return с_asset_ref_empty; }

	char const * extension_name = name.data + (name.length - extension_length);

	//
	uint32_t const extension_id = strings_find(&system->strings, (struct CString){
		.length = extension_length,
		.data = extension_name,
	});
	if (extension_id == INDEX_EMPTY) {
		logger_to_console("unknown extension: %.*s\n", extension_length, extension_name); DEBUG_BREAK();
		return с_asset_ref_empty;
	}

	uint32_t const * type_id = hash_table_u32_get(&system->map, extension_id);
	if (type_id == NULL) {
		logger_to_console("can't infer type from extension: %.*s\n", extension_length, extension_name); DEBUG_BREAK();
		return с_asset_ref_empty;
	}

	//
	uint32_t resource_id = strings_add(&system->strings, name);
	struct Ref const * instance_ref_ptr = hash_table_u32_get(&system->refs, resource_id);
	if (instance_ref_ptr != NULL) {
		return (struct Asset_Ref){
			.instance_ref = *instance_ref_ptr,
			.type_id = *type_id,
			.resource_id = resource_id,
		};
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, *type_id);
	if (asset_type == NULL) {
		struct CString const type = strings_get(&system->strings, *type_id);
		logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK();
		return с_asset_ref_empty;
	}

	//
	struct Ref const instance_ref = ref_table_aquire(&asset_type->instances, NULL);
	hash_table_u32_set(&system->refs, resource_id, &instance_ref);

	struct Asset_Entry * entry = ref_table_get(&asset_type->instances, instance_ref);
	entry->header.resource_id = resource_id;

	if (asset_type->callbacks.init != NULL) {
		asset_type->callbacks.init(system, entry->payload, name);
	}

	return (struct Asset_Ref){
		.instance_ref = instance_ref,
		.type_id = *type_id,
		.resource_id = resource_id,
	};
}

void asset_system_discard(struct Asset_System * system, struct Asset_Ref asset_ref) {
	if (asset_ref.type_id == с_asset_ref_empty.type_id) {
		logger_to_console("unknown type"); DEBUG_BREAK();
		return;
	}

	if (asset_ref.resource_id == с_asset_ref_empty.resource_id) {
		logger_to_console("unknown resource"); DEBUG_BREAK();
		return;
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, asset_ref.type_id);
	if (asset_type == NULL) {
		struct CString const type = strings_get(&system->strings, asset_ref.type_id);
		logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK();
	}

	//
	hash_table_u32_del(&system->refs, asset_ref.resource_id);
	if (asset_type->callbacks.free != NULL) {
		struct Asset_Entry * entry = ref_table_get(&asset_type->instances, asset_ref.instance_ref);
		asset_type->callbacks.free(system, entry->payload);
		ref_table_discard(&asset_type->instances, asset_ref.instance_ref);
	}
}

void * asset_system_find_instance(struct Asset_System * system, struct Asset_Ref asset_ref) {
	if (asset_ref.type_id == с_asset_ref_empty.type_id) {
		logger_to_console("unknown type"); DEBUG_BREAK(); return NULL;
	}

	if (asset_ref.resource_id == с_asset_ref_empty.resource_id) {
		logger_to_console("unknown resource"); DEBUG_BREAK(); return NULL;
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, asset_ref.type_id);
	if (asset_type == NULL) {
		struct CString const type = strings_get(&system->strings, asset_ref.type_id);
		logger_to_console("unknown type: %.*s\n", type.length, type.data); DEBUG_BREAK();
	}

	//
	struct Asset_Entry * entry = ref_table_get(&asset_type->instances, asset_ref.instance_ref);
	return entry->payload;
}

struct CString asset_system_get_name(struct Asset_System * system, struct Asset_Ref asset_ref) {
	return strings_get(&system->strings, asset_ref.resource_id);
}

void * asset_system_aquire_instance(struct Asset_System * system, struct CString name) {
	struct Asset_Ref const asset_ref = asset_system_aquire(system, name);
	return asset_system_find_instance(system, asset_ref);
}

//

static void asset_system_del_type_internal(struct Asset_System * system, struct Asset_Type * asset_type) {
	// @note: consider `ref.id == 0` empty
	for (struct Ref_Table_Iterator it = {.next = 1}; ref_table_iterate(&asset_type->instances, &it); /*empty*/) {
		struct Asset_Entry * entry = it.value;
		hash_table_u32_del(&system->refs, entry->header.resource_id);
		if (asset_type->callbacks.free != NULL) {
			asset_type->callbacks.free(system, entry->payload);
		}
	}

	ref_table_free(&asset_type->instances);
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
