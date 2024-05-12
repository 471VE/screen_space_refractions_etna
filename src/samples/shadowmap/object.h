#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

enum ModelFillType
{
	SOLID,
	HOLLOW
};

class ObjectMesh {
public:
	std::vector<float> vertices;
	std::vector<uint32_t> indices;
	std::vector<glm::vec3> v, vn;
	std::unordered_map<std::string, uint32_t> history;
	glm::mat4 preTransform;

	void load(const std::string &objFilePath, glm::mat4 preTransform);
	void readVertexData(const std::vector<std::string> &words);
	void readNormalData(const std::vector<std::string> &words);
	void readFaceData(const std::vector<std::string> &words);
	void readCorner(const std::string &vertex_description);
};

std::pair<std::string, ModelFillType> read_model_data(std::string modelNamePath);
std::vector<std::string> split_line(std::string line, std::string delimiter);