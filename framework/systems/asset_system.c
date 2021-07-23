#include "framework/logger.h"

#include <string.h>
#include <stdlib.h>

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
static uint32_t asset_system_get_extension_from_name(uint32_t name_lenth, char const * name);

void asset_system_init(struct Asset_System * system) {
	strings_init(&system->strings);
	hash_table_u32_init(&system->types, sizeof(struct Asset_Type));
	hash_table_u32_init(&system->refs, sizeof(struct Ref));
	hash_table_u32_init(&system->map, sizeof(uint32_t));

	// @note: consider type_id 0 empty
	asset_system_set_type(system, "", (struct Asset_Callbacks){0}, 0);

	// @note: consider resource_id 0 empty
	hash_table_u32_set(&system->refs, 0, &(struct Ref){0});
}

void asset_system_free(struct Asset_System * system) {
	for (struct Hash_Table_U32_Entry it = {0}; hash_table_u32_iterate(&system->types, &it); /*empty*/) {
		struct Asset_Type * asset_type = it.value;
		asset_system_del_type_internal(system, asset_type);
	}

	strings_free(&system->strings);
	hash_table_u32_free(&system->types);
	hash_table_u32_free(&system->refs);
	hash_table_u32_free(&system->map);
}

void asset_system_map_extension(struct Asset_System * system, char const * type_name, char const * extension) {
	uint32_t const type_length = (uint32_t)strlen(type_name);
	uint32_t const extension_length = (uint32_t)strlen(extension);

	uint32_t const type_id = strings_add(&system->strings, type_length, type_name);
	uint32_t const extension_id = strings_add(&system->strings, extension_length, extension);

	hash_table_u32_set(&system->map, extension_id, &type_id);
}

void asset_system_set_type(struct Asset_System * system, char const * type_name, struct Asset_Callbacks callbacks, uint32_t value_size) {
	struct Asset_Type asset_type = {
		.callbacks = callbacks,
	};
	ref_table_init(&asset_type.instances, SIZE_OF_MEMBER(struct Asset_Entry, header) + value_size);

	{
		// @note: consider ref.id 0 empty
		ref_table_aquire(&asset_type.instances, NULL);
	}

	uint32_t const type_length = (uint32_t)strlen(type_name);
	uint32_t const type_id = strings_add(&system->strings, type_length, type_name);
	hash_table_u32_set(&system->types, type_id, &asset_type);
}

void asset_system_del_type(struct Asset_System * system, char const * type_name) {
	uint32_t const type_length = (uint32_t)strlen(type_name);
	uint32_t const type_id = strings_find(&system->strings, type_length, type_name);
	if (type_id == INDEX_EMPTY || type_id == 0) {
		logger_to_console("unknown type: %*s\n", type_length, type_name); DEBUG_BREAK();
		return;
	}

	{
		struct Asset_Type * asset_type = hash_table_u32_get(&system->types, type_id);
		asset_system_del_type_internal(system, asset_type);
	}

	hash_table_u32_del(&system->types, type_id);
}

struct Asset_Ref asset_system_aquire(struct Asset_System * system, char const * name) {
	uint32_t const name_lenth = (uint32_t)strlen(name);

	//
	uint32_t const extension_length = asset_system_get_extension_from_name(name_lenth, name);
	if (extension_length == 0) {
		logger_to_console("no extension: %*s\n", name_lenth, name); DEBUG_BREAK();
		return (struct Asset_Ref){0};
	}

	char const * extension_name = name + (name_lenth - extension_length);

	//
	uint32_t const extension_id = strings_find(&system->strings, extension_length, extension_name);
	if (extension_id == INDEX_EMPTY || extension_id == 0) {
		logger_to_console("unknown extension: %*s\n", extension_length, extension_name); DEBUG_BREAK();
		return (struct Asset_Ref){0};
	}

	uint32_t const * type_id = hash_table_u32_get(&system->map, extension_id);
	if (type_id == NULL) {
		logger_to_console("can't infer type from extension: %*s\n", extension_length, extension_name); DEBUG_BREAK();
		return (struct Asset_Ref){0};
	}

