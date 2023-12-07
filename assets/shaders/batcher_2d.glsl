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

layout(std430, binding = 0) buffer Buffer {
	Instance instances[];
};

out vec4 v_Color;
out vec2 v_TexCoord;

void main() {
	Instance inst = instances[gl_InstanceID];

	vec2 position[6] = {
		inst.quad.xy, inst.quad.zy, inst.quad.zw,
		inst.quad.zw, inst.quad.xw, inst.quad.xy,
	};

	vec2 tex_coords[6] = {
		inst.uv.xy, inst.uv.zy, inst.uv.zw,
		inst.uv.zw, inst.uv.xw, inst.uv.xy,
	};

	v_Color = inst.color;
	v_TexCoord = tex_coords[gl_VertexID];
	gl_Position = vec4(position[gl_VertexID], 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec4 v_Color;
in vec2 v_TexCoord;

uniform vec4 p_Tint;
uniform sampler2D p_Texture;

layout(location = 0) out vec4 out_color;

void main() {
	vec4 texture_pixel = texture(p_Texture, v_TexCoord);
	out_color = texture_pixel * v_Color * p_Tint;
}
#endif
