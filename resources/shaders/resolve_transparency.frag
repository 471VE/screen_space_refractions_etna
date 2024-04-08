#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 out_fragColor;

layout (location = 0) in VS_OUT
{
  vec2 texCoord;
} vsOut;

layout (binding = 0) uniform sampler2D frameBeforeTransparency;
layout (binding = 1) uniform sampler2D frameTransparencyOnly;

void main()
{
  out_fragColor = texture(frameTransparencyOnly, vsOut.texCoord);
  if (out_fragColor.xyz == vec3(0.f))
    out_fragColor = texture(frameBeforeTransparency, vsOut.texCoord);
}