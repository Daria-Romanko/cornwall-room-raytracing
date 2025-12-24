#pragma once
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color; 

    Vertex(const glm::vec3& pos = glm::vec3(0.0f),
           const glm::vec3& norm = glm::vec3(0.0f),
           const glm::vec2& tex = glm::vec2(0.0f),
           const glm::vec3& col = glm::vec3(1.0f))
        : position(pos), normal(norm), texCoord(tex), color(col) {}
};