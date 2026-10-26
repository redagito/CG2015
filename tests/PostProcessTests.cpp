#include "graphics/camera/CCamera.h"
#include "graphics/renderer/CDeferredRenderer.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/scene/CScene.h"
#include "graphics/scene/CSceneQuery.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include "support/PostProcessTestRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace
{

std::vector<float> readTarget(const CGlfwWindow &window)
{
    // Read numeric values from this test's own offscreen FBO, never the desktop.
    GLint target = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &target);
    if (target == 0)
        throw std::runtime_error("Expected an isolated offscreen framebuffer");
    std::vector<float> values(window.getWidth() * window.getHeight() * 3);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, window.getWidth(), window.getHeight(), GL_RGB, GL_FLOAT, values.data());
    for (float value : values)
        if (!std::isfinite(value))
            throw std::runtime_error("Glare produced a non-finite pixel");
    return values;
}
} // namespace

bool runPostProcessTests(const CGlfwWindow &window)
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        CGraphicsResourceManager graphics;
        resources->addResourceListener(&graphics);
        PostProcessTestRenderer renderer;
        if (!renderer.init(*resources))
            throw std::runtime_error("Renderer initialization failed");

        CCamera camera;
        auto &features = camera.getFeatureInfoForWrite();
        features.fxaaActive = false;
        features.dofActive = false;
        features.godRayActive = false;
        features.lenseFlareActive = false;
        const auto width = window.getWidth(), height = window.getHeight();
        auto input = std::make_shared<CTexture>();
        if (!input->init(width, height, GL_RGB16F))
            throw std::runtime_error("HDR target allocation failed");
        std::vector<float> pixels(width * height * 3, 0.f);
        // A synthetic HDR sun, off center, should produce ghosts away from the source.
        for (unsigned y = height / 3; y < height / 3 + 7; ++y)
            for (unsigned x = width / 4; x < width / 4 + 7; ++x)
                for (unsigned channel = 0; channel < 3; ++channel)
                    pixels[3 * (y * width + x) + channel] = 32.f;
        glTextureSubImage2D(input->getId(), 0, 0, 0, width, height, GL_RGB, GL_FLOAT,
                            pixels.data());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        renderer.postProcessPass(camera, window, graphics, input);
        const auto withoutFlare = readTarget(window);
        features.lenseFlareActive = true;
        renderer.postProcessPass(camera, window, graphics, input);
        const auto withFlare = readTarget(window);
        double addedLight = 0.;
        for (size_t i = 0; i < withFlare.size(); ++i)
            addedLight += std::max(0.f, withFlare[i] - withoutFlare[i]);
        if (addedLight < 1.)
            throw std::runtime_error("The HDR sun produced no lens flare");
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            renderer.postProcessPass(camera, window, graphics, input);
            const auto repeated = readTarget(window);
            for (size_t i = 0; i < repeated.size(); ++i)
                if (std::abs(repeated[i] - withFlare[i]) > 0.0001f)
                    throw std::runtime_error("Identical glare frames are unstable");
        }
        std::fill(pixels.begin(), pixels.end(), 0.f);
        glTextureSubImage2D(input->getId(), 0, 0, 0, width, height, GL_RGB, GL_FLOAT,
                            pixels.data());
        renderer.postProcessPass(camera, window, graphics, input);
        const auto dark = readTarget(window);
        for (float value : dark)
            if (std::abs(value) > 0.0001f)
                throw std::runtime_error("A black input produced phantom glare");

        // Odd dimensions put one fragment exactly at the optical center. Exercise
        // the flare-generation shader directly to catch normalize(vec2(0)).
        auto output = std::make_shared<CTexture>();
        if (!output->init(width, height, GL_RGB16F))
            throw std::runtime_error("Flare target allocation failed");
        CFrameBuffer target;
        target.attach(output, GL_COLOR_ATTACHMENT0);
        target.setActive(GL_FRAMEBUFFER);
        glViewport(0, 0, width, height);
        renderer.lensFlarePass2(window, graphics, input);
        readTarget(window);
        // Exercise the combined optional effects against a defined depth buffer.
        camera.setView(glm::mat4(1.f));
        camera.setProjection(glm::perspective(glm::radians(60.f), 1.f, 0.1f, 100.f));
        camera.setPosition(glm::vec3(0.f));
        CScene empty(&graphics);
        CSceneQuery query;
        renderer.geometryPass(empty, camera, window, graphics, query);
        features.fxaaActive = true;
        features.dofActive = true;
        features.godRayActive = true;
        features.toonActive = true;
        features.fogType = FogType::Exp;
        renderer.postProcessPass(camera, window, graphics, input);
        readTarget(window);
        std::printf("Post-processing: stable finite frames, black input, optical center, HDR flare "
                    "(added light %.3f).\n",
                    addedLight);
        return true;
    }
    catch (const std::exception &error)
    {
        std::fprintf(stderr, "Post-processing regression: %s\n", error.what());
        return false;
    }
}
