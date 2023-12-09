layout (std140, binding = 0) uniform Global {
	float dummy;
} global;

layout (std140, binding = 1) uniform Camera {
	uvec2 viewport_size;
	// padding
	mat4 view;
	mat4 projection;
	mat4 projection_view;
} camera;




#if defined(VERTEX_SHADER)

// quad layout
// 4-----------3  2
// |         /  / |
// |       /  /   |
// |     /  /     |
// |   /  /       |
// | /  /         |
// 5  0-----------1

struct Instance {
	vec4 quad;
	vec4 uv;
	vec4 color;
};

layout(std430, binding = 2) buffer Buffer {
	Instance instances[];
} buff;

out Vertex {
	vec4 color;
	vec2 tex_coord;
} vert;

void main() {
	Instance inst = buff.instances[gl_InstanceID];

	vec2 position[6] = {
		inst.quad.xy, inst.quad.zy, inst.quad.zw,
		inst.quad.zw, inst.quad.xw, inst.quad.xy,
	};

	vec2 tex_coords[6] = {
		inst.uv.xy, inst.uv.zy, inst.uv.zw,
		inst.uv.zw, inst.uv.xw, inst.uv.xy,
	};

	vert.color = inst.color;
	vert.tex_coord = tex_coords[gl_VertexID];
	gl_Position = vec4(position[gl_VertexID], 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in Vertex {
	vec4 color;
	vec2 tex_coord;
} vert;

uniform vec4 p_Tint;
uniform sampler2D p_Texture;

layout(location = 0) out vec4 frag;

void main() {
	vec4 texture_pixel = texture(p_Texture, vert.tex_coord);
	frag = texture_pixel * vert.color * p_Tint;
}
#endif
