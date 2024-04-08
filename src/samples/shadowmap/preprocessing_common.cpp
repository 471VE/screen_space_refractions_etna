#include "preprocessing_common.h"

#include <algorithm>
#include <execution>

#include <glm/ext.hpp>

// Spherical harmonics without constant terms
static double Y00 (glm::dvec3 dir) { return 1.; }

static double Y1m1(glm::dvec3 dir) { return dir.y; }
static double Y10 (glm::dvec3 dir) { return dir.z; }
static double Y11 (glm::dvec3 dir) { return dir.x; }

static double Y2m2(glm::dvec3 dir) { return dir.x * dir.y; }
static double Y2m1(glm::dvec3 dir) { return dir.y * dir.z; }
static double Y20 (glm::dvec3 dir) { return 3. * dir.z * dir.z - 1.; }
static double Y21 (glm::dvec3 dir) { return dir.x * dir.z; }
static double Y22 (glm::dvec3 dir) { return dir.x * dir.x - dir.y * dir.y; }

static const std::vector<std::function<double(glm::dvec3)>> SPHERICAL_HARMONICS = {
  Y00, Y1m1, Y10, Y11, Y2m2, Y2m1, Y20, Y21, Y22 
};

static const std::vector<float> SH_CONSTANTS_SQUARED = {
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

static double van_der_corput_sequence(uint32_t bits)
{
  // Reversing bits:
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  // Equivalent to: double(bits) / double(0x100000000):
  return double(bits) * 2.3283064365386963e-10;
}

static glm::vec2 get_hammersley_point(uint32_t i, uint32_t N)
{
  return glm::vec2(double(i) / double(N), van_der_corput_sequence(i));
}

static glm::dvec3 sample_hemisphere_uniform(glm::vec2 H)
{
  double phi = H.y * 2. * glm::pi<double>();
  double cosTheta = 1. - H.x;
  double sinTheta = sqrt(1. - cosTheta * cosTheta);
  return glm::dvec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

std::vector<glm::dvec3> construct_hemisphere_hammersley_sequence(uint32_t numPoints)
{
  std::vector<glm::dvec3> hammersleySequence;
  hammersleySequence.reserve(numPoints);
  for (uint32_t i = 0; i < numPoints; i++)
    hammersleySequence.push_back(sample_hemisphere_uniform(get_hammersley_point(i, numPoints)));
  return hammersleySequence;
}

std::vector<float> calculate_sh_terms(
  std::vector<glm::dvec3> hammersleySequence, std::function<DataToEncode(glm::dvec3)> getDataToEncode
) {
  std::vector<double> shTermsSums(SPHERICAL_HARMONICS.size() * 4, 0.);

  std::for_each(
    std::execution::par,
    hammersleySequence.begin(),
    hammersleySequence.end(),
    [&getDataToEncode, &shTermsSums](auto&& direction)
    {
      DataToEncode data = getDataToEncode(direction);
      for (int i = 0; i < SPHERICAL_HARMONICS.size(); i++)
      {
        shTermsSums[i + 0 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.width;
        shTermsSums[i + 1 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.x;
        shTermsSums[i + 2 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.y;
        shTermsSums[i + 3 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.z;
      }
    });

  std::vector<float> shTerms;
  shTerms.reserve(shTermsSums.size());
  for (int i = 0; i < shTermsSums.size(); i++)
    shTerms.push_back(
      float(shTermsSums[i] * 2. * glm::pi<double>() / double(hammersleySequence.size()))
      * SH_CONSTANTS_SQUARED[i % SPHERICAL_HARMONICS.size()]
    );

  return shTerms;
}
