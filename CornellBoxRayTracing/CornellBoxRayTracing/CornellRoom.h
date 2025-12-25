#pragma once
#include "Scene.h"
#include "Mesh.h"
#include <memory>
#include <vector>

class CornellRoom {
public:
    struct WallProperties {
        glm::vec3 color = glm::vec3(1.0f);
        float reflectivity = 0.0f;
        float transparency = 0.0f;
        bool isMirror = false;
        bool isTransparent = false;
    };

    enum WallIndex {
        LEFT = 0,
        RIGHT = 1,
        BACK = 2,
        FLOOR = 3,
        CEILING = 4,
        FRONT = 5
    };

    CornellRoom(float size = 10.0f) : roomSize(size) {
        createRoom();
    }

    void addToScene(Scene& scene) {
        scene.getRoot()->children.push_back(std::move(roomNode));
    }

    void setLeftWallColor(const glm::vec3& color) { setWallColor(LEFT, color); }
    void setRightWallColor(const glm::vec3& color) { setWallColor(RIGHT, color); }
    void setBackWallColor(const glm::vec3& color) { setWallColor(BACK, color); }
    void setFloorColor(const glm::vec3& color) { setWallColor(FLOOR, color); }
    void setCeilingColor(const glm::vec3& color) { setWallColor(CEILING, color); }
    void setFrontWallColor(const glm::vec3& color) { setWallColor(FRONT, color); }

    void setWallColor(int wallIndex, const glm::vec3& color) {
        if (wallIndex >= 0 && wallIndex < (int)walls.size() && wallNodes[wallIndex]) {
            wallNodes[wallIndex]->mesh->material.diffuseColor = color;
            walls[wallIndex].color = color;
        }
    }

    void setWallReflectivity(int wallIndex, float reflectivity) {
        if (wallIndex >= 0 && wallIndex < (int)walls.size() && wallNodes[wallIndex]) {
            wallNodes[wallIndex]->mesh->material.reflectivity = reflectivity;
            wallNodes[wallIndex]->mesh->material.isMirror = reflectivity > 0.0f;

            walls[wallIndex].reflectivity = reflectivity;
            walls[wallIndex].isMirror = reflectivity > 0.0f;
        }
    }

    void setWallTransparency(int wallIndex, float transparency) {
        if (wallIndex >= 0 && wallIndex < (int)walls.size() && wallNodes[wallIndex]) {
            wallNodes[wallIndex]->mesh->material.transparency = transparency;
            wallNodes[wallIndex]->mesh->material.isTransparent = transparency > 0.0f;

            walls[wallIndex].transparency = transparency;
            walls[wallIndex].isTransparent = transparency > 0.0f;
        }
    }

    WallProperties getWallProperties(int wallIndex) const {
        if (wallIndex >= 0 && wallIndex < (int)walls.size()) {
            return walls[wallIndex];
        }
        return WallProperties();
    }

private:
    float roomSize;
    std::unique_ptr<SceneNode> roomNode;

    SceneNode* leftWall = nullptr;
    SceneNode* rightWall = nullptr;
    SceneNode* backWall = nullptr;
    SceneNode* floor = nullptr;
    SceneNode* ceiling = nullptr;
    SceneNode* frontWall = nullptr;

    std::vector<SceneNode*> wallNodes;
    std::vector<WallProperties> walls;

    void createRoom() {
        roomNode = std::make_unique<SceneNode>("CornellRoom");

        walls = {
            {glm::vec3(1.0f, 0.0f, 0.0f)},
            {glm::vec3(0.0f, 0.15f, 1.0f)},
            {glm::vec3(0.7f, 0.7f, 0.7f)},
            {glm::vec3(0.7f, 0.7f, 0.7f)},
            {glm::vec3(0.7f, 0.7f, 0.7f)},
            {glm::vec3(0.7f, 0.7f, 0.7f)}
        };

        wallNodes.assign(walls.size(), nullptr);

        createWall("LeftWall", glm::vec3(-roomSize / 2, 0, 0), glm::vec3(0, 90, 0), walls[LEFT], LEFT);
        createWall("RightWall", glm::vec3(roomSize / 2, 0, 0), glm::vec3(0, -90, 0), walls[RIGHT], RIGHT);
        createWall("BackWall", glm::vec3(0, 0, -roomSize / 2), glm::vec3(0, 0, 0), walls[BACK], BACK);
        createWall("Floor", glm::vec3(0, -roomSize / 2, 0), glm::vec3(-90, 0, 0), walls[FLOOR], FLOOR);
        createWall("Ceiling", glm::vec3(0, roomSize / 2, 0), glm::vec3(90, 0, 0), walls[CEILING], CEILING);
        createWall("FrontWall", glm::vec3(0, 0, roomSize / 2), glm::vec3(0, 180, 0), walls[FRONT], FRONT);
    }

    void createWall(const std::string& name,
        const glm::vec3& position,
        const glm::vec3& rotationDeg,
        const WallProperties& props,
        int wallIndex)
    {
        auto wallNode = std::make_unique<SceneNode>(name);

        wallNode->mesh = createWallMesh(name);
        wallNode->mesh->position = position;
        wallNode->mesh->rotation = glm::radians(rotationDeg);

        wallNode->mesh->material.diffuseColor = props.color;
        wallNode->mesh->material.reflectivity = props.reflectivity;
        wallNode->mesh->material.transparency = props.transparency;
        wallNode->mesh->material.isMirror = props.isMirror;
        wallNode->mesh->material.isTransparent = props.isTransparent;
        wallNode->mesh->material.shininess = 32.0f;

        if (name == "LeftWall") leftWall = wallNode.get();
        else if (name == "RightWall") rightWall = wallNode.get();
        else if (name == "BackWall") backWall = wallNode.get();
        else if (name == "Floor") floor = wallNode.get();
        else if (name == "Ceiling") ceiling = wallNode.get();
        else if (name == "FrontWall") frontWall = wallNode.get();

        wallNodes[wallIndex] = wallNode.get();
        roomNode->children.push_back(std::move(wallNode));
    }

    std::unique_ptr<Mesh> createWallMesh(const std::string& wallName) {
        auto mesh = std::make_unique<Mesh>();
        mesh->name = "Wall_" + wallName;
        float halfSize = roomSize / 2.0f;

        std::vector<Vertex> vertices = {
            {glm::vec3(-halfSize, -halfSize, 0), glm::vec3(0, 0, 1), glm::vec2(0, 0)},
            {glm::vec3(halfSize, -halfSize, 0), glm::vec3(0, 0, 1), glm::vec2(1, 0)},
            {glm::vec3(halfSize,  halfSize, 0), glm::vec3(0, 0, 1), glm::vec2(1, 1)},
            {glm::vec3(-halfSize,  halfSize, 0), glm::vec3(0, 0, 1), glm::vec2(0, 1)}
        };

        Face face1, face2;
        face1.vertices = { vertices[0], vertices[1], vertices[2] };
        face2.vertices = { vertices[0], vertices[2], vertices[3] };

        face1.calculateNormal();
        face2.calculateNormal();

        mesh->faces.push_back(face1);
        mesh->faces.push_back(face2);

        return mesh;
    }
};
