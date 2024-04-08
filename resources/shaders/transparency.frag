#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.h"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in float width;
layout(location = 2) in vec3 refractedVector;
layout(location = 3) in vec4 outVertexScreenPos;
layout(location = 4) in vec3 rayDirection;
layout(location = 5) in vec4 outVertexPos;
layout(location = 6) in vec4 inVertexPos;

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
	vec3 normalizedRefractedVector = normalize(refractedVector);
	
	const vec3 wPos = (Params.viewInverse * vec4(texture(positionMap, outVertexScreenPos.xy).xyz, 1.0)).xyz;

	vec3 vector = wPos - outVertexPos.xyz;
	float len = length(vector);
	vector = normalize(vector);
	float cosTheta = dot(vector, normalizedRefractedVector);

	vec3 sampleAt = outVertexPos.xyz + normalizedRefractedVector;
	vec2 sampleAtTexCoord = (Params.proj * Params.view * vec4(sampleAt, 1.f)).xy * 0.5f + 0.5f;

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
