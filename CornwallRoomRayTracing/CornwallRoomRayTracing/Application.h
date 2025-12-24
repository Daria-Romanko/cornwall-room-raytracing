#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "Scene.h"
#include "RenderStrategy.h"
#include "ImGuiManager.h"
#include "CornellRoom.h"
#include "OBJLoader.h"

class Application {
public:
    Application() {
        window.create(sf::VideoMode({ 1200,800 }), "3D Renderer");

        setupScene();
        setupRendering();

        imguiManager = std::make_unique<ImGuiManager>(window, *scene, cornellRoom.get());
    }

    void run() {
        sf::Clock clock;

        while (window.isOpen()) {
            float deltaTime = clock.restart().asSeconds();

            handleEvents();
            update(deltaTime);
            render();
        }
    }

private:
    sf::RenderWindow window;
    std::unique_ptr<ImGuiManager> imguiManager;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<CornellRoom> cornellRoom;
    std::unique_ptr<RenderStrategy> renderStrategy;

    bool showRayTracingResult = false;
    std::unique_ptr<sf::Texture> rayTracingTexture;
    bool needsRayTracingRender = false;

    void handleEvents() {
        while (auto event = window.pollEvent()) {
            imguiManager->processEvent(*event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (!ImGui::GetIO().WantCaptureMouse) {
                handleCameraInput(*event);
            }
        }

        if (imguiManager->shouldRenderRayTracing() && !showRayTracingResult) {
            std::cout << "Starting one-time ray tracing..." << std::endl;
            showRayTracingResult = true;
            needsRayTracingRender = true;
            imguiManager->setShowRayTracingResult(true);
            imguiManager->resetRenderFlags();
        }

        if (imguiManager->shouldReturnToEditing() && showRayTracingResult) {
            std::cout << "Returning to wireframe editing..." << std::endl;
            showRayTracingResult = false;
            rayTracingTexture.reset();
            imguiManager->setShowRayTracingResult(false);
            imguiManager->resetRenderFlags();
        }
    }

    void update(float deltaTime) {
        imguiManager->update(deltaTime);

        if (scene) {
            scene->getRoot()->update(deltaTime);
        }
    }

    void render() {
        window.clear(sf::Color::Black);

        if (showRayTracingResult) {
            if (needsRayTracingRender) {
                renderRayTracingOnce();
                needsRayTracingRender = false;
            }

            if (rayTracingTexture) {
                sf::Sprite sprite(*rayTracingTexture);
                window.draw(sprite);
            }
        }
        else {
            if (renderStrategy && scene) {
                renderStrategy->render(window, *scene);
            }
        }

        imguiManager->showSceneEditor();
        imguiManager->render();

        window.display();
    }

    void renderRayTracingOnce() {
        std::cout << "Performing one-time ray tracing render..." << std::endl;

        RayTracingStrategy rayTracer;

        sf::Image rayTracedImage = sf::Image(sf::Vector2u(window.getSize().x, window.getSize().y), sf::Color::Black);
        rayTracer.renderToImage(rayTracedImage, *scene);

        rayTracingTexture = std::make_unique<sf::Texture>();
        rayTracingTexture->loadFromImage(rayTracedImage);

        std::cout << "Ray tracing completed and saved to texture." << std::endl;
    }

    void setupScene() {
        scene = std::make_unique<Scene>();

        cornellRoom = std::make_unique<CornellRoom>(15.0f);
        cornellRoom->addToScene(*scene);

        addTopLight();
        loadObjects();

        scene->getCamera()->position = glm::vec3(-1.9f, 2.6f, 38.2f);
        scene->getCamera()->target = glm::vec3(-1.7f, 0, 0);
    }

    void addTopLight() {
        auto lightNode = scene->getRoot()->createChild("TopLight");
        lightNode->light = std::make_unique<Light>(
            glm::vec3(0, 7.0f, 0),
            glm::vec3(1.0f, 1.0f, 0.9f),
            1.5f
        );

        lightNode->mesh = createLightMesh();
        lightNode->mesh->position = glm::vec3(0, 7.4f, 0);
        lightNode->mesh->material.diffuseColor = glm::vec3(1.0f, 1.0f, 0.8f);

        scene->addLight(lightNode);
    }

    void loadObjects() {
        try {
            const float roomHalf = 7.5f;
            const float floorY = -roomHalf;

            Mesh sphereBase = Mesh::createSphereUV(1.0f, 32, 64);
            if (!sphereBase.faces.empty()) {
                {
                    auto n = scene->getRoot()->createChild("Sphere_A");
                    n->mesh = std::make_unique<Mesh>(sphereBase);
                    n->mesh->name = "Sphere_A";

                    float scale = 2.0f;
                    n->mesh->scale = glm::vec3(scale);
                    n->mesh->position = glm::vec3(2.6f, -3.5f, 1.2f);

                    n->mesh->material.diffuseColor = glm::vec3(0.98f, 0.78f, 0.18f);
                    n->mesh->calculateVertexNormals();
                }

                {
                    auto n = scene->getRoot()->createChild("Sphere_B");
                    n->mesh = std::make_unique<Mesh>(sphereBase);
                    n->mesh->name = "Sphere_B";

                    float scale = 1.3f;
                    n->mesh->scale = glm::vec3(scale);
                    n->mesh->position = glm::vec3(-3.5f, 1.3f, -2.6f);

                    n->mesh->material.diffuseColor = glm::vec3(0.25f, 0.85f, 0.75f);
                    n->mesh->calculateVertexNormals();
                }
            }

            Mesh cubeBase = OBJLoader::loadFromFile("../models/cube.obj");
            if (!cubeBase.faces.empty()) {
                {
                    auto n = scene->getRoot()->createChild("Cube_A");
                    n->mesh = std::make_unique<Mesh>(cubeBase);
                    n->mesh->name = "Cube_A";

                    glm::vec3 scale(2.8f, -3.7f, 2.8f);
                    n->mesh->scale = scale;
                    n->mesh->position = glm::vec3(-3.5f, -3.7f, -2.6f);
                    n->mesh->rotation = glm::radians(glm::vec3(0.0f, 18.0f, 0.0f));

                    n->mesh->material.diffuseColor = glm::vec3(0.92f, 0.92f, 0.94f);
                    n->mesh->calculateVertexNormals();
                }

                {
                    auto n = scene->getRoot()->createChild("Cube_B");
                    n->mesh = std::make_unique<Mesh>(cubeBase);
                    n->mesh->name = "Cube_B";

                    glm::vec3 scale(3.5f, 1.2f, 3.5f);
                    n->mesh->scale = scale;
                    n->mesh->position = glm::vec3(3.0f, -6.75f, 1.2f);
                    n->mesh->rotation = glm::radians(glm::vec3(0.0f, -22.0f, 0.0f));

                    n->mesh->material.diffuseColor = glm::vec3(0.70f, 0.55f, 0.95f);
                    n->mesh->calculateVertexNormals();
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error loading objects: " << e.what() << std::endl;
        }
    }


    std::unique_ptr<Mesh> createLightMesh() {
        auto mesh = std::make_unique<Mesh>(Mesh::createLightBox(6.0f, 0.15f, 6.0f));
        mesh->name = "LightCapsule";
        mesh->material.diffuseColor = glm::vec3(1.0f);
        return mesh;
    }

    void setupRendering() {
        renderStrategy = std::make_unique<WireframeStrategy>();
    }

    void handleCameraInput(const sf::Event& event) {
        
    }
};