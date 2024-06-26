#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout (location = 0) out vec4 frameBeforeTransparency;

layout (location = 0) in VS_OUT
{
  vec2 texCoord;
} vsOut;

layout(location = 1) in vec3 forwards;

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout (binding = 1) uniform sampler2D shadowMap;
layout (binding = 2) uniform sampler2D positionMap;
layout (binding = 3) uniform sampler2D normalMap;
layout (binding = 4) uniform sampler2D albedoMap;
layout (binding = 5) uniform sampler2D ssaoMap;
// layout (binding = 6) uniform sampler2D backgroundTexture;
layout (binding = 7) uniform samplerCube environmentMap;

void main()
{
  const vec3 wPos = (Params.viewInverse * vec4(texture(positionMap, vsOut.texCoord).xyz, 1.0)).xyz;
  const vec4 posLightClipSpace = Params.lightMatrix*vec4(wPos, 1.0f);
  const vec3 posLightSpaceNDC  = posLightClipSpace.xyz/posLightClipSpace.w;    // for orto matrix, we don't need perspective division, you can remove it if you want; this is general case;
  vec2 shadowTexCoord    = posLightSpaceNDC.xy*0.5f + vec2(0.5f, 0.5f);  // just shift coords from [-1,1] to [0,1]               

  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0091f || shadowTexCoord.y > 0.9999f);
  const float shadow    = ((posLightSpaceNDC.z < textureLod(shadowMap, shadowTexCoord, 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;

  const vec4 lightColor1 = vec4(1.f, 1.f, 1.f, 1.f);

  const vec3 normal = (Params.viewInverse * texture(normalMap, vsOut.texCoord)).xyz;
  vec4 albedo = texture(albedoMap, vsOut.texCoord);
  if (albedo.xyz == vec3(0.f))
    frameBeforeTransparency = texture(environmentMap, forwards);
  else
  {
    float occlusion = Params.ssaoEnabled ? texture(ssaoMap, vsOut.texCoord).r : 1.f;
    vec3 lightDir = normalize(Params.lightPos - wPos);
    vec4 lightColor = max(dot(normal, lightDir), 0.0f) * lightColor1;
    frameBeforeTransparency = (lightColor * shadow + vec4(0.4f) * occlusion) * albedo;
  }
}