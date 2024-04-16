#include "object.h"
#include "preprocessing_common.h"

#include <fstream>

static std::vector<std::string> split(std::string line, std::string delimiter)
{
	std::vector<std::string> split_line;

	size_t pos = 0;
	std::string token;
	while ((pos = line.find(delimiter)) != std::string::npos)
	{
		token = line.substr(0, pos);
		split_line.push_back(token);
		line.erase(0, pos + delimiter.length());
	}
	if (!line.empty())
		split_line.push_back(line);

	return split_line;
}

void ObjectMesh::load(const char* objFilepath, glm::mat4 preTransform)
{
	this->preTransform = preTransform;

	std::ifstream file;
	std::string line;
	std::vector<std::string> words;

	size_t vertex_count = 0;
	size_t normal_count = 0;
	size_t corner_count = 0;

	file.open(objFilepath);

	while (std::getline(file, line))
	{
		if (line.empty())
			continue;

		words = split(line, " ");

		if (!words[0].compare("v"))
			vertex_count += 1;

		else if (!words[0].compare("vn"))
			normal_count += 1;

		else if (!words[0].compare("f"))
		{
			size_t triangle_count = words.size() - 3;
			corner_count += 3 * triangle_count;
		}
	}

	file.close();

	v.reserve(vertex_count);
	vn.reserve(normal_count);
	vertices.reserve(corner_count);
	indices.reserve(corner_count);

	file.open(objFilepath);

	while (std::getline(file, line))
	{
		if (line.empty())
			continue;
			
		words = split(line, " ");

		if (!words[0].compare("v"))
			readVertexData(words);

		else if (!words[0].compare("vn"))
			readNormalData(words);

		else if (!words[0].compare("f"))
			readFaceData(words);
	}

	file.close();
}

void ObjectMesh::readVertexData(const std::vector<std::string>& words)
{
	glm::vec4 new_vertex = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 1.f);
	glm::vec3 transformed_vertex = glm::vec3(preTransform * new_vertex);
	v.push_back(transformed_vertex);
}

void ObjectMesh::readNormalData(const std::vector<std::string>& words)
{
	glm::vec4 new_normal = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 0.f);
	glm::vec3 transformed_normal = glm::vec3(preTransform * new_normal);
	vn.push_back(transformed_normal);
}

void ObjectMesh::readFaceData(const std::vector<std::string>& words)
{
	size_t triangleCount = words.size() - 3;

	for (int i = 0; i < triangleCount; ++i)
	{
		readCorner(words[1]);
		readCorner(words[2 + i]);
		readCorner(words[3 + i]);
	}
}

void ObjectMesh::readCorner(const std::string& vertex_description)
{
	if (history.contains(vertex_description))
	{
		indices.push_back(history[vertex_description]);
		return;
	}

	uint32_t index = static_cast<uint32_t>(history.size());
	history.insert({ vertex_description, index });
	indices.push_back(index);


	std::vector<std::string> v_vt_vn = split(vertex_description, "/");

	// Position
	glm::vec3 pos = v[std::stol(v_vt_vn[0]) - 1];
	vertices.push_back(pos[0]);
	vertices.push_back(pos[1]);
	vertices.push_back(pos[2]);

	// Normal
	glm::vec3 normal = vn[std::stol(v_vt_vn[2]) - 1];
	vertices.push_back(normal[0]);
	vertices.push_back(normal[1]);
	vertices.push_back(normal[2]);

	for (int i = 0; i < SH_COEEFS_NUM * SH_ENCODED_VALUES; i++)
		vertices.push_back(0);
}