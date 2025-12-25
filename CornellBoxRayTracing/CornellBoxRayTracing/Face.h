#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "Material.h"

class Face {
public:
    std::vector<Vertex> vertices;
    glm::vec3 normal;
    glm::vec3 color = glm::vec3(1.0f);

    void calculateNormal() {
        if (vertices.size() < 3) return;

        glm::vec3 v1 = vertices[1].position - vertices[0].position;
        glm::vec3 v2 = vertices[2].position - vertices[0].position;
        normal = glm::normalize(glm::cross(v1, v2));
    }

    void setColor(const glm::vec3& newColor) {
        color = newColor;
        for (auto& vertex : vertices) {
            vertex.color = newColor;
        }
    }

    bool isBackface(const glm::vec3& viewDir) const {
        return glm::dot(normal, viewDir) > 0;
    }
};