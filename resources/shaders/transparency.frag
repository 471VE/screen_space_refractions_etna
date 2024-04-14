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

void main()
{
	outColor = vec4(0.f);
	vec3 normalizedRefractedVector = normalize(vOut.refractedVector);
	
	const vec3 wPos = (Params.viewInverse * vec4(texture(positionMap, vOut.outVertexScreenPos.xy).xyz, 1.0)).xyz;
	float sceneViewDepth = (Params.view * vec4(wPos, 1.f)).z;
	float backfaceViewDepth = (Params.view * vec4(vOut.outVertexPos.xyz, 1.f)).z;

	float len = sceneViewDepth - backfaceViewDepth;
	float cosTheta = dot(normalizedRefractedVector, Params.camForward);


	// vec3 vector = wPos - vOut.outVertexPos.xyz;
	// float len = length(vector);
	// vector = normalize(vector);
	// float cosTheta = dot(vector, normalizedRefractedVector);

	vec3 sampleAt = vOut.outVertexPos.xyz + normalizedRefractedVector / cosTheta;
	vec2 sampleAtTexCoord = (Params.proj * Params.view * vec4(sampleAt, 1.f)).xy * 0.5f + 0.5f;

	outColor = vec4(normalizedRefractedVector * 0.5f + 0.5f, 1.f);
	if (sampleAtTexCoord.x < 0.f || sampleAtTexCoord.x > 1.f ||
			sampleAtTexCoord.y < 0.f || sampleAtTexCoord.y > 1.f)
		outColor = texture(environmentMap, normalizedRefractedVector);
	else
	{
		outColor = vec4(texture(frameBeforeTransparency, sampleAtTexCoord.xy).xyz, 1.f);
		// if (texture(albedoMap, sampleAtTexCoord.xy).xyz == vec3(0.f))
		// 	outColor = texture(environmentMap, normalizedRefractedVector);
	}
}
