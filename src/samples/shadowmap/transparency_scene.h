#pragma once

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

enum class meshTypes {
	SPHERE,
	CUBE
};

class TransparencyScene {
	public:
		TransparencyScene();
		std::unordered_map<meshTypes, std::vector<glm::vec3>> positions;
};