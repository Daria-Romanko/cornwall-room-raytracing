#pragma once
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include "Mesh.h"

class OBJLoader {
public:
    static Mesh loadFromFile(const std::string& filename) {
        Mesh mesh;
        mesh.name = filename;

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<Face> faces;

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return mesh;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "v") {
                float x, y, z;
                iss >> x >> y >> z;
                vertices.push_back(glm::vec3(x, y, z));
            }
            else if (type == "vn") {
                float x, y, z;
                iss >> x >> y >> z;
                normals.push_back(glm::vec3(x, y, z));
            }
            else if (type == "vt") {
                float u, v;
                iss >> u >> v;
                texCoords.push_back(glm::vec2(u, v));
            }
            else if (type == "f") {
                std::vector<Vertex> faceVertices;
                std::string vertexStr;

                while (iss >> vertexStr) {
                    Vertex vertex;
                    std::istringstream vss(vertexStr);
                    std::string indices[3];

                    for (int i = 0; i < 3; ++i) {
                        std::getline(vss, indices[i], '/');
                    }

                    int vIndex = std::stoi(indices[0]) - 1;
                    if (vIndex >= 0 && vIndex < vertices.size()) {
                        vertex.position = vertices[vIndex];
                    }

                    if (!indices[1].empty()) {
                        int vtIndex = std::stoi(indices[1]) - 1;
                        if (vtIndex >= 0 && vtIndex < texCoords.size()) {
                            vertex.texCoord = texCoords[vtIndex];
                        }
                    }

                    if (!indices[2].empty()) {
                        int vnIndex = std::stoi(indices[2]) - 1;
                        if (vnIndex >= 0 && vnIndex < normals.size()) {
                            vertex.normal = normals[vnIndex];
                        }
                    }
                    else {
                        vertex.normal = glm::vec3(0.0f);
                    }

                    faceVertices.push_back(vertex);
                }

                if (faceVertices.size() >= 3) {
                    for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                        Face triangle;
                        triangle.vertices = { faceVertices[0], faceVertices[i], faceVertices[i + 1] };

                        triangle.calculateNormal();

                        if (glm::any(glm::isnan(triangle.normal))) {
                            std::cout << "WARNING: Invalid normal detected, using default" << std::endl;
                            triangle.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                        }

                        mesh.faces.push_back(triangle);
                    }
                }
            }
        }

        file.close();

        int validNormals = 0;
        for (const auto& face : mesh.faces) {
            if (!glm::any(glm::isnan(face.normal)) && glm::length(face.normal) > 0.1f) {
                validNormals++;
            }
        }

        std::cout << "Loaded: " << filename
            << " | Vertices: " << vertices.size()
            << " | Faces: " << mesh.faces.size()
            << " | Valid normals: " << validNormals << "/" << mesh.faces.size() << std::endl;

        return mesh;
    }
};
