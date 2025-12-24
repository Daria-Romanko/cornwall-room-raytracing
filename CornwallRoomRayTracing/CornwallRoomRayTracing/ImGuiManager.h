#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Scene.h"
#include "CornellRoom.h"
#include "OBJLoader.h"
#include <vector>
#include <string>
#include <iostream>

class ImGuiManager {
public:
    ImGuiManager(sf::RenderWindow& window, Scene& scene, CornellRoom* cornellRoom = nullptr)
        : window(window), scene(scene), cornellRoom(cornellRoom) {
        ImGui::SFML::Init(window);
    }

    ~ImGuiManager() {
        ImGui::SFML::Shutdown();
    }

    void processEvent(const sf::Event& event) {
        ImGui::SFML::ProcessEvent(window, event);
    }

    void update(float deltaTime) {
        ImGui::SFML::Update(window, sf::seconds(deltaTime));
    }

    void render() {
        ImGui::SFML::Render(window);
    }

    void showSceneEditor() {
        ImGui::Begin("Scene Editor");

        if (ImGui::TreeNode("Objects")) {
            showObjectTree(scene.getRoot());
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Camera")) {
            showCameraControls(*scene.getCamera());
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Lighting")) {
            showLightingControls();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Ray Tracing")) {
            showRayTracingControls();
            ImGui::TreePop();
        }

        if (ImGui::Button("Load OBJ Model")) {
            showFileDialog = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Scene")) {
            clearScene();
        }

        if (showFileDialog) {
            showFileDialogWindow();
        }

        ImGui::End();

        if (selectedNode) {
            showPropertiesWindow();
        }
    }

    bool shouldRenderRayTracing() const { return renderRayTracing; }
    bool shouldReturnToEditing() const { return returnToEditing; }
    void resetRenderFlags() {
        renderRayTracing = false;
        returnToEditing = false;
    }

    void setShowRayTracingResult(bool show) { showRayTracingResult = show; }

private:
    sf::RenderWindow& window;
    Scene& scene;
    bool showFileDialog = false;
    char filePath[256] = "../models/";
    SceneNode* selectedNode = nullptr;
    std::vector<std::string> recentFiles;
    CornellRoom* cornellRoom = nullptr;

    bool renderRayTracing = false;
    bool returnToEditing = false;
    bool showRayTracingResult = false;

