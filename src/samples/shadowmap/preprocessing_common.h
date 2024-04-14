#pragma once

#include <functional>
#include <vector>

#include <glm/glm.hpp>

#define IOR 1.45f // index of refraction
#define SINGLE_VERTEX_FLOAT_NUM 42
#define VERTEX_POSITION_START 0
#define VERTEX_NORMAL_START 3

struct DataToEncode {
	float width, x, y, z;
};

// Calculations have to be performed using double, and only then results should be casted to float.
// Otherwise, precision is lost.
std::vector<glm::dvec3> construct_hemisphere_hammersley_sequence(uint32_t numPoints);

std::vector<float> calculate_sh_terms(
  std::vector<glm::dvec3> hammersleySequence, std::function<DataToEncode(glm::dvec3)> getDataToEncode
);
