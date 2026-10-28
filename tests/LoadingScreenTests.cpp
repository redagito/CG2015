#include "audio/GameAudio.h"
#include "collision/CCollidable.h"
#include "engine/CEngine.h"
#include "game/CGameSystem.h"
#include "game/state/CGamePlayState.h"
#include "game/state/CLoadState.h"
#include "game/state/CLoseState.h"
#include "game/state/CTitleState.h"
#include "game/state/CWinState.h"
#include "graphics/system/CGraphicsSystem.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include "support/TestInput.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

namespace
{

void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
bool runLoadingScreenTests(CGlfwWindow &window)
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        const auto boot = resources->loadManifestAsync("data/bootstrap-assets.json");
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        for (;;)
        {
            resources->pumpAsyncLoads(2.0);
            const auto status = resources->getLoadStatus(boot);
            require(status.phase != LoadPhase::Failed, status.error.c_str());
            if (status.phase == LoadPhase::Ready)
                break;
            require(std::chrono::steady_clock::now() < deadline, "Bootstrap timed out");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        resources->setCacheOnly(true);
        auto audio = createAudioSystem(AudioOutput::Offline);
        resources->addResourceListener(audio.get());
        GameAudio gameAudio(*audio);
        CGraphicsSystem graphics;
        require(graphics.init(*resources), "Cached renderer bootstrap failed");
        TestInput input;
        CGameSystem game;
        auto *loading =
            new CLoadState("data/world/load_1.json", "data/game/defenders_of_cthedra/assets.json");
        game.addState("load", loading);
        game.addState("title", new CTitleState("data/world/intro_1.json"));
        auto *gameplay = new CGamePlayState();
        gameplay->setAudio(&gameAudio);
        game.addState("game", gameplay);
        game.addState("lose", new CLoseState("data/world/lose.json"));
        game.addState("win", new CWinState("data/world/win.json"));
        require(game.init("load", &graphics, &input, resources.get(), &gameAudio),
                "Loading state initialization failed");
        input.enter = true; // A press during loading must not skip the ready prompt.
        unsigned loadingFrames = 0;
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!loading->readyToContinue())
        {
            require(game.update(0.016f), "Loading state failed");
            graphics.draw(window);
            ++loadingFrames;
            require(game.activeStateName() == "load",
                    "Enter skipped the loading screen before readiness");
            require(std::chrono::steady_clock::now() < deadline,
                    "Loading screen did not finish loading");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(loadingFrames > 1 && game.activeStateName() == "load",
                "Loading screen transitioned automatically");
        for (int i = 0; i < 3; ++i)
            require(game.update(0.016f) && game.activeStateName() == "load",
                    "Held Enter dismissed the ready prompt");
        input.enter = false;
        require(game.update(0.016f) && game.activeStateName() == "load",
                "Releasing Enter dismissed the ready prompt");
        input.enter = true;
        require(game.update(0.016f) && game.activeStateName() == "title",
                "Fresh Enter did not continue to title");
        std::vector<float> samples(4096 * 2);
        require(audio->render(samples.data(), 4096) && audio->activeVoices() > 0,
                "Title audio was not ready after loading");
        require(game.update(0.016f) && game.activeStateName() == "title",
                "The same Enter press skipped the title");
        input.enter = false;
        require(game.update(0.016f), "Title Enter release failed");
        input.enter = true;
        require(game.update(0.016f), "Gameplay initialization failed after loading");
        require(game.activeStateName() == "game", "Title Enter did not enter gameplay");
        require(input.flightMode, "Gameplay did not acquire flight input");
        input.viewport = glm::vec2(window.getWidth(), window.getHeight());
        input.pointer = glm::vec2(.1f, -.03f);
        input.fire = true;
        for (int i = 0; i < 3; ++i)
        {
            require(game.update(0.016f), "Gameplay update failed");
            graphics.draw(window);
            audio->render(samples.data(), 4096);
        }
        for (int frame = 0; frame < 240; ++frame)
        {
            require(game.update(.016f) && game.activeStateName() == "game",
                    "Arena play failed during enemy deployment");
            audio->render(samples.data(), 1024);
        }
        graphics.draw(window);
        require(gameplay->getGameWorld().targets().size() >= 2, "Pursuit enemies did not deploy");
        require(audio->activeVoices() >= 3, "Gameplay music and attached engines did not start");
        require(std::any_of(samples.begin(), samples.end(),
                            [](float v) { return std::abs(v) > 0.00001f; }),
                "Gameplay audio mix was silent");
        const auto *player = gameplay->getGameWorld().getObject(1);
        require(player && player->hasCollidable(), "Player collision/health fixture is missing");
        player->getCollidable()->receiveDamage(10000);
        require(game.update(.016f) && game.activeStateName() == "game" && !input.flightMode,
                "Player destruction did not start visible feedback and release flight input");
        for (int frame = 0; frame < 120 && game.activeStateName() == "game"; ++frame)
            require(game.update(.016f), "Defeat feedback transition failed");
        require(game.activeStateName() == "lose",
                "Player destruction did not finish the defeat transition");
        graphics.draw(window);
        // Exercise the actual deadline path with cached resources and live rendering.
        // A single large step reaches the deadline without an unrelated player death fixture.
        {
            CGamePlayState siege;
            siege.setAudio(&gameAudio);
            require(siege.init(&graphics, &input, resources.get()), "Siege initialization failed");
            siege.onEnter();
            input.fire = true;
            require(siege.update(35) && siege.getGameWorld().simulationFrozen() &&
                        !input.flightMode,
                    "Mothership deadline skipped cinematic or failed to freeze combat");
            const auto frozenTargets = siege.getGameWorld().targets();
            const auto frozenPlayer = siege.getGameWorld().getObject(1)->getPosition();
            for (int frame = 0; frame < 23; ++frame)
            {
                require(siege.update(.25f), "Cinematic entered defeat before its final fade");
                gameAudio.update(.25f);
                if (frame % 4 == 0)
                    graphics.draw(window);
                const auto targets = siege.getGameWorld().targets();
                require(targets.size() == frozenTargets.size(),
                        "Enemies spawned or died during cinematic");
                for (std::size_t j = 0; j < targets.size(); ++j)
                    require(targets[j].position == frozenTargets[j].position,
                            "Enemy moved during cinematic");
                require(siege.getGameWorld().getObject(1)->getPosition() == frozenPlayer,
                        "Player moved during cinematic");
            }
            require(!siege.update(.25f) && siege.getNextState() == "lose",
                    "Completed mothership cinematic did not route to defeat");
            siege.onExit();
        }
        std::printf("Loading screen rendered %u frames, then title and gameplay initialized from "
                    "cache only.\n",
                    loadingFrames);
        return true;
    }
    catch (const std::exception &error)
    {
        std::fprintf(stderr, "Loading integration: %s\n", error.what());
        return false;
    }
}
