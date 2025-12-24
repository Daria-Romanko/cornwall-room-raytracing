#pragma once
#include <SFML/Graphics.hpp>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <thread>
#include <vector>
#include <atomic>
#include <future>
#include <algorithm>
#include <limits>
#include <cmath>
#include <array>

#include "Scene.h"
#include "Mesh.h"
#include "Camera.h"
#include <iostream>

class RenderStrategy {
public:
    virtual ~RenderStrategy() = default;
    virtual void render(sf::RenderWindow& window, Scene& scene) = 0;
};

class WireframeStrategy : public RenderStrategy {
public:
    void render(sf::RenderWindow& window, Scene& scene) override {
        auto meshes = scene.getAllMeshes();
        auto* camera = scene.getCamera();

        for (auto* mesh : meshes) {
            renderMesh(window, *mesh, *camera);
        }
    }

private:
    void renderMesh(sf::RenderWindow& window, const Mesh& mesh, const Camera& camera) {

        if (mesh.name == "Wall_FrontWall")
            return;

        auto view = camera.getViewMatrix();
        auto projection = camera.getProjectionMatrix();
        auto model = mesh.getTransformMatrix();
        auto mvp = projection * view * model;

        for (const auto& face : mesh.faces) {
            for (size_t i = 0; i < face.vertices.size(); ++i) {
                size_t next = (i + 1) % face.vertices.size();

                auto p1 = transformPoint(mvp, face.vertices[i].position);
                auto p2 = transformPoint(mvp, face.vertices[next].position);

                sf::Vector2f screen1 = toScreenCoords(p1, window.getSize());
                sf::Vector2f screen2 = toScreenCoords(p2, window.getSize());

                std::array<sf::Vertex, 2> line = {
                    sf::Vertex{screen1, sf::Color::White},
                    sf::Vertex{screen2, sf::Color::White}
                };
                window.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
            }
        }
    }

    glm::vec3 transformPoint(const glm::mat4& mvp, const glm::vec3& point) {
        glm::vec4 result = mvp * glm::vec4(point, 1.0f);
        if (result.w != 0) result /= result.w;
        return glm::vec3(result);
    }

    sf::Vector2f toScreenCoords(const glm::vec3& point, sf::Vector2u windowSize) {
        return sf::Vector2f(
            (point.x + 1.0f) * 0.5f * windowSize.x,
            (1.0f - point.y) * 0.5f * windowSize.y
        );
    }
};


class RayTracingStrategy {
public:
    void renderToImage(sf::Image& image, Scene& scene) {
        auto* camera = scene.getCamera();
        if (!camera) return;

        const unsigned width = image.getSize().x;
        const unsigned height = image.getSize().y;
        if (width == 0 || height == 0) return;

        std::vector<RTMesh>   meshes;
        std::vector<RTSphere> spheres;
        buildRTObjects(scene, meshes, spheres);

        auto lights = scene.getLights();

        const float fov = glm::radians(camera->fov);
        const float aspect = float(width) / float(height);
        const float scale = std::tan(fov * 0.5f);
        const glm::mat4 invView = glm::inverse(camera->getViewMatrix());

        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                float ndcX = (2.0f * (x + 0.5f) / float(width) - 1.0f);
                float ndcY = (1.0f - 2.0f * (y + 0.5f) / float(height));

                ndcX *= aspect * scale;
                ndcY *= scale;

                glm::vec3 rayDirCam = glm::normalize(glm::vec3(ndcX, ndcY, -1.0f));
                glm::vec3 rayDirWorld = glm::normalize(glm::vec3(invView * glm::vec4(rayDirCam, 0.0f)));
                glm::vec3 rayOrigin = camera->position;

                glm::vec3 color = traceRay(rayOrigin, rayDirWorld, meshes, spheres, lights, scene, 0, 1.0f);
                image.setPixel({ x, y }, toSFMLColor(color));
            }
        }
    }

private:
    static constexpr int   MAX_DEPTH = 6;
    static constexpr float EPS = 1e-3f;

    struct RTMesh {
        Mesh* mesh = nullptr;
        glm::mat4 model{ 1.0f };
        glm::mat4 invModel{ 1.0f };
        glm::mat3 normalMat{ 1.0f };
        bool isLight = false;
        bool isHidden = false; 
    };

    struct RTSphere {
        glm::vec3 center{ 0.0f };
        float     radius = 1.0f;
        Material  material{};
        bool isLight = false;
        bool isHidden = false;
    };

    struct HitInfo {
        float t = std::numeric_limits<float>::max();
        glm::vec3 p{ 0.0f };
        glm::vec3 nGeom{ 0.0f, 1.0f, 0.0f };
        glm::vec3 nShade{ 0.0f, 1.0f, 0.0f };
        bool frontFace = true;
        bool hit = false;
        bool hitLight = false;
        Material material{};
    };

