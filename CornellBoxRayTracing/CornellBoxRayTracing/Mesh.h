#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Face.h"
#include "AffineTransform.h"
#include <string>

class Mesh {
public: 
	std::string name;
	std::vector<Face> faces;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0);
    Material material;

	void applyTransform(const glm::mat4& transform) {
        for (auto& face : faces) {
            for (auto& vertex : face.vertices) {
                glm::vec4 transformed = transform * glm::vec4(vertex.position, 1.0f);
                vertex.position = glm::vec3(transformed);

                glm::vec3 normalTransformed = glm::mat3(transform) * vertex.normal;
                vertex.normal = glm::normalize(normalTransformed);
            }
        }
	}

	glm::mat4 getTransformMatrix() const {
		return AffineTransform::translation(position)
			* AffineTransform::rotationX(rotation.x)
			* AffineTransform::rotationY(rotation.y)
			* AffineTransform::rotationZ(rotation.z)
			* AffineTransform::scaling(scale);
	}

	void translate(const glm::vec3& offset) {
		position += offset;
	}

    void rotate(float angle, const glm::vec3& axis) {
        glm::mat4 rotationMat = AffineTransform::rotationAroundAxis(axis, angle);
        applyTransform(rotationMat);
    }

    void setScale(const glm::vec3& newScale) {
        glm::mat4 scaleMat = AffineTransform::scaling(newScale.x / scale.x,
            newScale.y / scale.y,
            newScale.z / scale.z);
        applyTransform(scaleMat);
        scale = newScale;
    }

    void setMaterial(const Material& newMaterial) {
        material = newMaterial;
        for (auto& face : faces) {
            face.setColor(material.diffuseColor);
        }
    }

    void setColor(const glm::vec3& color) {
        material.diffuseColor = color;
        for (auto& face : faces) {
            face.setColor(color);
        }
    }

    glm::vec3 getCenter() const {
        if (faces.empty()) return glm::vec3(0.0f);

        glm::vec3 center(0.0f);
        size_t count = 0;

        for (const auto& face : faces) {
            for (const auto& vertex : face.vertices) {
                center += vertex.position;
                count++;
            }
        }

        return center / static_cast<float>(count);
    }

    void rotateAroundCenter(float angle, const glm::vec3& axis) {
        glm::vec3 center = getCenter();

        glm::mat4 toOrigin = AffineTransform::translation(-center);
        glm::mat4 rotation = AffineTransform::rotationAroundAxis(axis, angle);
        glm::mat4 fromOrigin = AffineTransform::translation(center);

        glm::mat4 transform = fromOrigin * rotation * toOrigin;
        applyTransform(transform);
    }

    void rotateAroundCenterX(float angle) { rotateAroundCenter(angle, glm::vec3(1, 0, 0)); }
    void rotateAroundCenterY(float angle) { rotateAroundCenter(angle, glm::vec3(0, 1, 0)); }
    void rotateAroundCenterZ(float angle) { rotateAroundCenter(angle, glm::vec3(0, 0, 1)); }

    void scaleAroundCenter(float factor) {
        scaleAroundCenter(glm::vec3(factor));
    }

    void scaleAroundCenter(const glm::vec3& factors) {
        glm::vec3 center = getCenter();

        glm::mat4 toOrigin = AffineTransform::translation(-center);
        glm::mat4 scaleMat = AffineTransform::scaling(factors);
        glm::mat4 fromOrigin = AffineTransform::translation(center);

        glm::mat4 transform = fromOrigin * scaleMat * toOrigin;
        applyTransform(transform);

        scale *= factors;
    }

    void reflectXY() {
        applyTransform(AffineTransform::reflectionXY());
        scale.z *= -1;
    }

    void reflectXZ() {
        applyTransform(AffineTransform::reflectionXZ());
        scale.y *= -1;
    }

    void reflectYZ() {
        applyTransform(AffineTransform::reflectionYZ());
        scale.x *= -1;
    }

    void calculateVertexNormals() {
        std::vector<glm::vec3> uniquePositions;
        std::vector<std::vector<size_t>> vertexToFaces;

        for (size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
            const auto& face = faces[faceIndex];
            for (const auto& vertex : face.vertices) {
                bool found = false;
                for (size_t i = 0; i < uniquePositions.size(); ++i) {
                    if (glm::distance(vertex.position, uniquePositions[i]) < 1e-5f) {
                        found = true;
                        vertexToFaces[i].push_back(faceIndex);
                        break;
                    }
                }
                if (!found) {
                    uniquePositions.push_back(vertex.position);
                    vertexToFaces.push_back({ faceIndex });
                }
            }
        }

        std::vector<glm::vec3> vertexNormals(uniquePositions.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < uniquePositions.size(); ++i) {
            for (size_t faceIndex : vertexToFaces[i]) {
                vertexNormals[i] += faces[faceIndex].normal;
            }
            if (!vertexToFaces[i].empty()) {
                vertexNormals[i] = glm::normalize(vertexNormals[i] / static_cast<float>(vertexToFaces[i].size()));
            }
        }

        for (auto& face : faces) {
            for (auto& vertex : face.vertices) {
                for (size_t i = 0; i < uniquePositions.size(); ++i) {
                    if (glm::distance(vertex.position, uniquePositions[i]) < 1e-5f) {
                        vertex.normal = vertexNormals[i];
                        break;
                    }
                }
            }
        }
    }

    static Mesh createSphereUV(float radius = 1.0f, int stacks = 24, int slices = 48) {
        Mesh mesh;
        mesh.name = "Sphere";

        stacks = std::max(3, stacks);
        slices = std::max(3, slices);

        std::vector<Vertex> verts;
        verts.reserve((stacks + 1) * (slices + 1));

        for (int i = 0; i <= stacks; ++i) {
            float v = float(i) / float(stacks);      
            float theta = v * glm::pi<float>();

            float st = std::sin(theta);
            float ct = std::cos(theta);

            for (int j = 0; j <= slices; ++j) {
                float u = float(j) / float(slices);  
                float phi = u * (2.0f * glm::pi<float>());

                float sp = std::sin(phi);
                float cp = std::cos(phi);

                glm::vec3 n = glm::normalize(glm::vec3(st * cp, ct, st * sp));
                glm::vec3 p = n * radius;

                Vertex vx;
                vx.position = p;
                vx.normal = n;
                vx.texCoord = glm::vec2(u, 1.0f - v);
                verts.push_back(vx);
            }
        }

        auto addTri = [&](int ia, int ib, int ic) {
            Face f;
            f.vertices = { verts[ia], verts[ib], verts[ic] };

            glm::vec3 e1 = f.vertices[1].position - f.vertices[0].position;
            glm::vec3 e2 = f.vertices[2].position - f.vertices[0].position;
            glm::vec3 n = glm::normalize(glm::cross(e1, e2));

            if (glm::dot(n, f.vertices[0].position) < 0.0f) {
                std::swap(f.vertices[1], f.vertices[2]);
                n = -n;
            }
            f.normal = n;

            mesh.faces.push_back(std::move(f));
            };

        int stride = slices + 1;
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int i0 = i * stride + j;
                int i1 = i0 + 1;
                int i2 = (i + 1) * stride + j;
                int i3 = i2 + 1;

                if (i != 0)        addTri(i0, i2, i1);
                if (i != stacks - 1) addTri(i1, i2, i3);
            }
        }

        mesh.calculateVertexNormals();
        return mesh;
    }

    static Mesh createLightBox(float width = 0.3f, float height = 0.3f, float depth = 1.0f) {
        Mesh mesh;
        mesh.name = "LightCapsule";

        const float w = width * 0.5f;
        const float h = height * 0.5f;
        const float d = depth * 0.5f;

        std::vector<Vertex> vertices = {
            {{-w, -h,  d}, {0, 0, 1}, {0, 0}},
            {{ w, -h,  d}, {0, 0, 1}, {1, 0}},
            {{ w,  h,  d}, {0, 0, 1}, {1, 1}},
            {{-w,  h,  d}, {0, 0, 1}, {0, 1}},

            {{-w, -h, -d}, {0, 0, -1}, {0, 0}},
            {{ w, -h, -d}, {0, 0, -1}, {1, 0}},
            {{ w,  h, -d}, {0, 0, -1}, {1, 1}},
            {{-w,  h, -d}, {0, 0, -1}, {0, 1}},
        };

        auto quad = [&](int a, int b, int c, int d) {
            Face face;
            face.vertices = { vertices[a], vertices[b], vertices[c] };
            mesh.faces.push_back(face);
            Face face2;
            face2.vertices = { vertices[a], vertices[c], vertices[d] };
            mesh.faces.push_back(face2);
            };

        quad(0, 1, 2, 3); 
        quad(5, 4, 7, 6);
        quad(4, 0, 3, 7);
        quad(1, 5, 6, 2);
        quad(3, 2, 6, 7);
        quad(4, 5, 1, 0);

        mesh.calculateVertexNormals();
        return mesh;
    }

};