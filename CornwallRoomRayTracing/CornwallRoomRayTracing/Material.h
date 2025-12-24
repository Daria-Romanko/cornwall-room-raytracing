#pragma once
#include <glm/glm.hpp>

struct Material {
	glm::vec3 diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glm::vec3 specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	float shininess = 32.0f;

    float reflectivity = 0.0f;
    bool isMirror = false;
 
    float transparency = 0.0f;
    bool isTransparent = false;
    float refractiveIndex = 1.0f;
};