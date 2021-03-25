#if defined(VERTEX_SHADER)
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_Camera;
uniform mat4 u_Transform;

out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_Camera * u_Transform * vec4(a_Position, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;

uniform vec4 u_Color;
uniform sampler2D u_Font;

layout(location = 0) out vec4 out_color;

void main()
{
	float value = texture(u_Font, v_TexCoord).r;
	out_color = vec4(u_Color.rgb, u_Color.a * value);
}
#endif