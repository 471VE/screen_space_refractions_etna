#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.h"

layout (location = 0) in vec4 vPosNorm;
layout (location = 1) in vec4 vTexCoordAndTang;

layout (push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mModel;
    uint colorNo;
} PushConstant;

layout (location = 0) out VS_OUT
{ 
    vec3 wPos;
    vec3 wNorm;
    vec2 texCoord;

} vOut;

out gl_PerVertex { vec4 gl_Position; };
void main(void)
{
    const vec4 wNorm = vec4(DecodeNormal(floatBitsToInt(vPosNorm.w)),         0.0f);
    const vec4 wTang = vec4(DecodeNormal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

    vOut.wPos     = (PushConstant.mModel * vec4(vPosNorm.xyz, 1.0f)).xyz;
    vOut.wNorm    = normalize(mat3(transpose(inverse(PushConstant.mModel))) * wNorm.xyz);
    vOut.texCoord = vTexCoordAndTang.xy;

    gl_Position   = PushConstant.mProjView * vec4(vOut.wPos, 1.0);
}