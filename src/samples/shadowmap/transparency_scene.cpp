#include "transparency_scene.h"

TransparencyScene::TransparencyScene()
{
	// Turn off scene for now
	
	// positions.insert({ meshTypes::GROUND, {} });
	// positions.insert({ meshTypes::GIRL, {} });
	// positions.insert({ meshTypes::SKULL, {} });
	// positions.insert({ meshTypes::VIKING_ROOM, {} });
	positions.insert({ meshTypes::CUBE, {} });
	positions[meshTypes::CUBE].push_back(glm::vec3(0.f, 0.f, 0.f));
	// positions[meshTypes::CUBE].push_back(glm::vec3(0.f, 0.f, 0.f));

	// positions[meshTypes::GROUND].push_back(glm::vec3(10.f, 0.f, 0.f));
	// positions[meshTypes::GIRL].push_back(glm::vec3(5.f, 0.f, 0.f));
	// positions[meshTypes::SKULL].push_back(glm::vec3(15.f, -5.f, 1.f));
	// positions[meshTypes::SKULL].push_back(glm::vec3(15.f, 5.f, 1.f));
	// positions[meshTypes::VIKING_ROOM].push_back(glm::vec3(3.f, 1.5f, 4.f));
};