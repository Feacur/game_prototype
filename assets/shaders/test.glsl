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

float linearize_depth(float ndc_fraction) {
	float ndc = mix(NDC_NEAR, NDC_FAR, ndc_fraction);
	float scale = camera.projection[2][2];
	float offset = camera.projection[3][2];
	float ortho = camera.projection[3][3];
	return mix(offset / (ndc - scale), scale / (ndc - offset), ortho);
}

float visualize_depth(float depth, float factor) {
	float ndc_fraction = (depth - DEPTH_NEAR) / (DEPTH_FAR - DEPTH_NEAR);
	// return linearize_depth(ndc_fraction) / factor;
	return (1 - ndc_fraction) * factor;
}

void main() {
	vec4 texture_pixel = texture(p_Texture, vert.tex_coord * p_Texture_OS.zw + p_Texture_OS.xy);
	float depth = visualize_depth(gl_FragCoord.z, 20);
	frag = texture_pixel * p_Tint;
	frag = mix(frag, vec4(depth, depth, depth, 1), gl_FragCoord.x / camera.viewport_size.x < 0.5);
}
#endif
