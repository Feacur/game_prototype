#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec2 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;
layout(location = ATTRIBUTE_TYPE_COLOR) in vec4 a_Color;

out vec2 v_TexCoord;
out vec4 v_Color;

void main() {
	v_TexCoord = a_TexCoord;
	v_Color = a_Color;
	gl_Position = vec4(a_Position, 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;
in vec4 v_Color;

uniform vec4 p_Tint;
uniform sampler2D p_Texture;

layout(location = 0) out vec4 out_color;

void main() {
	vec4 texture_pixel = texture(p_Texture, v_TexCoord);
	out_color = texture_pixel * v_Color * p_Tint;
}
#endif