    void showRayTracingControls() {
        if (ImGui::TreeNode("Ray Tracing")) {

            if (!showRayTracingResult) {
                ImGui::Text("Scene is in EDITING mode");
                ImGui::Text("Wireframe view for fast editing");

                if (ImGui::Button("Render with Ray Tracing", ImVec2(200, 40))) {
                    renderRayTracing = true;
                }
            }
            else {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "RAY TRACING RESULT");
                ImGui::Text("High-quality rendering complete");

                if (ImGui::Button("Return to Editing", ImVec2(200, 40))) {
                    returnToEditing = true;
                }
                ImGui::Text("Go back to wireframe mode");
                ImGui::Text("to continue editing the scene");
            }
            ImGui::TreePop();
        }
    }

    void showObjectTree(SceneNode* node) {
        bool isSelected = (selectedNode == node);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;
        if (node->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

        bool isOpen = ImGui::TreeNodeEx(node->name.c_str(), flags);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            selectedNode = node;
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                deleteSelectedObject();
            }
            if (ImGui::MenuItem("Duplicate")) {
                duplicateObject(node);
            }
            ImGui::EndPopup();
        }

        if (isOpen) {
            for (auto& child : node->children) {
                showObjectTree(child.get());
            }
            ImGui::TreePop();
        }
    }

    void showPropertiesWindow() {
        ImGui::Begin("Properties");

        if (selectedNode) {
            ImGui::Text("Name: %s", selectedNode->name.c_str());

            if (selectedNode->mesh) {
                showMeshControls(*selectedNode->mesh);
                showMaterialControls(selectedNode->mesh->material);
            }

            if (ImGui::TreeNode("Transform")) {
                showTransformControls();
                ImGui::TreePop();
            }

            if (ImGui::Button("Reset Transform")) {
                resetTransform();
            }

            ImGui::SameLine();
            if (ImGui::Button("Delete Object")) {
                deleteSelectedObject();
            }
        }

        ImGui::End();
    }

    void showMeshControls(Mesh& mesh) {
        ImGui::Text("Mesh: %s", mesh.name.c_str());
        ImGui::Text("Faces: %d", mesh.faces.size());

        if (mesh.material.isMirror) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "MIRROR SURFACE");
        }
        if (mesh.material.isTransparent) {
            ImGui::TextColored(ImVec4(0, 1, 1, 1), "TRANSPARENT SURFACE");
        }
    }

    void showTransformControls() {
        if (!selectedNode || !selectedNode->mesh) return;

        auto& mesh = *selectedNode->mesh;

        if (ImGui::DragFloat3("Position", &mesh.position[0], 0.1f, -100.0f, 100.0f)) {
        }

        glm::vec3 rotationDeg = glm::degrees(mesh.rotation);
        if (ImGui::DragFloat3("Rotation", &rotationDeg[0], 1.0f, -180.0f, 180.0f)) {
            mesh.rotation = glm::radians(rotationDeg);
        }

        if (ImGui::DragFloat3("Scale", &mesh.scale[0], 0.1f, 0.01f, 10.0f)) {
        }

        if (ImGui::Button("Reset Position")) {
            mesh.position = glm::vec3(0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Rotation")) {
            mesh.rotation = glm::vec3(0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Scale")) {
            mesh.scale = glm::vec3(1.0f);
        }
    }

    void showCameraControls(Camera& camera) {
        glm::vec3 camPos = camera.position;
        if (ImGui::DragFloat3("Camera Position", &camPos[0], 0.1f, -50.0f, 50.0f)) {
            camera.position = camPos;
        }

        glm::vec3 camTarget = camera.target;
        if (ImGui::DragFloat3("Camera Target", &camTarget[0], 0.1f, -50.0f, 50.0f)) {
            camera.target = camTarget;
        }

        if (ImGui::DragFloat("FOV", &camera.fov, 1.0f, 10.0f, 120.0f)) {
        }

        if (ImGui::Button("Reset Camera")) {
            camera.position = glm::vec3(0.0f, 0.0f, 10.0f);
            camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
            camera.fov = 60.0f;
        }
    }

    void showMaterialControls(Material& material) {
        if (ImGui::TreeNode("Material")) {
            ImGui::ColorEdit3("Diffuse Color", &material.diffuseColor[0]);
            ImGui::ColorEdit3("Specular Color", &material.specularColor[0]);
            ImGui::DragFloat("Shininess", &material.shininess, 1.0f, 1.0f, 256.0f);

            ImGui::Separator();
            ImGui::Text("Ray Tracing Properties:");

            if (ImGui::Checkbox("Enable Mirror Reflections", &material.isMirror)) {
                if (material.isMirror && material.reflectivity == 0.0f) {
                    material.reflectivity = 0.8f;
                }
            }
            if (material.isMirror) {
                ImGui::DragFloat("Reflectivity", &material.reflectivity, 0.01f, 0.0f, 1.0f);
                ImGui::Text("Reflects other objects in the scene");
            }

            if (ImGui::Checkbox("Enable Transparency", &material.isTransparent)) {
                if (material.isTransparent && material.transparency == 0.0f) {
                    material.transparency = 0.7f;
                }
            }
            if (material.isTransparent) {
                ImGui::DragFloat("Transparency", &material.transparency, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Refractive Index", &material.refractiveIndex, 0.01f, 1.0f, 2.5f);
                ImGui::Text("Allows light to pass through the object");
            }

            if (material.isMirror && material.isTransparent) {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Warning: Glass material (reflective + transparent)");
            }

            ImGui::TreePop();
        }
    }

    void showFileDialogWindow() {
        ImGui::Begin("Load OBJ File", &showFileDialog);

        ImGui::InputText("File path", filePath, sizeof(filePath));
        ImGui::Text("Example: ../models/cube.obj");

        if (ImGui::Button("Load as Mirror")) {
            loadOBJModelWithMaterial(filePath, true, false);
            showFileDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Load as Glass")) {
            loadOBJModelWithMaterial(filePath, false, true);
            showFileDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Load as Normal")) {
            loadOBJModel(filePath);
            showFileDialog = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            showFileDialog = false;
        }

        ImGui::End();
    }

    void showLightingControls() {
        if (ImGui::TreeNode("Lighting Settings")) {
            ImGui::Text("Shading Model: Phong (Ray Tracing)");

            glm::vec3 ambient = scene.ambientLight;
            if (ImGui::ColorEdit3("Ambient Light", &ambient[0])) {
                scene.ambientLight = ambient;
            }

            if (ImGui::Button("Add Point Light")) {
                addLightToScene();
            }

            ImGui::Separator();
            ImGui::Text("Lights:");

            auto& lights = scene.lights;
            for (size_t i = 0; i < lights.size(); ++i) {
                if (lights[i] && lights[i]->light) {
                    ImGui::PushID(static_cast<int>(i));

                    if (ImGui::TreeNode(lights[i]->name.c_str())) {
                        showLightControls(*lights[i]);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }
    }

    std::unique_ptr<Mesh> createLightMesh() {
        auto mesh = std::make_unique<Mesh>(Mesh::createLightBox(0.3f, 1, 1));
        mesh->name = "LightCapsule";
        mesh->material.diffuseColor = glm::vec3(1.0f, 1.0f, 0.8f);
        return mesh;
    }

    void addLightToScene() {
        static int lightCount = 0;
        std::string lightName = "PointLight_" + std::to_string(lightCount++);
        auto lightNode = scene.getRoot()->createChild(lightName);

        lightNode->light = std::make_unique<Light>(glm::vec3(0.0f, 6.2f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.5f );
        lightNode->mesh = createLightMesh();
        lightNode->mesh->position = lightNode->light->position;

        scene.addLight(lightNode);
        selectedNode = lightNode;
    }

    void showLightControls(SceneNode& lightNode) {
        if (!lightNode.light) return;

        auto& light = *lightNode.light;

        if (ImGui::DragFloat3("Position", &light.position[0], 0.1f, -50.0f, 50.0f)) {
            if (lightNode.mesh) {
                lightNode.mesh->position = light.position;
            }
        }

        ImGui::ColorEdit3("Color", &light.color[0]);
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 10.0f);

        if (ImGui::Button("Delete Light")) {
            deleteLight(lightNode);
        }
    }

    void deleteLight(SceneNode& lightNode) {
        scene.removeLight(&lightNode);
        lightNode.removeFromParent();

        if (selectedNode == &lightNode) {
            selectedNode = nullptr;
        }
    }

    void loadOBJModel(const std::string& filename) {
        loadOBJModelWithMaterial(filename, false, false);
    }

    void loadOBJModelWithMaterial(const std::string& filename, bool isMirror, bool isTransparent) {
        try {
            Mesh mesh = OBJLoader::loadFromFile(filename);
            if (!mesh.faces.empty()) {
                std::string objectName = filename.substr(filename.find_last_of("/\\") + 1);
                objectName = objectName.substr(0, objectName.find_last_of('.'));

                auto modelNode = scene.getRoot()->createChild(objectName);
                modelNode->mesh = std::make_unique<Mesh>(std::move(mesh));

                if (isMirror) {
                    modelNode->mesh->material.diffuseColor = glm::vec3(0.8f, 0.8f, 1.0f);
                    modelNode->mesh->material.isMirror = true;
                    modelNode->mesh->material.reflectivity = 0.8f;
                    objectName += " (Mirror)";
                }
                else if (isTransparent) {
                    modelNode->mesh->material.diffuseColor = glm::vec3(0.9f, 1.0f, 0.9f);
                    modelNode->mesh->material.isTransparent = true;
                    modelNode->mesh->material.transparency = 0.7f;
                    objectName += " (Glass)";
                }

                modelNode->name = objectName;
                selectedNode = modelNode;

                modelNode->mesh->calculateVertexNormals();

                std::cout << "Model loaded successfully: " << objectName << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading model: " << e.what() << std::endl;
        }
    }

    void clearScene() {
        std::vector<SceneNode*> lightsToKeep;
        for (auto* light : scene.lights) {
            if (light && light->light) {
                lightsToKeep.push_back(light);
            }
        }

        scene.getRoot()->children.clear();

        for (auto* light : lightsToKeep) {
            scene.getRoot()->children.push_back(std::unique_ptr<SceneNode>(light));
        }
        scene.lights = lightsToKeep;

        selectedNode = nullptr;
        std::cout << "Scene cleared (lights preserved)" << std::endl;
    }

    void resetTransform() {
        if (selectedNode && selectedNode->mesh) {
            selectedNode->mesh->position = glm::vec3(0.0f);
            selectedNode->mesh->rotation = glm::vec3(0.0f);
            selectedNode->mesh->scale = glm::vec3(1.0f);
        }
    }

    void deleteSelectedObject() {
        if (selectedNode && selectedNode->parent) {
            if (selectedNode->light) {
                scene.removeLight(selectedNode);
            }
            selectedNode->removeFromParent();
            selectedNode = nullptr;
        }
    }

    void duplicateObject(SceneNode* node) {
        if (!node || !node->mesh) return;

        auto newNode = scene.getRoot()->createChild(node->name + "_Copy");
        newNode->mesh = std::make_unique<Mesh>(*node->mesh);
        newNode->mesh->position += glm::vec3(2.0f, 0.0f, 0.0f);
        selectedNode = newNode;
    }
};