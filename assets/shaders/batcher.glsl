// @todo: automate locations
#define ATTRIBUTE_TYPE_POSITION 0
#define ATTRIBUTE_TYPE_TEXCOORD 1
#define ATTRIBUTE_TYPE_NORMAL   2
#define ATTRIBUTE_TYPE_COLOR    3
#define ATTRIBUTE_TYPE_MIXER    4




#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec2 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;
// layout(location = ATTRIBUTE_TYPE_MIXER)    in vec2 a_Mixer;

uniform mat4 u_Camera;
uniform mat4 u_Transform;

out vec2 v_TexCoord;
// out vec2 v_Mixer;

void main()
{
	v_TexCoord = a_TexCoord;
	// v_Mixer = a_Mixer;
	gl_Position = u_Camera * u_Transform * vec4(a_Position, 0, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;
// in vec2 v_Mixer;

uniform vec4 u_Color;
uniform sampler2D u_Texture;

layout(location = 0) out vec4 out_color;

void main()
{
	vec4 texture_pixel = texture(u_Texture, v_TexCoord);
	out_color = (texture_pixel * u_Color);
	// out_color = (texture_pixel * u_Color) * v_Mixer.x
	//           + vec4(u_Color.rgb, texture_pixel.r * u_Color.a) * v_Mixer.y;
}
#endif
