#ifndef SPHERICAL_HARMONICS_H
#define SPHERICAL_HARMONICS_H

// GLSL-C++ datatype compatibility layer

#ifdef __cplusplus

#include <glm/glm.hpp>

using shader_double = double;
using shader_dvec3  = glm::dvec3;

#else

#define shader_double float
#define shader_dvec3  vec3

#endif

// Spherical harmonics without constant terms
shader_double Y00 (shader_dvec3 dir) { return 1.; }

shader_double Y1m1(shader_dvec3 dir) { return dir.y; }
shader_double Y10 (shader_dvec3 dir) { return dir.z; }
shader_double Y11 (shader_dvec3 dir) { return dir.x; }

shader_double Y2m2(shader_dvec3 dir) { return dir.x * dir.y; }
shader_double Y2m1(shader_dvec3 dir) { return dir.y * dir.z; }
shader_double Y20 (shader_dvec3 dir) { return 3. * dir.z * dir.z - 1.; }
shader_double Y21 (shader_dvec3 dir) { return dir.x * dir.z; }
shader_double Y22 (shader_dvec3 dir) { return dir.x * dir.x - dir.y * dir.y; }


#ifdef __cplusplus

#include <functional>
#include <glm/ext.hpp>

const std::vector<std::function<double(glm::dvec3)>> SPHERICAL_HARMONICS = {
  Y00, Y1m1, Y10, Y11, Y2m2, Y2m1, Y20, Y21, Y22 
};

const std::vector<float> SH_CONSTANTS_SQUARED = {
  1.f / (4.f * glm::pi<float>()),
  3.f / (4.f * glm::pi<float>()),
  3.f / (4.f * glm::pi<float>()),
  3.f / (4.f * glm::pi<float>()),
  15.f / (4.f * glm::pi<float>()),
  15.f / (4.f * glm::pi<float>()),
  5.f / (16.f * glm::pi<float>()),
  15.f / (4.f * glm::pi<float>()),
  15.f / (16.f * glm::pi<float>())
};

#endif

#endif // SPHERICAL_HARMONICS_H