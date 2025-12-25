#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Light.h"
#include <string>

class SceneNode {
public:
	std::string name;
	glm::mat4 transform = glm::mat4(1.0f);
	std::vector<std::unique_ptr<SceneNode>> children;
	std::unique_ptr<Mesh> mesh;
	std::unique_ptr<Light> light;

	SceneNode* parent = nullptr;

	SceneNode(const std::string& name = "Node") : name(name) {}

	void addChild(std::unique_ptr<SceneNode> child) {
		child->parent = this;
		children.push_back(std::move(child));
	}

	SceneNode* createChild(const std::string& name = "Node") {
		auto child = std::make_unique<SceneNode>(name);
		auto ptr = child.get();
		child->parent = this;
		children.push_back(std::move(child));
		return ptr;
	}

	glm::mat4 getWorldTransform() const {
		if (parent) {
			return parent->getWorldTransform() * transform;
		}
		return transform;
	}

	void update(float deltaTime) {
		for (auto& child : children) {
			child->update(deltaTime);
		}
	}

	void removeFromParent() {
		if (parent) {
			auto& siblings = parent->children;
			siblings.erase(
				std::remove_if(siblings.begin(), siblings.end(),
					[this](const std::unique_ptr<SceneNode>& node) {
						return node.get() == this;
					}),
				siblings.end()
			);
		}
	}
};