#pragma once
#include <glm/glm.hpp>
#include <cmath>

class Camera {
public:
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	float fov = 45.0f;
	float aspectRatio = 16.0f / 9.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	enum class ProjectionType {
		Perspective,
		Orthographic
	};

	ProjectionType projectionType = ProjectionType::Perspective;

	glm::vec3 getForwardDirection() const {
		return glm::normalize(target - position);
	}

	glm::vec3 getRightDirection() const {
		return glm::normalize(glm::cross(getForwardDirection(), up));
	}

	glm::vec3 getUpDirection() const {
		return glm::normalize(glm::cross(getRightDirection(), getForwardDirection()));
	}

	glm::mat4 getViewMatrix() const {
		glm::vec3 zaxis = glm::normalize(position - target);
		glm::vec3 xaxis = glm::normalize(glm::cross(up, zaxis));
		glm::vec3 yaxis = glm::cross(zaxis, xaxis);

		glm::mat4 view = glm::mat4(1.0f);

		view[0][0] = xaxis.x; view[1][0] = xaxis.y; view[2][0] = xaxis.z;
		view[0][1] = yaxis.x; view[1][1] = yaxis.y; view[2][1] = yaxis.z;
		view[0][2] = zaxis.x; view[1][2] = zaxis.y; view[2][2] = zaxis.z;

		view[3][0] = -glm::dot(xaxis, position);
		view[3][1] = -glm::dot(yaxis, position);
		view[3][2] = -glm::dot(zaxis, position);

		return view;
	}

	glm::mat4 getPerspectiveProjection() const {
		glm::mat4 projection = glm::mat4(0.0f);

		float f = 1.0f / std::tan(glm::radians(fov) / 2.0f);
		float rangeInv = 1.0f / (nearPlane - farPlane);

		projection[0][0] = f / aspectRatio;
		projection[1][1] = f;
		projection[2][2] = (nearPlane + farPlane) * rangeInv;
		projection[2][3] = -1.0f;
		projection[3][2] = nearPlane * farPlane * rangeInv * 2.0f;

		return projection;
	}

	glm::mat4 getOrthographicProjection() const {
		glm::mat4 projection = glm::mat4(1.0f);

		float distance = glm::length(position - target);
		
		projection[0][0] = 1.0f / distance;
		projection[1][1] = 1.0f / distance;
		projection[2][2] = -2.0f / (farPlane - nearPlane);
		projection[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);

		return projection;
	}

	glm::mat4 getProjectionMatrix() const {
		if (projectionType == ProjectionType::Perspective) {
			return getPerspectiveProjection();
		}
		else {
			return getOrthographicProjection();
		}
	}

	void setAspectRatio(float width, float height) {
		aspectRatio = width / height;
	}
};