	//
	uint32_t resource_id = strings_add(&system->strings, name_lenth, name);
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
		char const * type_name = strings_get(&system->strings, *type_id);
		uint32_t const type_length = strings_get_length(&system->strings, *type_id);
		logger_to_console("unknown type: %*s\n", type_length, type_name); DEBUG_BREAK();
		return (struct Asset_Ref){0};
	}

	//
	struct Ref const instance_ref = ref_table_aquire(&asset_type->instances, NULL);
	hash_table_u32_set(&system->refs, resource_id, &instance_ref);

	struct Asset_Entry * entry = ref_table_get(&asset_type->instances, instance_ref);
	entry->header.resource_id = resource_id;
	if (asset_type->callbacks.init != NULL) {
		asset_type->callbacks.init(entry->payload, name);
	}

	return (struct Asset_Ref){
		.instance_ref = instance_ref,
		.type_id = *type_id,
		.resource_id = resource_id,
	};
}

void asset_system_discard(struct Asset_System * system, struct Asset_Ref asset_ref) {
	if (asset_ref.type_id == INDEX_EMPTY || asset_ref.type_id == 0) {
		logger_to_console("unknown type"); DEBUG_BREAK(); return;
	}

	if (asset_ref.resource_id == INDEX_EMPTY) {
		logger_to_console("unknown resource"); DEBUG_BREAK(); return;
	}

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, asset_ref.type_id);
	if (asset_type == NULL) {
		char const * type_name = strings_get(&system->strings, asset_ref.type_id);
		uint32_t const type_length = strings_get_length(&system->strings, asset_ref.type_id);
		logger_to_console("unknown type: %*s\n", type_length, type_name); DEBUG_BREAK();
	}

	//
	if (asset_type->callbacks.free != NULL) {
		struct Asset_Entry * entry = ref_table_get(&asset_type->instances, asset_ref.instance_ref);
		// if (entry->header.resource_id != asset_ref.resource_id) { DEBUG_BREAK(); return; }
		asset_type->callbacks.free(entry->payload);
	}
}

void * asset_system_get_instance(struct Asset_System * system, struct Asset_Ref asset_ref) {
	if (asset_ref.type_id == INDEX_EMPTY || asset_ref.type_id == 0) { return NULL; }
	if (asset_ref.resource_id == INDEX_EMPTY)                       { return NULL; }

	//
	struct Asset_Type * asset_type = hash_table_u32_get(&system->types, asset_ref.type_id);
	if (asset_type == NULL) {
		char const * type_name = strings_get(&system->strings, asset_ref.type_id);
		uint32_t const type_length = strings_get_length(&system->strings, asset_ref.type_id);
		logger_to_console("unknown type: %*s\n", type_length, type_name); DEBUG_BREAK();
	}

	//
	struct Asset_Entry * entry = ref_table_get(&asset_type->instances, asset_ref.instance_ref);
	// if (entry->header.resource_id != asset_ref.resource_id) { DEBUG_BREAK(); return NULL; }
	return entry->payload;
}

void * asset_system_find_instance(struct Asset_System * system, char const * name) {
	struct Asset_Ref const asset_ref = asset_system_aquire(system, name);
	return asset_system_get_instance(system, asset_ref);
}

//

static void asset_system_del_type_internal(struct Asset_System * system, struct Asset_Type * asset_type) {
	// @note: consider ref.id 0 empty
	for (uint32_t i = 1; i < asset_type->instances.count; i++) {
		struct Asset_Entry * entry = ref_table_value_at(&asset_type->instances, i);
		hash_table_u32_del(&system->refs, entry->header.resource_id);
		if (asset_type->callbacks.free != NULL) {
			asset_type->callbacks.free(entry->payload);
		}
	}

	ref_table_free(&asset_type->instances);
}

static uint32_t asset_system_get_extension_from_name(uint32_t name_lenth, char const * name) {
	uint32_t type_length = 0;
	for (; type_length < name_lenth; type_length++) {
		// @todo: make it unicode-aware?
		char const symbol = name[name_lenth - type_length - 1];
		if (symbol == '.') { return type_length; }
		if (symbol == '/') { break; }
	}
	return 0;
}
