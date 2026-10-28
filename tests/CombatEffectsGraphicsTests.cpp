#include "graphics/camera/CCamera.h"
#include "graphics/renderer/pass/deferred/ParticlePass.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/resource/CTexture.h"
#include "graphics/scene/CScene.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
bool runCombatEffectsGraphicsTests(const CGlfwWindow &window)
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        CGraphicsResourceManager gpu;
        resources->addResourceListener(&gpu);
        ColorTarget target;
        target.color = std::make_shared<CTexture>();
        require(target.color->init(window.getWidth(), window.getHeight(), GL_RGBA16F),
                "Effects target allocation failed");
        auto depth = std::make_shared<CTexture>();
        require(depth->init(window.getWidth(), window.getHeight(), GL_DEPTH_COMPONENT24),
                "Effects depth allocation failed");
        target.framebuffer.attach(target.color, GL_COLOR_ATTACHMENT0);
        target.framebuffer.attach(depth, GL_DEPTH_ATTACHMENT);
        ParticlePass pass(target);
        require(pass.init(*resources), "Particle shader failed to load");
        CCamera camera;
        camera.setPosition({0, 0, 0});
        camera.setView(glm::mat4(1));
        camera.setProjection(glm::ortho(-1.f, 1.f, -1.f, 1.f, .1f, 10.f));
        CScene scene(&gpu);
        const auto read = [&](unsigned x, unsigned y)
        {
            GLint bound = 0;
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &bound);
            require(bound != 0, "Expected the test's isolated framebuffer");
            std::array<float, 4> pixel{};
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, pixel.data());
            for (auto value : pixel)
                require(std::isfinite(value), "Particle output became non-finite");
            return pixel;
        };
        const auto clear = [&](float z)
        {
            target.framebuffer.setActive(GL_FRAMEBUFFER);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_SCISSOR_TEST);
            glDepthMask(GL_TRUE);
            glClearColor(0, 0, 0, 0);
            glClearDepth(z);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        };
        scene.setParticles(
            {{{0, 0, -2}, {0, 0, 0}, {2, .5f, .1f, 1}, {.6f, .6f}, 0, ParticleKind::Glow}});
        clear(1);
        pass.render(scene, camera, window, gpu);
        require(read(window.getWidth() / 2, window.getHeight() / 2)[0] > 1,
                "Impact glow is not visible");
        GLboolean writes = false;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &writes);
        require(writes, "Particle pass leaked its depth-write state");
        float actualDepth = 0;
        glReadPixels(window.getWidth() / 2, window.getHeight() / 2, 1, 1, GL_DEPTH_COMPONENT,
                     GL_FLOAT, &actualDepth);
        require(actualDepth == 1, "Particles wrote depth and occluded the scene");
        const auto wall = camera.getProjection() * glm::vec4(0, 0, -1, 1);
        clear(wall.z / wall.w * .5f + .5f);
        pass.render(scene, camera, window, gpu);
        require(read(window.getWidth() / 2, window.getHeight() / 2)[0] < .0001f,
                "Particle glow leaked through an opaque wall");
        scene.setParticles(
            {{{0, 0, -2}, {0, 0, 0}, {.4f, 6, 12, 1}, {.6f, .6f}, 0, ParticleKind::Shield}});
        clear(1);
        pass.render(scene, camera, window, gpu);
        const float center = read(window.getWidth() / 2, window.getHeight() / 2)[2];
        float rim = 0;
        for (unsigned x = window.getWidth() / 2 + 1; x < window.getWidth(); ++x)
            rim = std::max(rim, read(x, window.getHeight() / 2)[2]);

        require(rim > center * 3 && rim > .2f, "Shield pulse has no visible rim");
        scene.setParticles(
            {{{-.5f, 0, -2}, {1, 0, 0}, {3, 6, 4, 1}, {.05f, 1}, 0, ParticleKind::Beam}});
        clear(1);
        pass.render(scene, camera, window, gpu);
        require(read(window.getWidth() / 2, window.getHeight() / 2)[0] > 1,
                "World-space beam core is not visible");
        require(read(window.getWidth() / 3, window.getHeight() / 2)[1] > .5f &&
                    read(window.getWidth() * 2 / 3, window.getHeight() / 2)[1] > .5f,
                "Beam ribbon does not join its world endpoints");
        clear(wall.z / wall.w * .5f + .5f);
        pass.render(scene, camera, window, gpu);
        require(read(window.getWidth() / 2, window.getHeight() / 2)[0] < .0001f,
                "Cinematic beam ignores opaque occlusion");
        std::puts(
            "Effects GPU: visible glow, opaque occlusion, isolated state and shield rim passed.");
        return true;
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "Effects GPU regression: %s\n", e.what());
        return false;
    }
}
