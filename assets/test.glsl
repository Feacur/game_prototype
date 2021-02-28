#if defined(VERTEX_SHADER)
layout(location = 0) in vec3 a_Position;
// layout(location = 1) in vec4 a_Color;

// out vec4 v_Color;

void main()
{
	// v_Color = a_Color;
	gl_Position = vec4(a_Position, 1.0);
}
#endif




#if defined(FRAGMENT_SHADER)
// in vec4 v_Color;

uniform vec4 u_Color = vec4(1, 1, 1, 1);

layout(location = 0) out vec4 color;

void main()
{
	color = u_Color;
}
#endif
