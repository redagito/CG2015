#include "graphics/camera/CFirstPersonCamera.h"
#include "graphics/renderer/CFrameBuffer.h"
#include "graphics/renderer/Draw.h"
#include "graphics/renderer/pass/deferred/CInstancedDraw.h"
#include "graphics/renderer/pass/deferred/MaterialDraw.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/system/CGraphicsSystem.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include "support/TestViewport.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <glm/ext.hpp>
#include <stdexcept>

bool runShaderTests(const CGlfwWindow &window)
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        CGraphicsResourceManager graphics;
        resources->addResourceListener(&graphics);
        const auto shader = [&](const char *path)
        {
            auto *program = graphics.getShaderProgram(resources->loadShader(path));
            if (!program || !program->isValid())
                throw std::runtime_error("Shader failed to initialize");
            program->setUniform("screen_width", static_cast<float>(window.getWidth()));
            program->setUniform("screen_height", static_cast<float>(window.getHeight()));
            return program;
        };
        const auto texture = [&](GLint format)
        {
            auto result = std::make_shared<CTexture>();
            if (!result->init(window.getWidth(), window.getHeight(), format))
                throw std::runtime_error("Texture allocation failed");
            return result;
        };
        auto diffuse = texture(GL_RGBA16F), normal = texture(GL_RGBA16F);
        CFrameBuffer target;
        target.attach(diffuse, GL_COLOR_ATTACHMENT0);
        target.attach(normal, GL_COLOR_ATTACHMENT1);
        target.setActive(GL_FRAMEBUFFER);
        glViewport(0, 0, window.getWidth(), window.getHeight());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        const auto read = [&](GLenum attachment)
        {
            // Numeric samples from this test's own FBO; no display capture.
            glReadBuffer(attachment);
            std::array<float, 4> value{};
            glReadPixels(window.getWidth() / 2, window.getHeight() / 2, 1, 1, GL_RGBA, GL_FLOAT,
                         value.data());
            for (float component : value)
                if (!std::isfinite(component))
                    throw std::runtime_error("Non-finite shader output");
            return value;
        };
        auto *geometry = shader("data/shader/deferred/geometry_pass.ini");
        CMesh triangle({-1.f, -1.f, 0.f, 1.f, -1.f, 0.f, 0.f, 1.f, 0.f}, {},
                       {1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f}, std::vector<float>(6, 0.f),
                       EPrimitiveType::Triangle);
        geometry->setUniform("model", glm::scale(glm::mat4(1.f), glm::vec3(2.f, 1.f, 1.f)));
        geometry->setUniform("view", glm::mat4(1.f));
        geometry->setUniform("projection", glm::mat4(1.f));
        graphics.getDefaultDiffuseTexture()->setActive(0);
        graphics.getDefaultNormalTexture()->setActive(1);
        graphics.getDefaultSpecularTexture()->setActive(2);
        graphics.getDefaultGlowTexture()->setActive(3);
        graphics.getDefaultAlphaTexture()->setActive(4);
        geometry->setUniform("diffuse_texture", 0);
        geometry->setUniform("normal_texture", 1);
        geometry->setUniform("specular_texture", 2);
        geometry->setUniform("glow_texture", 3);
        geometry->setUniform("alpha_texture", 4);
        for (int mapping : {0, 1})
        {
            geometry->setUniform("normal_mapping_enabled", mapping);
            draw(triangle);
            const auto result = read(GL_COLOR_ATTACHMENT1);
            const auto expected = glm::normalize(glm::vec3(0.5f, 0.f, 1.f));
            if (glm::distance(glm::vec3(result[0], result[1], result[2]), expected) > 0.003f)
                throw std::runtime_error("Normal transform/degenerate UV handling is incorrect");
        }

        // Compare batched and reference transforms, including mirrored/nonuniform scales.
        {
            auto batchColor = texture(GL_RGBA16F), batchNormal = texture(GL_RGBA16F);
            CFrameBuffer batchTarget;
            batchTarget.attach(batchColor, GL_COLOR_ATTACHMENT0);
            batchTarget.attach(batchNormal, GL_COLOR_ATTACHMENT1);
            batchTarget.setActive(GL_FRAMEBUFFER);
            const auto meshId = resources->loadMesh("data/mesh/screen_quad.obj");
            const auto materialId = resources->loadMaterial("data/material/white.json");
            auto *mesh = graphics.getMesh(meshId);
            auto *material = graphics.getMaterial(materialId);
            const glm::vec3 positions[] = {{-.5f, 0, 0}, {.1f, .3f, 0}, {.55f, -.3f, 0}};
            const glm::vec3 scales[] = {{.2f, .3f, 1}, {.15f, .2f, .8f}, {-.15f, .2f, 1.2f}};
            geometry->setUniform("view", glm::mat4(1));
            geometry->setUniform("projection", glm::mat4(1));
            geometry->setUniform("normal_mapping_enabled", 0);
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            const auto clear = [&]
            {
                const float black[] = {0, 0, 0, 0};
                glClearBufferfv(GL_COLOR, 0, black);
                glClearBufferfv(GL_COLOR, 1, black);
            };
            const auto image = [&](GLenum attachment)
            {
                std::vector<float> result(window.getWidth() * window.getHeight() * 4);
                glReadBuffer(attachment);
                glReadPixels(0, 0, window.getWidth(), window.getHeight(), GL_RGBA, GL_FLOAT,
                             result.data());
                return result;
            };
            clear();
            for (int i = 0; i < 3; ++i)
                drawMaterialMesh(mesh, glm::translate(glm::mat4(1), positions[i]), glm::mat4(1),
                                 glm::scale(glm::mat4(1), scales[i]), material, graphics, geometry);
            const auto referenceColor = image(GL_COLOR_ATTACHMENT0),
                       referenceNormal = image(GL_COLOR_ATTACHMENT1);
            clear();
            CInstancedDraw batch;
            batch.begin();
            for (int i = 0; i < 3; ++i)
                batch.add(meshId, materialId,
                          glm::translate(glm::mat4(1), positions[i]) *
                              glm::scale(glm::mat4(1), scales[i]),
                          scales[i].x < 0);
            batch.draw(graphics, *geometry);
            const auto actualColor = image(GL_COLOR_ATTACHMENT0),
                       actualNormal = image(GL_COLOR_ATTACHMENT1);
            unsigned covered = 0;
            for (std::size_t i = 0; i < actualColor.size(); ++i)
            {
                if (referenceColor[i] > .1f)
                    ++covered;
                if (std::abs(referenceColor[i] - actualColor[i]) > .003f ||
                    std::abs(referenceNormal[i] - actualNormal[i]) > .003f)
                    throw std::runtime_error(
                        "Instanced transforms/materials differ from reference draws");
            }
            if (covered < 20)
                throw std::runtime_error("Instancing reference did not draw visible geometry");
        }
        auto output = texture(GL_RGBA16F);
        CFrameBuffer screen;
        screen.attach(output, GL_COLOR_ATTACHMENT0);
        screen.setActive(GL_FRAMEBUFFER);
        auto *quad = graphics.getMesh(resources->loadMesh("data/mesh/screen_quad.obj"));
        auto depth = texture(GL_DEPTH_COMPONENT24), shadow = texture(GL_DEPTH_COMPONENT24);
        float depthValue = 0.5f, shadowValue = 0.f;
        glClearTexImage(depth->getId(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);
        glClearTexImage(shadow->getId(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, &shadowValue);
        shadow->setParameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        shadow->setParameter(GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        depth->setActive(0);
        normal->setActive(1);
        shadow->setActive(2);
        auto *directional = shader("data/shader/deferred/directional_light_pass.ini");
        directional->setUniform("depth_texture", 0);
        directional->setUniform("normal_specular_texture", 1);
        directional->setUniform("shadow_map", 2);
        directional->setUniform("inverse_view_projection", glm::mat4(1.f));
        directional->setUniform("shadow_view_projection_bias",
                                glm::translate(glm::mat4(1.f), glm::vec3(0.5f)));
        directional->setUniform("light_direction", glm::vec3(0.f, 0.f, -1.f));
        directional->setUniform("light_color", glm::vec3(1.f));
        directional->setUniform("light_intensity", 1.f);
        directional->setUniform("shadows_enabled", 1);
        draw(*quad);
        if (read(GL_COLOR_ATTACHMENT0)[0] > 0.001f)
            throw std::runtime_error("Shadow test fixture is not occluded");
        directional->setUniform("shadows_enabled", 0);
        draw(*quad);
        if (read(GL_COLOR_ATTACHMENT0)[0] < 0.8f)
            throw std::runtime_error("Disabling shadows still samples the shadow map");

        auto scene = texture(GL_RGB16F);
        const float white[] = {1.f, 1.f, 1.f};
        glClearTexImage(scene->getId(), 0, GL_RGB, GL_FLOAT, white);
        const auto projection = glm::perspective(glm::radians(60.f), 1.f, 0.1f, 100.f);
        const auto clip = projection * glm::vec4(0.f, 0.f, -10.f, 1.f);
        depthValue = 0.5f * (clip.z / clip.w) + 0.5f;
        glClearTexImage(depth->getId(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);
        scene->setActive(0);
        depth->setActive(1);
        auto *fog = shader("data/shader/post/fog_pass.ini");
        fog->setUniform("scene_texture", 0);
        fog->setUniform("depth_texture", 1);
        fog->setUniform("inverse_projection", glm::inverse(projection));
        fog->setUniform("fog_type", 1);
        fog->setUniform("fog_begin", 0.f);
        fog->setUniform("fog_end", 20.f);
        draw(*quad);
        const auto fogged = read(GL_COLOR_ATTACHMENT0);
        if (glm::distance(glm::vec3(fogged[0], fogged[1], fogged[2]),
                          glm::vec3(0.85f, 0.8f, 0.75f)) > 0.003f)
            throw std::runtime_error("Fog ignores camera projection or configured distances");
        CFrameBuffer::setDefaultActive();
        // Exercise the actual system resize path with two viewport shapes. This
        // uses the hidden test context, without resizing or capturing the desktop.
        TestViewport viewport(window);
        std::unique_ptr<IResourceManager> viewportResources(createResourceManager());
        CGraphicsSystem system;
        if (!system.init(*viewportResources))
            throw std::runtime_error("Viewport fixture failed to initialize");
        CFirstPersonCamera camera({0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 1.f, 0.f}, 60.f, 1.f,
                                  0.1f, 100.f);
        camera.getFeatureInfoForWrite().dofActive = false;
        camera.getFeatureInfoForWrite().godRayActive = false;
        camera.getFeatureInfoForWrite().lenseFlareActive = false;
        system.setActiveCamera(&camera);
        system.setActiveScene(system.createScene());
        system.draw(viewport);
        if (std::abs(camera.getProjection()[1][1] / camera.getProjection()[0][0] - 2.f) > 0.0001f)
            throw std::runtime_error("Camera aspect ratio did not follow the viewport");
        viewport.width = 35;
        viewport.height = 70;
        system.draw(viewport);
        if (std::abs(camera.getProjection()[1][1] / camera.getProjection()[0][0] - 0.5f) > 0.0001f)
            throw std::runtime_error("Camera projection was not updated after resize");
        system.setActiveCamera(nullptr);
        system.toggleDebugOverlay();
        system.draw(viewport);
        std::puts("Shader checks passed: normal transforms, degenerate UVs, shadow switch, and fog "
                  "distances.");
        return true;
    }
    catch (const std::exception &error)
    {
        std::fprintf(stderr, "Shader regression: %s\n", error.what());
        return false;
    }
}
