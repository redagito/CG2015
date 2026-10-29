#include "graphics/renderer/CFrameBuffer.h"
#include "graphics/renderer/Draw.h"
#include "graphics/renderer/pass/deferred/CInstancedDraw.h"
#include "graphics/renderer/pass/deferred/MaterialDraw.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <glm/ext.hpp>
#include <json/json.h>
#include <stdexcept>
#include <thread>

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
bool runEnemyHighlightTests(const CGlfwWindow &)
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        CGraphicsResourceManager graphics;
        resources->addResourceListener(&graphics);
        // Exercise the same asynchronous dependency preparation used by the loading screen.
        const char *manifest = "build/enemy-highlight-assets.json";
        {
            std::ofstream file(manifest);
            file << R"({"assets":[
                {"kind":"material","file":"data/material/enemy.json"},
                {"kind":"material","file":"data/material/enemy_guardian.json"},
                {"kind":"material","file":"data/material/line_metal.json"},
                {"kind":"mesh","file":"data/mesh/enemy.obj"},
                {"kind":"mesh","file":"data/mesh/ship_2.obj"},
                {"kind":"mesh","file":"data/mesh/screen_quad.obj"},
                {"kind":"shader","file":"data/shader/deferred/geometry_pass.ini"},
                {"kind":"shader","file":"data/shader/deferred/illumination_pass.ini"},
                {"kind":"json","file":"data/world/game_1.json"}
            ]})";
        }
        const auto request = resources->loadManifestAsync(manifest);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        for (;;)
        {
            resources->pumpAsyncLoads(4);
            const auto status = resources->getLoadStatus(request);
            require(status.phase != LoadPhase::Failed, status.error.c_str());
            if (status.phase == LoadPhase::Ready)
                break;
            require(std::chrono::steady_clock::now() < deadline,
                    "Enemy material loading timed out");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        resources->setCacheOnly(true);
        Json::Value scene;
        require(resources->loadJsonDocument("data/world/game_1.json", scene) &&
                    scene["ambient_light"]["intensity"].asFloat() == .2f &&
                    scene["directional_lights"].size() == 2,
                "The darker gameplay lighting was not restored");
        require(
            resources->getMaterialSettings(resources->loadMaterial("data/material/line_metal.json"))
                    .emissiveAccents.strength == 0,
            "Enemy markings leaked into the player's material");

        constexpr unsigned size = 256;
        const auto texture = [](GLint format)
        {
            auto image = std::make_shared<CTexture>();
            require(image->init(size, size, format), "Highlight render texture allocation failed");
            return image;
        };
        auto surface = texture(GL_RGBA16F), normal = texture(GL_RGBA16F),
             depth = texture(GL_DEPTH_COMPONENT24);
        auto output = texture(GL_RGBA16F), darkLight = texture(GL_RGBA16F);
        const float black[] = {0, 0, 0, 0};
        glClearTexImage(darkLight->getId(), 0, GL_RGBA, GL_FLOAT, black);
        CFrameBuffer geometryTarget, outputTarget;
        geometryTarget.attach(surface, GL_COLOR_ATTACHMENT0);
        geometryTarget.attach(normal, GL_COLOR_ATTACHMENT1);
        geometryTarget.attach(depth, GL_DEPTH_ATTACHMENT);
        outputTarget.attach(output, GL_COLOR_ATTACHMENT0);
        auto *geometry = graphics.getShaderProgram(
            resources->loadShader("data/shader/deferred/geometry_pass.ini"));
        auto *compose = graphics.getShaderProgram(
            resources->loadShader("data/shader/deferred/illumination_pass.ini"));
        auto *quad = graphics.getMesh(resources->loadMesh("data/mesh/screen_quad.obj"));
        require(geometry && compose && quad, "Prepared highlight render assets are missing");
        const char *meshes[] = {"data/mesh/enemy.obj", "data/mesh/ship_2.obj"};
        const char *materials[] = {"data/material/enemy.json", "data/material/enemy_guardian.json"};
        for (unsigned type = 0; type < 2; ++type)
        {
            const auto meshId = resources->loadMesh(meshes[type]);
            const auto materialId = resources->loadMaterial(materials[type]);
            const auto authored = resources->getMaterialSettings(materialId);
            require(authored.emissiveAccents.strength > .5f,
                    "Async loading lost enemy glow settings");
            auto invalid = authored;
            invalid.emissiveAccents.hullStripWidth = 0;
            require(!resources->setMaterialSettings(materialId, invalid),
                    "Zero-width hull strips were accepted");
            invalid = authored;
            invalid.emissiveAccents.engineHalfHeight = -1;
            require(!resources->setMaterialSettings(materialId, invalid),
                    "Negative engine size was accepted");
            // Top-down and stern views exercise hull strips and rear-facing exhausts separately.
            for (int view = 0; view < 2; ++view)
                for (int transformed = 0; transformed < 2; ++transformed)
                    for (bool enabled : {true, false})
                    {
                        auto settings = authored;
                        if (!enabled)
                            settings.emissiveAccents.strength = 0;
                        require(resources->setMaterialSettings(materialId, settings),
                                "Cannot update accent settings");
                        const glm::vec3 translation =
                            transformed ? glm::vec3(20, 5, 30) : glm::vec3(0);
                        const auto rotation =
                            glm::rotate(glm::mat4(1), transformed ? .7f : 0.f, glm::vec3(0, 1, 0));
                        const auto eye =
                            translation + glm::vec3(rotation * glm::vec4(view ? glm::vec3(0, 0, -25)
                                                                              : glm::vec3(0, 25, 0),
                                                                         0));
                        const auto up =
                            glm::vec3(rotation *
                                      glm::vec4(view ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1), 0));
                        geometryTarget.setActive(GL_FRAMEBUFFER);
                        glViewport(0, 0, size, size);
                        glEnable(GL_DEPTH_TEST);
                        glDepthMask(GL_TRUE);
                        glDisable(GL_BLEND);
                        glDisable(GL_CULL_FACE);
                        glClearBufferfv(GL_COLOR, 0, black);
                        glClearBufferfv(GL_COLOR, 1, black);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        geometry->setUniform("normal_mapping_enabled", 0);
                        geometry->setUniform("view", glm::lookAt(eye, translation, up));
                        geometry->setUniform("projection",
                                             glm::ortho(-10.f, 10.f, -10.f, 10.f, .5f, 50.f));
                        if (transformed)
                        {
                            CInstancedDraw batch;
                            batch.begin();
                            batch.add(meshId, materialId,
                                      glm::translate(glm::mat4(1), translation) * rotation, false);
                            batch.draw(graphics, *geometry);
                        }
                        else
                            drawMaterialMesh(graphics.getMesh(meshId), glm::mat4(1), rotation,
                                             glm::mat4(1), graphics.getMaterial(materialId),
                                             graphics, geometry);
                        outputTarget.setActive(GL_FRAMEBUFFER);
                        glDisable(GL_DEPTH_TEST);
                        surface->setActive(5);
                        darkLight->setActive(6);
                        compose->setUniform("screen_width", float(size));
                        compose->setUniform("screen_height", float(size));
                        compose->setUniform("diffuse_glow_texture", 5);
                        compose->setUniform("light_texture", 6);
                        compose->setUniform("specular_light_texture", 6);
                        compose->setActive();
                        draw(*quad);
                        // Read only these isolated test textures; never capture a display.
                        std::vector<float> pixels(size * size * 4);
                        glGetTextureImage(output->getId(), 0, GL_RGBA, GL_FLOAT,
                                          GLsizei(pixels.size() * sizeof(float)), pixels.data());
                        unsigned glowing = 0, dark = 0;
                        float peak = 0, peakLuminance = 0;
                        for (std::size_t i = 0; i < pixels.size(); i += 4)
                        {
                            require(std::isfinite(pixels[i]), "Enemy glow became non-finite");
                            peak = std::max(peak, pixels[i]);
                            peakLuminance = std::max(peakLuminance, pixels[i] * .2126f +
                                                                        pixels[i + 1] * .7152f +
                                                                        pixels[i + 2] * .0722f);
                            if (pixels[i] > .5f)
                            {
                                ++glowing;
                                require(pixels[i] > pixels[i + 1] * 8 &&
                                            pixels[i] > pixels[i + 2] * 8,
                                        "Enemy accents do not emit red light");
                            }
                            if (pixels[i] < .01f)
                                ++dark;
                        }
                        std::printf("Enemy %u / %s / instanced %d / accents %d: %u glowing pixels, "
                                    "peak %.2f\n",
                                    type, view ? "stern" : "hull", transformed, enabled, glowing,
                                    peak);
                        require(enabled ? glowing > 10 && peak > 2 && peakLuminance > 1.4f
                                        : peak < .001f,
                                "Hull strips/exhausts are missing in darkness or disabled markings "
                                "still glow");
                        require(dark > size * size * 9 / 10,
                                "Enemy glow covers too much of the image");
                    }
            require(resources->setMaterialSettings(materialId, authored),
                    "Cannot restore authored enemy material");
        }
        std::puts("Enemy highlights: async materials, dark lighting, red hull/exhaust emission, "
                  "instancing, and opt-out passed.");
        return true;
    }
    catch (const std::exception &error)
    {
        std::fprintf(stderr, "Enemy highlight regression: %s\n", error.what());
        return false;
    }
}
