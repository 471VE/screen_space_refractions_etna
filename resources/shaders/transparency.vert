#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.h"
#include "../../src/samples/shadowmap/spherical_harmonics.h"

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

layout(location = 2) in vec3 sphCoeffsWidth1;
layout(location = 3) in vec3 sphCoeffsWidth2;
layout(location = 4) in vec3 sphCoeffsWidth3;

layout(location = 5) in vec3 sphCoeffsX1;
layout(location = 6) in vec3 sphCoeffsX2;
layout(location = 7) in vec3 sphCoeffsX3;

layout(location = 8) in vec3 sphCoeffsY1;
layout(location = 9) in vec3 sphCoeffsY2;
layout(location = 10) in vec3 sphCoeffsY3;

layout(location = 11) in vec3 sphCoeffsZ1;
layout(location = 12) in vec3 sphCoeffsZ2;
layout(location = 13) in vec3 sphCoeffsZ3;

layout (location = 0) out VS_OUT
{ 
  vec3 fragNormal;
  float width;
  vec3 refractedVector;
  vec4 outVertexScreenPos;
  vec3 rayDirection;
  vec4 outVertexPos;
  vec4 inVertexPos;
} vOut;

#define UP vec3(0.f, 1.f, 0.f)

float reconstruct_from_sh(vec3 rd, vec3 n, vec3 sphCoeffs1, vec3 sphCoeffs2, vec3 sphCoeffs3)
{
  // Constructing right-handed orthonormal basis.
  // Again, look at how z-axis is UP direction in the local coordinate system, and not y-direction.
  // This was done to preserve the common spherical harmonics definition for the sake of confusion avoidance.
  vec3 x_axis = (abs(dot(UP, n)) == 1.f) ? vec3(1.f, 0.f, 0.f) : cross(UP, n);
  vec3 y_axis = cross(n, x_axis);
  mat3 transform = mat3(x_axis, y_axis, n);
  
  // Here we go from object reference frame to vertex reference frame
  vec3 localDirection = rd * transform;
  return sphCoeffs1.x * Y00 (localDirection)

       + sphCoeffs1.y * Y1m1(localDirection)
       + sphCoeffs1.z * Y10 (localDirection)
       + sphCoeffs2.x * Y11 (localDirection)

       + sphCoeffs2.y * Y2m2(localDirection)
       + sphCoeffs2.z * Y2m1(localDirection)
       + sphCoeffs3.x * Y20 (localDirection)
       + sphCoeffs3.y * Y21 (localDirection)
       + sphCoeffs3.z * Y22 (localDirection);
}

vec3 refract_safe(vec3 I, vec3 N, float eta)
{
  vec3 R = refract(I, N, eta);
  return dot(R, R) != 0.f ? normalize(R) : R;
}

mat4 MOVE_TRANSFORM = mat4(
  1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 1, 0,
  0, 0, 5, 1
);

void main()
{
  vec4 currentVertexPos = /*ObjectData.model[gl_InstanceIndex] * */ MOVE_TRANSFORM * vec4(vertexPosition, 1.f);
	gl_Position = Params.proj * Params.view * currentVertexPos;
  vOut.inVertexPos = gl_Position;
	vOut.fragNormal = normalize((/*ObjectData.model[gl_InstanceIndex] * */ MOVE_TRANSFORM * vec4(vertexNormal, 0.f)).xyz);
	vec3 rayDirection = normalize(currentVertexPos.xyz - Params.camPosition.xyz);

	vec3 inRayDirection = refract_safe(rayDirection, vOut.fragNormal, 1.f / IOR);
  vOut.width = reconstruct_from_sh(inRayDirection, -vOut.fragNormal, sphCoeffsWidth1, sphCoeffsWidth2, sphCoeffsWidth3);
  
  vec4 outVertexPos = currentVertexPos;
  outVertexPos.xyz += inRayDirection * vOut.width;
  vec4 outVertexScreenPos = Params.proj * Params.view * outVertexPos * 0.5f + 0.5f;

  vOut.refractedVector.x = reconstruct_from_sh(inRayDirection, -vOut.fragNormal, sphCoeffsX1, sphCoeffsX2, sphCoeffsX3);
  vOut.refractedVector.y = reconstruct_from_sh(inRayDirection, -vOut.fragNormal, sphCoeffsY1, sphCoeffsY2, sphCoeffsY3);
  vOut.refractedVector.z = reconstruct_from_sh(inRayDirection, -vOut.fragNormal, sphCoeffsZ1, sphCoeffsZ2, sphCoeffsZ3);
}
