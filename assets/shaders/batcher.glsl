// @todo: automate locations
#define ATTRIBUTE_TYPE_POSITION 0
#define ATTRIBUTE_TYPE_TEXCOORD 1
#define ATTRIBUTE_TYPE_NORMAL   2
#define ATTRIBUTE_TYPE_COLOR    3




#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec2 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;

uniform mat4 u_Camera;
uniform mat4 u_Transform;

out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_Camera * u_Transform * vec4(a_Position, 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

layout(location = 0) out vec4 out_color;

void main()
{
	vec4 texture_pixel = texture(u_Texture, v_TexCoord);
	out_color = (texture_pixel * u_Color);
}
#endif