private:
    static glm::vec3 reflectVec(const glm::vec3& v, const glm::vec3& nUnit) {
        return v - 2.0f * glm::dot(v, nUnit) * nUnit;
    }

    static float schlick(float cosTheta, float n1, float n2) {
        cosTheta = std::clamp(cosTheta, 0.0f, 1.0f);
        float r0 = (n1 - n2) / (n1 + n2);
        r0 = r0 * r0;
        float x = 1.0f - cosTheta;
        return r0 + (1.0f - r0) * x * x * x * x * x;
    }

    static bool refractVec(const glm::vec3& I_unit, const glm::vec3& N_unit, float eta, glm::vec3& outT) {
        float cosTheta = std::min(glm::dot(-I_unit, N_unit), 1.0f);
        glm::vec3 rOutPerp = eta * (I_unit + cosTheta * N_unit);
        float k = 1.0f - glm::dot(rOutPerp, rOutPerp);
        if (k < 0.0f) return false;
        glm::vec3 rOutParallel = -std::sqrt(k) * N_unit;
        outT = rOutPerp + rOutParallel;
        return true;
    }

    void buildRTObjects(Scene& scene, std::vector<RTMesh>& outMeshes, std::vector<RTSphere>& outSpheres) {
        auto meshes = scene.getAllMeshes();

        outMeshes.clear();
        outSpheres.clear();
        outMeshes.reserve(meshes.size());

        for (auto* m : meshes) {
            if (!m) continue;

            const bool isSphere = (m->name.find("Sphere") != std::string::npos);

            if (isSphere) {
                RTSphere s;
                s.isHidden = false;
                s.isLight = false;
                s.material = m->material;

                s.center = m->position;

                float r = std::max({ std::abs(m->scale.x), std::abs(m->scale.y), std::abs(m->scale.z) });
                s.radius = std::max(1e-4f, r);

                outSpheres.push_back(s);
                continue;
            }

            RTMesh r;
            r.mesh = m;

            r.isHidden = (m->name == "Wall_FrontWall");
            r.isLight = (m->name.find("LightCapsule") != std::string::npos ||
                m->name.find("Light_") != std::string::npos);

            r.model = m->getTransformMatrix();
            r.invModel = glm::inverse(r.model);
            r.normalMat = glm::transpose(glm::inverse(glm::mat3(r.model)));

            outMeshes.push_back(r);
        }
    }

    glm::vec3 traceRay(const glm::vec3& origin, const glm::vec3& dirUnit, const std::vector<RTMesh>& meshes, const std::vector<RTSphere>& spheres, const std::vector<Light>& lights, const Scene& scene, int depth, float environmentIor)
    {
        if (depth >= MAX_DEPTH) return scene.backgroundColor;

        HitInfo hit;
        const bool primary = (depth == 0);
        if (!intersectScene(origin, dirUnit, meshes, spheres, hit, primary))
            return scene.backgroundColor;

        if (hit.hitLight) return glm::vec3(1.0f);

        const Material& mat = hit.material;

        glm::vec3 direct = shadeDirect(hit, lights, scene, meshes, spheres);

        if (mat.isMirror && mat.reflectivity > 0.0f) {
            float k = std::clamp(mat.reflectivity, 0.0f, 1.0f);

            glm::vec3 R = glm::normalize(reflectVec(dirUnit, hit.nGeom));
            glm::vec3 o = hit.p + hit.nGeom * (glm::dot(R, hit.nGeom) > 0.0f ? EPS : -EPS);

            glm::vec3 refl = traceRay(o, R, meshes, spheres, lights, scene, depth + 1, environmentIor);
            return glm::clamp(direct * (1.0f - k) + refl * k, 0.0f, 1.0f);
        }

        if (mat.isTransparent && mat.transparency > 0.0f) {
            float tr = std::clamp(mat.transparency, 0.0f, 1.0f);

            float ior = (mat.refractiveIndex > 1e-4f) ? mat.refractiveIndex : 1.5f;
            float n1 = environmentIor;
            float n2 = hit.frontFace ? ior : 1.0f;
            float eta = n1 / n2;

            glm::vec3 N = hit.nGeom;
            float cosTheta = std::clamp(glm::dot(-dirUnit, N), 0.0f, 1.0f);

            float kr = schlick(cosTheta, n1, n2);
            kr = std::clamp(kr, 0.0f, 1.0f);

            // Если материал одновременно стекло+зеркало — усилим отражение
            if (mat.isMirror) kr = std::max(kr, std::clamp(mat.reflectivity, 0.0f, 1.0f));

            // reflect
            glm::vec3 R = glm::normalize(reflectVec(dirUnit, N));
            glm::vec3 oR = hit.p + N * (glm::dot(R, N) > 0.0f ? EPS : -EPS);
            glm::vec3 refl = traceRay(oR, R, meshes, spheres, lights, scene, depth + 1, environmentIor);

            // refract
            glm::vec3 refr(0.0f);
            glm::vec3 T;
            bool canRefract = refractVec(dirUnit, N, eta, T);

            if (canRefract) {
                T = glm::normalize(T);
                glm::vec3 oT = hit.p + N * (glm::dot(T, N) > 0.0f ? EPS : -EPS);

                float nextEnvIor = hit.frontFace ? ior : 1.0f;
                refr = traceRay(oT, T, meshes, spheres, lights, scene, depth + 1, nextEnvIor);

                refr *= mat.diffuseColor;
            }
            else {
                kr = 1.0f;
            }

            glm::vec3 glass = refl * kr + refr * (1.0f - kr);

            glm::vec3 out = direct * (1.0f - tr) + glass * tr;
            return glm::clamp(out, 0.0f, 1.0f);
        }

        return glm::clamp(direct, 0.0f, 1.0f);
    }

    bool intersectScene(const glm::vec3& origin, const glm::vec3& dirUnit, const std::vector<RTMesh>& meshes, const std::vector<RTSphere>& spheres, HitInfo& outHit, bool skipHiddenForPrimary)
    {
        bool hitAny = false;
        float nearest = std::numeric_limits<float>::max();

        for (const auto& s : spheres) {
            if (skipHiddenForPrimary && s.isHidden) continue;

            HitInfo h;
            if (intersectSphere(origin, dirUnit, s, h)) {
                if (h.t > EPS && h.t < nearest) {
                    nearest = h.t;
                    outHit = h;
                    outHit.hitLight = s.isLight;
                    hitAny = true;
                }
            }
        }

        for (const auto& m : meshes) {
            if (skipHiddenForPrimary && m.isHidden) continue;

            HitInfo h;
            if (intersectMesh(origin, dirUnit, m, h)) {
                if (h.t > EPS && h.t < nearest) {
                    nearest = h.t;
                    outHit = h;
                    outHit.hitLight = m.isLight;
                    hitAny = true;
                }
            }
        }

        return hitAny;
    }

    bool intersectSphere(const glm::vec3& o, const glm::vec3& d, const RTSphere& s, HitInfo& outHit)
    {
        glm::vec3 oc = o - s.center;

        float a = glm::dot(d, d);
        float halfB = glm::dot(oc, d);
        float c = glm::dot(oc, oc) - s.radius * s.radius;

        float disc = halfB * halfB - a * c;
        if (disc < 0.0f) return false;

        float sqrtD = std::sqrt(disc);

        float t = (-halfB - sqrtD) / a;
        if (t <= EPS) {
            t = (-halfB + sqrtD) / a;
            if (t <= EPS) return false;
        }

        outHit.t = t;
        outHit.p = o + d * t;

        glm::vec3 outward = glm::normalize(outHit.p - s.center);

        outHit.frontFace = (glm::dot(d, outward) < 0.0f);
        outHit.nGeom = outHit.frontFace ? outward : -outward;
        outHit.nShade = outHit.nGeom;

        outHit.material = s.material;
        outHit.hit = true;
        return true;
    }

    bool intersectMesh(const glm::vec3& originWorld, const glm::vec3& dirWorldUnit, const RTMesh& rt, HitInfo& outHit)
    {
        float bestT = std::numeric_limits<float>::max();
        bool hit = false;

        glm::vec3 localO = glm::vec3(rt.invModel * glm::vec4(originWorld, 1.0f));
        glm::vec3 localD = glm::normalize(glm::vec3(rt.invModel * glm::vec4(dirWorldUnit, 0.0f)));

        for (const auto& face : rt.mesh->faces) {
            const size_t n = face.vertices.size();
            if (n < 3) continue;

            for (size_t i = 1; i + 1 < n; ++i) {
                const auto& V0 = face.vertices[0];
                const auto& V1 = face.vertices[i];
                const auto& V2 = face.vertices[i + 1];

                float tLocal, u, v;
                if (!rayTri(localO, localD, V0.position, V1.position, V2.position, tLocal, u, v))
                    continue;
                if (tLocal <= EPS) continue;

                glm::vec3 localP = localO + localD * tLocal;
                glm::vec3 worldP = glm::vec3(rt.model * glm::vec4(localP, 1.0f));
                float tWorld = glm::dot(worldP - originWorld, dirWorldUnit);

                if (tWorld > EPS && tWorld < bestT) {
                    bestT = tWorld;
                    outHit.t = tWorld;
                    outHit.p = worldP;

                    // geom normal
                    glm::vec3 localNg = glm::normalize(glm::cross(V1.position - V0.position, V2.position - V0.position));
                    glm::vec3 Ng = glm::normalize(rt.normalMat * localNg);

                    // shading normal
                    float w = 1.0f - u - v;
                    glm::vec3 localNs = glm::normalize(V0.normal * w + V1.normal * u + V2.normal * v);
                    glm::vec3 Ns = glm::normalize(rt.normalMat * localNs);

                    bool front = (glm::dot(dirWorldUnit, Ng) < 0.0f);
                    outHit.frontFace = front;
                    outHit.nGeom = front ? Ng : -Ng;
                    outHit.nShade = front ? Ns : -Ns;

                    outHit.material = rt.mesh->material;
                    outHit.hit = true;
                    hit = true;
                }
            }
        }

        return hit;
    }

    static bool rayTri(const glm::vec3& o, const glm::vec3& d, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v)
    {
        constexpr float EPS_MT = 1e-6f;
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec3 p = glm::cross(d, e2);
        float det = glm::dot(e1, p);
        if (std::fabs(det) < EPS_MT) return false;

        float invDet = 1.0f / det;
        glm::vec3 tv = o - v0;

        u = glm::dot(tv, p) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 q = glm::cross(tv, e1);
        v = glm::dot(d, q) * invDet;
        if (v < 0.0f || (u + v) > 1.0f) return false;

        t = glm::dot(e2, q) * invDet;
        return t > EPS_MT;
    }

    glm::vec3 shadeDirect(const HitInfo& hit,
        const std::vector<Light>& lights,
        const Scene& scene,
        const std::vector<RTMesh>& meshes,
        const std::vector<RTSphere>& spheres)
    {
        const Material& m = hit.material;

        glm::vec3 N = glm::normalize(hit.nShade);
        glm::vec3 V = glm::normalize(scene.getCamera()->position - hit.p);

        glm::vec3 col = scene.ambientLight * m.diffuseColor;

        for (const auto& Ls : lights) {
            glm::vec3 toL = Ls.position - hit.p;
            float dist = glm::length(toL);
            if (dist <= 1e-6f) continue;
            glm::vec3 L = toL / dist;

            if (inShadow(hit.p, hit.nGeom, L, dist, meshes, spheres))
                continue;

            float ndotl = std::max(glm::dot(N, L), 0.0f);
            if (ndotl <= 0.0f) continue;

            float atten = 1.0f / (1.0f + 0.1f * dist + 0.01f * dist * dist);
            glm::vec3 lightCol = Ls.color * Ls.intensity * atten;

            // diffuse
            col += lightCol * (m.diffuseColor * ndotl);

            // spec
            if (m.shininess > 1.0f) {
                glm::vec3 R = reflectVec(-L, N);
                float spec = std::pow(std::max(glm::dot(V, glm::normalize(R)), 0.0f), m.shininess);
                col += lightCol * (m.specularColor * spec);
            }
        }

        return col;
    }

    bool inShadow(const glm::vec3& p, const glm::vec3& Ng, const glm::vec3& lightDir, float maxDist, const std::vector<RTMesh>& meshes, const std::vector<RTSphere>& spheres)
    {
        glm::vec3 n = glm::normalize(Ng);
        glm::vec3 o = p + n * (glm::dot(lightDir, n) > 0.0f ? EPS : -EPS);

        for (const auto& s : spheres) {
            if (s.material.isTransparent && s.material.transparency > 0.0f) continue;

            HitInfo h;
            if (intersectSphere(o, lightDir, s, h)) {
                if (h.t > EPS && h.t < maxDist - EPS) return true;
            }
        }

        for (const auto& m : meshes) {
            if (m.isLight) continue;
            if (m.mesh->material.isTransparent && m.mesh->material.transparency > 0.0f) continue;

            HitInfo h;
            if (intersectMesh(o, lightDir, m, h)) {
                if (h.t > EPS && h.t < maxDist - EPS) return true;
            }
        }

        return false;
    }

    static sf::Color toSFMLColor(const glm::vec3& colorLinear01) {
        glm::vec3 c = glm::clamp(colorLinear01, 0.0f, 1.0f);

        c.r = std::pow(c.r, 1.0f / 2.2f);
        c.g = std::pow(c.g, 1.0f / 2.2f);
        c.b = std::pow(c.b, 1.0f / 2.2f);

        auto toByte = [](float v) -> std::uint8_t {
            v = std::clamp(v, 0.0f, 1.0f);
            return static_cast<std::uint8_t>(v * 255.0f + 0.5f);
            };

        return { toByte(c.r), toByte(c.g), toByte(c.b) };
    }
};
