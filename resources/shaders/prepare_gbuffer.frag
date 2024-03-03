#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "common.h"

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 albedo;

layout (location = 0) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec2 texCoord;
} vsOut;

layout (push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mModel;
    uint colorNo;
} PushConstant;

layout (binding = 0, set = 0) uniform AppData
{
  UniformParams Params;
};

void main()
{
  position = (Params.view * vec4(vsOut.wPos, 1.0)).xyz;
  normal = (Params.view * vec4(vsOut.wNorm, 0.0)).xyz;
  switch(PushConstant.colorNo)
  {
    case 0: // Room walls
      albedo = vec3(0.733f, 1.f, 0.596f); break;            
    case 1: // Teapot
      albedo = vec3(1.f, 0.596f, 0.651f); break;            
    case 2: // Box
      albedo = vec3(1.f, 0.424f, 0.424f); break;            
    case 3: // Cylinder
      albedo = vec3(0.596f, 0.957f, 1.f); break;            
    case 4: // L-shaped figure
      albedo = vec3(0.749f, 0.725f, 1.f); break;            
    case 5: // Sphere
      albedo = vec3(1.f, 0.788f, 0.424f); break;       
  }
}