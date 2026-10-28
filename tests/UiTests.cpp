#include "game/combat/ProjectileBallistics.h"
#include "game/combat/TargetingSolution.h"
#include "game/flight/CChaseCamera.h"
#include "game/flight/FlightHud.h"
#include "game/progression/CPlayerProgression.h"
#include "game/progression/CSkillTreeScreen.h"
#include "game/state/CLoseState.h"
#include "game/state/CTitleState.h"
#include "game/state/CWinState.h"
#include "graphics/ICamera.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/renderer/CFrameBuffer.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/resource/CTexture.h"
#include "graphics/scene/CScene.h"
#include "graphics/ui/CUiRenderer.h"
#include "resource/Resource.h"
#include "support/TestGraphics.h"
#include "support/TestInput.h"
#include <GLFW/glfw3.h>
#include <algorithm>
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

bool runUiTests()
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        CGraphicsResourceManager gpu;
        resources->addResourceListener(&gpu);
        CUiRenderer ui;
        require(ui.init(*resources), "Bundled UI initialization failed");
        TestGraphics graphics(&gpu);
        TestInput input;
        CTitleState title("data/world/intro_1.json");
        require(title.init(&graphics, &input, resources.get()), "Title initialization failed");
        input.enter = true;
        title.onEnter();
        require(title.update(0.016f), "A held Enter key skipped the title");
        require(graphics.overlay.texts.size() == 4 &&
                    graphics.overlay.texts[0].text == "DEFENDERS" &&
                    graphics.overlay.texts[1].text == "of CTHEDRA",
                "The title UI text is missing");

        for (const glm::uvec2 size :
             {glm::uvec2(320, 180), glm::uvec2(1280, 720), glm::uvec2(480, 900)})
        {
            auto color = std::make_shared<CTexture>();
            require(color->init(size.x, size.y, GL_RGBA8), "UI target allocation failed");
            CFrameBuffer target;
            target.attach(color, GL_COLOR_ATTACHMENT0);
            target.setActive(GL_FRAMEBUFFER);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            // A hostile caller state must neither hide the UI nor be lost.
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, 0, 0);
            glDepthMask(GL_TRUE);
            glEnable(GL_FRAMEBUFFER_SRGB);
            glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
            ui.draw(graphics.overlay, size.x, size.y, gpu);
            require(glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_SCISSOR_TEST) &&
                        glIsEnabled(GL_FRAMEBUFFER_SRGB),
                    "UI leaked caller GL state");
            GLint fbo = 0;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
            require(static_cast<GLuint>(fbo) == target.getId(), "UI changed the destination");
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_FRAMEBUFFER_SRGB);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            std::vector<unsigned char> pixels(size.x * size.y * 4);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            unsigned titlePixels = 0, subtitlePixels = 0, promptPixels = 0, centerPixels = 0;
            for (unsigned y = 0; y < size.y; ++y)
                for (unsigned x = 0; x < size.x; ++x)
                {
                    const auto p = 4 * (y * size.x + x);
                    const float top = 1.f - (y + 0.5f) / size.y;
                    const bool bright = pixels[p + 1] > 150 && pixels[p + 2] > 150;
                    if (bright && top < 0.125f)
                        ++titlePixels;
                    if (bright && top >= 0.125f && top < 0.20f)
                        ++subtitlePixels;
                    if (bright && top > 0.83f && top < 0.89f)
                        ++promptPixels;
                    if (top > 0.35f && top < 0.70f && pixels[p + 1] > 0)
                        ++centerPixels;
                }
            require(titlePixels > 15 && subtitlePixels > 8 && promptPixels > 5,
                    "Title UI was clipped or missing after resize");
            require(centerPixels == 0, "Title overlay obscured the ship area");
            CChaseCamera chase(FlightTuning{});
            FlightState flight;
            flight.pointer = glm::vec2(.22f, 0);
            chase.follow(glm::vec3(0), flight, 0, float(size.x) / size.y);
            glClear(GL_COLOR_BUFFER_BIT);
            ui.draw(FlightHud::build(chase, glm::vec3(0), flight, glm::vec2(size)), size.x, size.y,
                    gpu);
            glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            unsigned steeringPixels = 0, headingPixels = 0;
            for (unsigned y = size.y / 4; y < size.y * 3 / 4; ++y)
                for (unsigned x = size.x / 8; x < size.x * 7 / 8; ++x)
                {
                    const auto p = 4 * (y * size.x + x);
                    if (pixels[p] < 160 && pixels[p + 1] > 100 && pixels[p + 2] > 100)
                        ++steeringPixels;
                    if (pixels[p] > 180 && pixels[p + 1] > 180 && pixels[p + 2] > 180)
                        ++headingPixels;
                }
            require(steeringPixels > 3 && headingPixels > 1,
                    "Flight reticles vanished or were clipped after resize");
            TargetingSolution selected;
            selected.target = {1, "TEST", {20, 0, 100}, {-5, 0, 0}};
            WeaponAim gun;
            selected.intercept = ProjectileBallistics::intercept(gun, selected.target.position,
                                                                 selected.target.velocity);
            selected.range = 102;
            glClear(GL_COLOR_BUFFER_BIT);
            ui.draw(FlightHud::build(chase, glm::vec3(0), flight, glm::vec2(size), &gun, &selected),
                    size.x, size.y, gpu);
            glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            unsigned targetPixels = 0, leadPixels = 0;
            for (unsigned y = size.y / 4; y < size.y * 3 / 4; ++y)
                for (unsigned x = size.x / 8; x < size.x * 7 / 8; ++x)
                {
                    const auto p = 4 * (y * size.x + x);
                    if (pixels[p] > 180 && pixels[p + 1] < 160 && pixels[p + 2] < 160)
                        ++targetPixels;
                    if (pixels[p] > 40 && pixels[p] < 180 && pixels[p + 1] > 180 &&
                        pixels[p + 2] < 180)
                        ++leadPixels;
                }
            require(targetPixels > 3 && leadPixels > 1,
                    "Target box or lead indicator vanished after resize");
            CPlayerProgression progression;
            for (int i = 0; i < 8; ++i)
                progression.awardKill();
            CSkillTreeScreen tree(progression);
            glClear(GL_COLOR_BUFFER_BIT);
            ui.draw(tree.build(glm::vec2(size), {}), size.x, size.y, gpu);
            glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            const float scale = std::min(size.x / 1280.f, size.y / 720.f);
            for (unsigned node = 0; node < 6; ++node)
            {
                const glm::vec2 center((.18f + .32f * (node / 2)) * size.x,
                                       (1 - (.37f + .23f * (node % 2))) * size.y);
                unsigned textPixels = 0;
                for (unsigned y = 0; y < size.y; ++y)
                    for (unsigned x = 0; x < size.x; ++x)
                    {
                        if (std::abs(float(x) - center.x) > 130 * scale ||
                            std::abs(float(y) - center.y) > 50 * scale)
                            continue;
                        const auto p = 4 * (y * size.x + x);
                        if (pixels[p + 1] > 140 && pixels[p + 2] > 140)
                            ++textPixels;
                    }
                require(textPixels > 3, "Skill node text is clipped or missing after resize");
            }
            CFrameBuffer::setDefaultActive();
        }
        input.enter = false;
        require(title.update(0.016f), "Title exited on Enter release");
        input.enter = true;
        require(!title.update(0.016f), "Enter did not activate the title action");
        title.onExit();
        require(graphics.overlay.empty(), "Title overlay leaked into gameplay");
        CWinState win("data/world/win.json");
        CLoseState lose("data/world/lose.json");
        for (IGameState *state :
             {static_cast<IGameState *>(&win), static_cast<IGameState *>(&lose)})
        {
            require(state->init(&graphics, &input, resources.get()),
                    "Result screen initialization failed");
            state->onEnter();
            const auto &features = graphics.activeCamera->getFeatureInfo();
            require(!features.lenseFlareActive && !features.godRayActive && !features.dofActive,
                    "Result screen enabled distracting post-processing");
            require(graphics.overlay.texts.size() >= 20 &&
                        graphics.overlay.texts[0].text == (state == &win ? "YOU WON" : "YOU LOST"),
                    "Result screen UI is missing");
            require(state->update(0.016f), "Result screen exited unexpectedly");
            state->onExit();
            require(graphics.overlay.empty(), "Result overlay leaked into the next state");
        }
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        std::puts("Title UI: bundled font, responsive text, isolated GL state and Enter lifecycle "
                  "passed.");
        return true;
    }
    catch (const std::exception &error)
    {
        std::fprintf(stderr, "UI regression: %s\n", error.what());
        return false;
    }
}
