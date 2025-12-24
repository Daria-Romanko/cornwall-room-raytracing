#pragma once
#include <glm/glm.hpp>

class AffineTransform {
public:
	static glm::mat4 translation(float x, float y, float z) {
		glm::mat4 res = glm::mat4(1.0f);
		res[3][0] = x;
		res[3][1] = y;
		res[3][2] = z;
		return res;
	}

	static glm::mat4 translation(const glm::vec3& t) {
		return translation(t.x, t.y, t.z);
	}

	static glm::mat4 scaling(float sx, float sy, float sz) {
		glm::mat4 res = glm::mat4(1.0f);
		res[0][0] = sx;
		res[1][1] = sy;
		res[2][2] = sz;
		return res;
	}

	static glm::mat4 scaling(const glm::vec3& s) {
		return scaling(s.x, s.y, s.z);
	}

	static glm::mat4 rotationX(float angle) {
		glm::mat4 res = glm::mat4(1.0f);
		float c = std::cos(angle);
		float s = std::sin(angle);

		res[1][1] = c;
		res[1][2] = s;
		res[2][1] = -s;
		res[2][2] = c;

		return res;
	}

	static glm::mat4 rotationY(float angle) {
		glm::mat4 res = glm::mat4(1.0f);
		float c = std::cos(angle);
		float s = std::sin(angle);

		res[0][0] = c;
		res[0][2] = -s;
		res[2][0] = s;
		res[2][2] = c;

		return res;
	}

	static glm::mat4 rotationZ(float angle) {
		glm::mat4 res = glm::mat4(1.0f);
		float c = std::cos(angle);
		float s = std::sin(angle);

		res[0][0] = c;
		res[0][1] = s;
		res[1][0] = -s;
		res[1][1] = c;

		return res;
	}

	static glm::mat4 reflectionXY() {
		return scaling(1.0f, 1.0f, -1.0f);
	}

	static glm::mat4 reflectionXZ() {
		return scaling(1.0f, -1.0f, 1.0f);
	}

	static glm::mat4 reflectionYZ() {
		return scaling(-1.0f, 1.0f, 1.0f);
	}

	static glm::mat4 rotationAroundAxis(const glm::vec3& axis, float angle) {
		glm::vec3 a = glm::normalize(axis);
		float c = std::cos(angle);
		float s = std::sin(angle);
		float t = 1.0f - c;

		glm::mat4 res;
		res[0][0] = c + a.x * a.x * t;
		res[0][1] = a.x * a.y * t - a.z * s;
		res[0][2] = a.x * a.z * t + a.y * s;
		res[0][3] = 0;

		res[1][0] = a.y * a.x * t + a.z * s;
		res[1][1] = c + a.y * a.y * t;
		res[1][2] = a.y * a.z * t - a.x * s;
		res[1][3] = 0;

		res[2][0] = a.z * a.x * t - a.y * s;
		res[2][1] = a.z * a.y * t + a.x * s;
		res[2][2] = c + a.z * a.z * t;
		res[2][3] = 0;

		res[3][0] = 0; res[3][1] = 0; res[3][2] = 0; res[3][3] = 1;
		return res;
	}
};