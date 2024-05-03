INTERFACE_BLOCK_GLOBAL Global {
	float dummy;
} global;

INTERFACE_BLOCK_CAMERA Camera {
	uvec2 viewport_size;
	// padding to align `.view.x` at 4 floats
	mat4  view;
	mat4  projection;
	mat4  projection_view;
} camera;




#if defined(VERTEX_SHADER)

// quad layout
// 4-----------3 2
// |         / / |
// |       / /   |
// |     / /     |
// |   / /       |
// | / /         |
// 5 0-----------1

struct Instance {
	vec4 quad;
	vec4 uv;
	vec4 color;
	uint flags;
	// padding to align `.quad` at 4 floats
};

INTERFACE_BLOCK_DYNAMIC Dynamic {
	Instance instances[];
} dyn;

out Vertex {
	uint flags;
	vec4 color;
	vec2 tex_coord;
} vert;

void main() {
	Instance inst = dyn.instances[gl_InstanceID];

	vec2 position[6] = {
		inst.quad.xy, inst.quad.zy, inst.quad.zw,
		inst.quad.zw, inst.quad.xw, inst.quad.xy,
	};

	vec2 tex_coords[6] = {
		inst.uv.xy, inst.uv.zy, inst.uv.zw,
		inst.uv.zw, inst.uv.xw, inst.uv.xy,
	};

	vert.flags = inst.flags;
	vert.color = inst.color;
	vert.tex_coord = tex_coords[gl_VertexID];
	gl_Position = vec4(position[gl_VertexID], 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in Vertex {
	flat uint flags;
	vec4 color;
	vec2 tex_coord;
} vert;

uniform vec4 p_Tint;
uniform sampler2D p_Font;
uniform sampler2D p_Image;

layout(location = 0) out vec4 frag;

void main() {
	vec4 texture_pixel = (bool(vert.flags & BATCHER_FLAG_FONT))
		? texture(p_Font, vert.tex_coord)
		: texture(p_Image, vert.tex_coord);
	// if (texture_pixel.a == 0) { discard; }
	frag = texture_pixel * vert.color * p_Tint;
}
#endif
