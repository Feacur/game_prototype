#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec2 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main() {
	v_TexCoord = a_TexCoord;
	gl_Position = vec4(a_Position, 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;

uniform vec4 p_Color;
uniform sampler2D p_Texture;

layout(location = 0) out vec4 out_color;

void main() {
	vec4 texture_pixel = texture(p_Texture, v_TexCoord);
	out_color = texture_pixel * p_Color;
}
#endif
