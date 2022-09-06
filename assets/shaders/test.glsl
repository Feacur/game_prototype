// @todo: automate locations
#define ATTRIBUTE_TYPE_POSITION 0
#define ATTRIBUTE_TYPE_TEXCOORD 1
#define ATTRIBUTE_TYPE_NORMAL   2
#define ATTRIBUTE_TYPE_COLOR    3

// @todo: create common GLSL code and prove `#include` functionality
#define R32_POS_INFINITY uintBitsToFloat(0x7f800000)

// @todo: push these as defines
#define NS_NCP 1
#define NS_FCP 0

// @todo: push these as uniform data
const float ncp = 0.1, fcp = R32_POS_INFINITY;




#if defined(VERTEX_SHADER)
layout(location = ATTRIBUTE_TYPE_POSITION) in vec3 a_Position;
layout(location = ATTRIBUTE_TYPE_TEXCOORD) in vec2 a_TexCoord;

uniform mat4 u_ProjectionView;
uniform mat4 u_Model;

out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_ProjectionView * u_Model * vec4(a_Position, 1);
}
#endif




#if defined(FRAGMENT_SHADER)
in vec2 v_TexCoord;

uniform vec4 p_Color;
uniform sampler2D p_Texture;
uniform vec4 p_Texture_OS;

uniform uvec2 u_ViewportSize;

layout(location = 0) out vec4 out_color;

float linearize_depth(float depth)
{
	if (isinf(fcp)) { return (NS_NCP - NS_FCP) * ncp / (depth - NS_FCP); }
	return (NS_NCP - NS_FCP) * ncp * fcp / (depth * (fcp - ncp) - (NS_FCP * fcp - NS_NCP * ncp));
}

void main()
{
	vec4 texture_pixel = texture(p_Texture, v_TexCoord * p_Texture_OS.zw + p_Texture_OS.xy);
	float depth = clamp(linearize_depth(gl_FragCoord.z) / 10, 0, 1);
	out_color = texture_pixel * p_Color;
	out_color = mix(out_color, vec4(depth, depth, depth, 1), gl_FragCoord.x / u_ViewportSize.x < 0.5);
}
#endif
