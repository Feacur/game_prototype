#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec3 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;

uniform mat4 u_ProjectionView;
uniform mat4 u_Model;

out vec2 v_TexCoord;

void main() {
	v_TexCoord = a_TexCoord;
	gl_Position = u_ProjectionView * u_Model * vec4(a_Position, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;

uniform vec4 p_Color;
uniform sampler2D p_Texture;
uniform vec4 p_Texture_OS;

uniform mat4 u_Projection;
uniform uvec2 u_ViewportSize;

layout(location = 0) out vec4 out_color;

float linearize_depth(float ndc_fraction) {
	float ndc = mix(NDC_NEAR, NDC_FAR, ndc_fraction);
	float scale = u_Projection[2][2];
	float offset = u_Projection[3][2];
	float ortho = u_Projection[3][3];
	return mix(offset / (ndc - scale), scale / (ndc - offset), ortho);
}

float visualize_depth(float depth, float factor) {
	float ndc_fraction = (depth - DEPTH_NEAR) / (DEPTH_FAR - DEPTH_NEAR);
	// return linearize_depth(ndc_fraction) / factor;
	return (1 - ndc_fraction) * factor;
}

void main() {
	vec4 texture_pixel = texture(p_Texture, v_TexCoord * p_Texture_OS.zw + p_Texture_OS.xy);
	float depth = visualize_depth(gl_FragCoord.z, 20);
	out_color = texture_pixel * p_Color;
	out_color = mix(out_color, vec4(depth, depth, depth, 1), gl_FragCoord.x / u_ViewportSize.x < 0.5);
}
#endif
