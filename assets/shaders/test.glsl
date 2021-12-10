// @todo: automate locations
#define ATTRIBUTE_TYPE_POSITION 0
#define ATTRIBUTE_TYPE_TEXCOORD 1
#define ATTRIBUTE_TYPE_NORMAL   2
#define ATTRIBUTE_TYPE_COLOR    3




#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec3 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;

uniform mat4 u_Projection;
uniform mat4 u_InverseCamera;
uniform mat4 u_Transform;

out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_Projection * u_InverseCamera * u_Transform * vec4(a_Position, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = texture(u_Texture, v_TexCoord) * u_Color;
}
#endif
