#pragma once
#include <memory>
#include <vector>
#include "SceneNode.h"
#include "Camera.h"

class Scene {
public:
	std::unique_ptr<SceneNode> root;
	std::unique_ptr<Camera> camera;
	std::vector<SceneNode*> lights;

	glm::vec3 ambientLight = glm::vec3(0.1f);
	glm::vec3 backgroundColor = glm::vec3(0.0f);

	Scene() {
		root = std::make_unique<SceneNode>("Root");
		camera = std::make_unique<Camera>();
	}

	SceneNode* getRoot() const { return root.get(); }
	Camera* getCamera() const { return camera.get(); }

	void addLight(SceneNode* light) {
		lights.push_back(light);
	}

	void removeLight(SceneNode* lightNode) {
		lights.erase(std::remove(lights.begin(), lights.end(), lightNode), lights.end());
	}

	std::vector<Mesh*> getAllMeshes() const {
		std::vector<Mesh*> meshes;
		collectMeshes(root.get(), meshes);
		return meshes;
	}

	std::vector<Light> getLights() const {
		std::vector<Light> lightComponents;
		for (auto* lightNode : lights) {
			if (lightNode && lightNode->light) {
				lightComponents.push_back(*lightNode->light);
			}
		}

		return lightComponents;
	}
private:
	void collectMeshes(SceneNode* node, std::vector<Mesh*>& meshes) const {
		if (node->mesh) {
			meshes.push_back(node->mesh.get());
		}
		for (auto& child : node->children) {
			collectMeshes(child.get(), meshes);
		}
	}
};