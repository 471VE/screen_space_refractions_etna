#include "preprocessing_common.h"
#include "spherical_harmonics.h"

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

static const double INTEGRATION_CONE_ANGLE = glm::pi<double>() / 12.f;
static const double COS_THRESHOLD = std::cos(glm::pi<double>() / 2.f - INTEGRATION_CONE_ANGLE);
static const double AREA_OF_INTEGRATION = 2. * glm::pi<double>() * COS_THRESHOLD;

static glm::vec2 get_hammersley_point(uint32_t i, uint32_t N)
{
  return glm::vec2(double(i) / double(N), van_der_corput_sequence(i));
}

static glm::dvec3 sample_hemisphere_uniform(glm::vec2 H)
{
  double phi = H.y * 2. * glm::pi<double>();
  double cosTheta = 1. - H.x * COS_THRESHOLD;
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
  std::vector<double> shTermsSums(SPHERICAL_HARMONICS.size() * SH_ENCODED_VALUES, 0.);

  for (const auto &direction : hammersleySequence)
  {
    DataToEncode data = getDataToEncode(direction);
    for (int i = 0; i < SPHERICAL_HARMONICS.size(); i++)
    {
      shTermsSums[i + 0 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.width;
      shTermsSums[i + 1 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.x;
      shTermsSums[i + 2 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.y;
      shTermsSums[i + 3 * SPHERICAL_HARMONICS.size()] += SPHERICAL_HARMONICS[i](direction) * data.z;
    }
  }

  std::vector<float> shTerms;
  shTerms.reserve(shTermsSums.size());
  for (int i = 0; i < shTermsSums.size(); i++)
    shTerms.push_back(
      float(shTermsSums[i] * AREA_OF_INTEGRATION / double(hammersleySequence.size()))
      * SH_CONSTANTS_SQUARED[i % SPHERICAL_HARMONICS.size()]
    );

  return shTerms;
}
