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

#define X dir.x
#define Y dir.y
#define Z dir.z

shader_double pow3(shader_double x) { return x * x * x; }
shader_double pow4(shader_double x) { shader_double y = x * x; return y * y; }

// Spherical harmonics without constant terms
shader_double Y00 (shader_dvec3 dir) { return 1.; }

shader_double Y1m1(shader_dvec3 dir) { return Y; }
shader_double Y10 (shader_dvec3 dir) { return Z; }
shader_double Y11 (shader_dvec3 dir) { return X; }

shader_double Y2m2(shader_dvec3 dir) { return X * Y; }
shader_double Y2m1(shader_dvec3 dir) { return Y * Z; }
shader_double Y20 (shader_dvec3 dir) { return 3. * Z * Z - 1.; }
shader_double Y21 (shader_dvec3 dir) { return X * Z; }
shader_double Y22 (shader_dvec3 dir) { return X * X - Y * Y; }

shader_double Y3m3(shader_dvec3 dir) { return Y * (3. * X * X - Y * Y); }
shader_double Y3m2(shader_dvec3 dir) { return X * Y * Z; }
shader_double Y3m1(shader_dvec3 dir) { return Y * (5. * Z * Z - 1.); }
shader_double Y30 (shader_dvec3 dir) { return 5. * pow3(Z) - 3 * Z; }
shader_double Y31 (shader_dvec3 dir) { return X * (5. * Z * Z - 1.); }
shader_double Y32 (shader_dvec3 dir) { return (X * X - Y * Y) * Z; }
shader_double Y33 (shader_dvec3 dir) { return X * (X * X - 3 * Y * Y); }

shader_double Y4m4(shader_dvec3 dir) { return X * Y * (X * X - Y * Y); }
shader_double Y4m3(shader_dvec3 dir) { return Y * (3. * X * X - Y * Y) * Z; }
shader_double Y4m2(shader_dvec3 dir) { return X * Y * (7. * Z * Z - 1.); }
shader_double Y4m1(shader_dvec3 dir) { return Y * (7. * pow3(Z) - 3. * Z); }
shader_double Y40 (shader_dvec3 dir) { return 35. * pow4(Z) - 30. * Z * Z + 3.; }
shader_double Y41 (shader_dvec3 dir) { return X * (7. * pow3(Z) - 3. * Z); }
shader_double Y42 (shader_dvec3 dir) { return (X * X - Y * Y) * (7. * Z * Z - 1.); }
shader_double Y43 (shader_dvec3 dir) { return X * Z * (X * X - 3. * Y * Y); }
shader_double Y44 (shader_dvec3 dir) { return X * X * (X * X - 3. * Y * Y) - Y * Y * (3. * X * X - Y * Y); }


#ifdef __cplusplus

#include <functional>
#include <glm/ext.hpp>

const std::vector<std::function<double(glm::dvec3)>> SPHERICAL_HARMONICS = {
  Y00,
  Y1m1, Y10, Y11,
  Y2m2, Y2m1, Y20, Y21, Y22,
  // extra:
  Y3m3, Y3m2, Y3m1, Y30, Y31, Y32, Y33,
  Y4m4, Y4m3, Y4m2, Y4m1, Y40, Y41, Y42, Y43, Y44,
};

#define PI glm::pi<float>()

const std::vector<float> SH_CONSTANTS_SQUARED = {
  1.f / (4.f * PI),

  3.f / (4.f * PI),
  3.f / (4.f * PI),
  3.f / (4.f * PI),

  15.f / (4.f * PI),
  15.f / (4.f * PI),
  5.f / (16.f * PI),
  15.f / (4.f * PI),
  15.f / (16.f * PI),

  // extra:
  35.f / (32.f * PI),
  105.f / (4.f * PI),
  21.f / (32.f * PI),
  7.f / (16.f * PI),
  21.f / (32.f * PI),
  105.f / (16.f * PI),
  35.f / (32.f * PI),

  315.f / (16.f * PI),
  315.f / (32.f * PI),
  45.f / (16.f * PI),
  45.f / (32.f * PI),
  9.f / (256.f * PI),
  45.f / (32.f * PI),
  45.f / (64.f * PI),
  315.f / (32.f * PI),
  315.f / (256.f * PI),
};

#endif

#endif // SPHERICAL_HARMONICS_H