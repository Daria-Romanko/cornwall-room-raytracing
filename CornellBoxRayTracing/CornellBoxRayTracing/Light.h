#pragma once
#include <glm/glm.hpp>

struct Light {
    glm::vec3 position{ 0.0f };
    glm::vec3 color{ 1.0f };
    float intensity{ 1.0f };

    Light() = default;

    Light(const glm::vec3& pos,
        const glm::vec3& col = glm::vec3(1.0f),
        float intens = 1.0f)
        : position(pos), color(col), intensity(intens) {
    }
};
