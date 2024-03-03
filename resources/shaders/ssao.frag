#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout(location = 0) out float out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} vsOut;

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout (binding = 1) uniform sampler2D gPosition;
layout (binding = 2) uniform sampler2D gNormal;
layout (binding = 3) buffer ssaoSamples
{
    vec4 samples[];
};
layout (binding = 4) buffer ssaoNoise
{
    vec4 ssaoNoiseVector[];
};

vec3 sample_noise(vec2 coord)
{
  vec2 noiseCoord = mod(vec2(coord.x * 1024, coord.y * 1024), Params.ssaoNoiseSize - 1);
  return  ssaoNoiseVector[uint(noiseCoord.x * Params.ssaoNoiseSize + noiseCoord.y)].xyz;
}

void main()
{
  vec3 fragPos = texture(gPosition, vsOut.texCoord).xyz;
  vec3 randomVec  = sample_noise(vsOut.texCoord);
  vec3 normal    = texture(gNormal, vsOut.texCoord).xyz;
  vec3 tangent   = normalize(randomVec  - normal * dot(randomVec , normal));
  vec3 bitangent = cross(normal, tangent);
  mat3 TBN = mat3(tangent, bitangent, normal);
  float occlusion = 0.f;
  for (int i = 0; i < Params.ssaoKernelSize; ++i)
  {
    vec3 ssaoSample = TBN * samples[i].xyz;
    ssaoSample = fragPos + ssaoSample * Params.ssaoRadius;
    vec4 offset = Params.proj * vec4(ssaoSample, 1.f);
    offset.xyz /= offset.w;
    offset.xyz = offset.xyz * 0.5 + 0.5;
    float sampleDepth = texture(gPosition, offset.xy).z;
    float rangeCheck = smoothstep(0.0, 1.0, Params.ssaoRadius / abs(fragPos.z - sampleDepth));
    occlusion += (sampleDepth >= ssaoSample.z + 0.025 ? 1.0 : 0.0) * rangeCheck;
  }
  occlusion = 1.0 - (occlusion / Params.ssaoKernelSize);
  out_fragColor = occlusion;  
}