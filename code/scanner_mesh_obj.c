#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
#include "scanner_mesh_obj.h"

void scanner_mesh_obj_init(struct Scanner_Mesh_Obj * scanner, char const * text) {
	*scanner = (struct Scanner_Mesh_Obj){
		.start = text,
		.current = text,
		.line = 0,
	};
}

void scanner_mesh_obj_free(struct Scanner_Mesh_Obj * scanner) {
	scanner_mesh_obj_init(scanner, NULL);
}

struct Mesh_Obj_Token scanner_mesh_obj_next(struct Scanner_Mesh_Obj * scanner) {
	(void)scanner;
	return (struct Mesh_Obj_Token){
		.type = MESH_OBJ_TOKEN_ERROR,
	};
}
