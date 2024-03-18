#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout (location = 0 ) out VS_OUT
{
  vec2 texCoord;
} vOut;

layout(location = 1) out vec3 forwards;

const vec2 screen_corners[6] = vec2[](
	vec2(-1.f, -1.f),
	vec2(-1.f,  1.f),
	vec2( 1.f,  1.f),
	vec2( 1.f,  1.f),
	vec2( 1.f, -1.f),
	vec2(-1.f, -1.f)
);

void main(void)
{
  vec2 pos = screen_corners[gl_VertexIndex];
	gl_Position = vec4(pos, 0.f, 1.f);
  vOut.texCoord = pos * 0.5f + 0.5f;
  forwards = normalize(Params.camForward + (pos.x * Params.camRight - 9.f / 16.f * pos.y * Params.camUp)).xyz;
}
