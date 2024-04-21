#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.h"

layout (location = 0) in VS_OUT
{ 
  vec3 fragNormal;
  float width;
  vec3 refractedVector;
  vec4 outVertexScreenPos;
  vec3 rayDirection;
  vec4 outVertexPos;
  vec4 inVertexPos;
} vOut;

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout (binding = 1) uniform sampler2D frameBeforeTransparency;
layout (binding = 2) uniform sampler2D positionMap;
layout (binding = 3) uniform sampler2D albedoMap;
layout (binding = 4) uniform samplerCube environmentMap;

layout(location = 0) out vec4 outColor;

float profile1d(float x)
{
	return max(0.f, min(1.f, 1.f / Params.screenSpaceBlendingWidth * (0.5f - abs(x - 0.5f))));
}

float profile2d(vec2 p)
{
	return smoothstep(0.f, 1.f, min(profile1d(p.x), profile1d(p.y)));
}

void main()
{
	outColor = vec4(0.f);
	vec3 normalizedRefractedVector = normalize(vOut.refractedVector);

	float cosTheta = dot(normalizedRefractedVector, Params.camForward);

	vec3 sampleAt = vOut.outVertexPos.xyz + normalizedRefractedVector / cosTheta;
	vec2 sampleAtTexCoord = (Params.proj * Params.view * vec4(sampleAt, 0.f)).xy * 0.5f + 0.5f;

	vec4 environmentColor = vec4(0.f);
	vec4 screenSpaceColor = vec4(0.f);

	if (sampleAtTexCoord.x < Params.screenSpaceBlendingWidth || sampleAtTexCoord.x > 1.f - Params.screenSpaceBlendingWidth ||
			sampleAtTexCoord.y < Params.screenSpaceBlendingWidth || sampleAtTexCoord.y > 1.f - Params.screenSpaceBlendingWidth)
		environmentColor = texture(environmentMap, normalizedRefractedVector);

	if (0.f <= sampleAtTexCoord.x && sampleAtTexCoord.x <= 1.f &&
			0.f <= sampleAtTexCoord.y && sampleAtTexCoord.y <= 1.f)
		screenSpaceColor = texture(frameBeforeTransparency, sampleAtTexCoord.xy);

	float screenSpaceStrength = profile2d(sampleAtTexCoord);

	outColor = screenSpaceColor * screenSpaceStrength + environmentColor * (1.f - screenSpaceStrength);
}
