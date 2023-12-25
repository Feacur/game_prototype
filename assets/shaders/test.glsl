INTERFACE_BLOCK_GLOBAL Global {
	float dummy;
} global;

INTERFACE_BLOCK_CAMERA Camera {
	uvec2 viewport_size;
	// padding
	mat4 view;
	mat4 projection;
	mat4 projection_view;
} camera;




#if defined(VERTEX_SHADER)
ATTRIBUTE_POSITION vec3 a_Position;
ATTRIBUTE_TEXCOORD vec2 a_TexCoord;

uniform mat4 u_Model;

out Vertex {
	vec2 tex_coord;
} vert;

void main() {
	vert.tex_coord = a_TexCoord;
	gl_Position = camera.projection_view * u_Model * vec4(a_Position, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in Vertex {
	vec2 tex_coord;
} vert;

uniform vec4 p_Tint;
uniform sampler2D p_Texture;
uniform vec4 p_Texture_OS;

layout(location = 0) out vec4 frag;

void main() {
	vec4 texture_pixel = texture(p_Texture, vert.tex_coord * p_Texture_OS.zw + p_Texture_OS.xy);
	frag = texture_pixel * p_Tint;
}
#endif
