#include "animation/CAnimationWorld.h"
#include "audio/GameAudio.h"
#include "audio/IAudioSystem.h"
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
#include "debug/Logging.h"
#include "engine/CFrameRateGuard.h"
#include "game/CGameWorld.h"
#include "game/combat/CPlayerCombatUpgrades.h"
#include "game/combat/CombatStatus.h"
#include "game/combat/CombatTuning.h"
#include "game/control/CAudioEmitterController.h"
#include "game/control/CHealthController.h"
#include "game/control/CWeaponController.h"
#include "game/effects/CCombatEffects.h"
#include "game/encounter/CEnemySpawner.h"
#include "game/flight/FlightHud.h"
#include "game/flight/FlightState.h"
#include "game/flight/FlightTuning.h"
#include "game/progression/CPlayerProgression.h"
#include "graphics/camera/CCamera.h"
#include "graphics/system/CGraphicsSystem.h"
#include "graphics/window/CGlfwWindow.h"
#include "io/CSceneLoader.h"
#include "resource/Resource.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <thread>
int main(int argc, char **argv)
{
    try
    {
        logging::initialize();
        const unsigned frames = argc > 1 ? std::max(1, std::atoi(argv[1])) : 1800;
        const unsigned width = argc > 2 ? unsigned(std::max(64, std::atoi(argv[2]))) : 1280;
        const unsigned height = argc > 3 ? unsigned(std::max(64, std::atoi(argv[3]))) : 720;
        CGlfwWindow window;
        if (!window.init(width, height, "CG2015 isolated swarm benchmark", false))
            return 1;
        glfwSwapInterval(0);
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        const auto await = [&](AsyncLoadId request)
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
            for (;;)
            {
                resources->pumpAsyncLoads(4);
                const auto status = resources->getLoadStatus(request);
                if (status.phase == LoadPhase::Ready)
                    break;
                if (status.phase == LoadPhase::Failed)
                    throw std::runtime_error(status.error);
                if (std::chrono::steady_clock::now() > deadline)
                    throw std::runtime_error("Swarm loading timed out");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        };
        await(resources->loadManifestAsync("data/bootstrap-assets.json"));
        CGraphicsSystem graphics;
        if (!graphics.init(*resources))
            return 2;
        auto audioSystem = createAudioSystem(AudioOutput::Offline);
        resources->addResourceListener(audioSystem.get());
        GameAudio audio(*audioSystem);
        await(resources->loadManifestAsync("data/game/defenders_of_cthedra/assets.json"));
        resources->setCacheOnly(true);
        audio.configure(*resources);
        audio.enterState("game");
        auto *scene = graphics.createScene();
        CAnimationWorld animation;
        CSceneLoader loader(*resources);
        if (!loader.load("data/world/game_1.json", *scene, animation))
            return 3;
        CCollisionSystem collisions;
        const auto playerGroup = collisions.getNewGroupId(),
                   enemyGroup = collisions.getNewGroupId();
        CGameWorld world;
        const bool profile = argc > 4 && std::string_view(argv[4]) == "profile";
        world.profileControllers(profile);
        std::unordered_map<const std::type_info *, double> controllerTimes;
        auto *player = new CGameObject();
        player->setPosition({0, 100, -130});
        player->setCollidable(collisions.add(CAABBox::create({-2, -2, -2, 2, 2, 2}), playerGroup));
        const auto playerId = world.addObject(player);
        CCombatEffects effects(world, *scene, CombatEffectsTuning{});
        effects.bindPlayer(playerId);
        const auto arena = ArenaTuning::load(*resources);
        const auto combat = CombatTuning::load(*resources);
        auto roster = EnemyRoster::load(*resources, arena, combat.weapon);
        std::erase_if(roster.spawns, [](const auto &spawn) { return spawn.wave != 9; });
        for (auto &type : roster.types)
            type.health = 1e9f; // Keep the full population alive while both sides fire.
        for (auto &spawn : roster.spawns)
            spawn.time = 0;
        const auto mesh = resources->loadMesh("data/mesh/bullet.obj"),
                   material = resources->loadMaterial("data/material/white_glowing.json");
        CPlayerProgression progression;
        for (int i = 0; i < 500; ++i)
            progression.awardKill();
        for (const auto &skill : CPlayerProgression::skills())
            for (unsigned rank = 0; rank < 5; ++rank)
                progression.buy(skill.id);
        auto playerTuning = combat.weapon;
        FlightTuning flight;
        progression.apply(flight, playerTuning);
        auto playerHealth =
            std::make_shared<CHealthController>(1e9f, &audio, &effects, progression.defenses());
        player->addController(playerHealth);
        CPlayerCombatUpgrades upgrades(world, *playerHealth, &effects,
                                       progression.combatUpgrades());
        auto gun = std::make_shared<CWeaponController>(nullptr, &world, scene, mesh, material,
                                                       resources.get(), &collisions, playerGroup,
                                                       &audio, playerTuning, &effects, &upgrades);
        gun->setTrigger(true);
        player->addController(gun);
        player->addController(std::make_shared<CAudioEmitterController>(&audio, "player_engine"));
        scene->createObject(resources->loadMesh("data/mesh/piramyde.obj"),
                            resources->loadMaterial("data/material/sand.json"),
                            {0, .517224f * arena.templeScale, 0}, glm::quat(1, 0, 0, 0),
                            glm::vec3(arena.templeScale));
        scene->createObject(resources->loadMesh("data/mesh/mothership.obj"),
                            resources->loadMaterial("data/material/mothership.json"),
                            arena.mothership.destination, glm::quat(1, 0, 0, 0),
                            glm::vec3(arena.mothership.scale));
        CEnemySpawner spawner(
            roster, arena, world, *scene, *resources, collisions, playerId, enemyGroup, mesh,
            material, &audio, [] {}, &effects);
        const auto beforeSpawn = std::chrono::steady_clock::now();
        spawner.update(0);
        const auto spawned = world.targets().size();
        std::printf("SWARM spawn: %zu enemies, %zu pending, %.2f ms\n", spawned, spawner.pending(),
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
                                                              beforeSpawn)
                        .count());
        if (spawned != 5120 || spawner.pending() != 0)
            throw std::runtime_error("Spawner could not hold all 5120 enemies simultaneously");
        CCamera camera;
        auto &features = camera.getFeatureInfoForWrite();
        features.lenseFlareActive = features.godRayActive = features.dofActive = false;
        features.frustumCullingActive =
            false; // Deliberately draw every fighter in this stress view.
        camera.setPosition({0, 500, -1300});
        camera.setView(glm::lookAt(camera.getPosition(), glm::vec3(0, 100, 0), glm::vec3(0, 1, 0)));
        camera.setProjection(glm::perspective(
            glm::radians(75.f), float(window.getWidth()) / window.getHeight(), .5f, 12000.f));
        graphics.setActiveScene(scene);
        graphics.setActiveCamera(&camera);
        std::vector<double> frameTimes, simulationTimes, renderTimes;
        std::vector<float> samples(12000 * 2);
        CFrameRateGuard guard;
        double minimumWindowFps = 1e9;
        bool performancePassed = true;
        double collisionMs = 0, worldMs = 0, effectsMs = 0, audioMs = 0;
        double previousFrameSeconds = 1.0 / 60;
        std::array<double, 3> worldPhases{};
        double traces = 0, receivers = 0, indexBuilds = 0;
        double simulatedSeconds = 0;
        for (unsigned frame = 0; frame < frames + 3 || (frames >= 900 && simulatedSeconds < 65);
             ++frame)
        {
            const float step =
                profile ? 1.f / 30 : float(std::clamp(previousFrameSeconds, .001, .25));
            const auto begin = std::chrono::steady_clock::now();
            collisions.update();
            collisions.beginStep();
            const auto collided = std::chrono::steady_clock::now();
            world.update(step);
            const auto moved = std::chrono::steady_clock::now();
            upgrades.update(step);
            effects.update(step);
            const auto effected = std::chrono::steady_clock::now();
            audio.update(step);
            audioSystem->render(samples.data(), std::min(12000u, unsigned(48000 * step)));
            const auto simulated = std::chrono::steady_clock::now();
            CombatStatus status;
            status.health = status.maxHealth = 300;
            status.wave = status.waveCount = 10;
            status.waveRemaining = 5120;
            graphics.setUiOverlay(FlightHud::build(camera, player->getPosition(), FlightState{},
                                                   {window.getWidth(), window.getHeight()}, nullptr,
                                                   nullptr, &arena, &status, &effects.feedback()));
            graphics.draw(window);
            window.swapBuffer();
            glFinish();
            glfwPollEvents();
            const auto end = std::chrono::steady_clock::now();
            previousFrameSeconds = std::chrono::duration<double>(end - begin).count();
            if (frame >= 3)
            {
                simulatedSeconds += step;
                const auto ms = [](auto end, auto start)
                { return std::chrono::duration<double, std::milli>(end - start).count(); };
                const bool healthy = guard.record(ms(end, begin) / 1000, true);
                if (guard.lastFps() > 0)
                    minimumWindowFps = std::min(minimumWindowFps, guard.lastFps());
                if (!healthy)
                {
                    performancePassed = false;
                    guard.reset();
                }
                if (profile)
                    for (const auto &[type, elapsed] : world.controllerMilliseconds())
                        controllerTimes[type] += elapsed;
                for (unsigned i = 0; i < 3; ++i)
                    worldPhases[i] += world.stepMilliseconds()[i];
                traces += collisions.queryStats().traces;
                receivers += collisions.queryStats().receivers;
                indexBuilds += collisions.queryStats().indexBuilds;
                collisionMs += ms(collided, begin);
                worldMs += ms(moved, collided);
                effectsMs += ms(effected, moved);
                audioMs += ms(simulated, effected);
                if (!healthy && !profile)
                    break;
                frameTimes.push_back(
                    std::chrono::duration<double, std::milli>(end - begin).count());
                simulationTimes.push_back(
                    std::chrono::duration<double, std::milli>(simulated - begin).count());
                renderTimes.push_back(
                    std::chrono::duration<double, std::milli>(end - simulated).count());
            }
        }
        if (world.targets().size() != 5120)
            throw std::runtime_error("Swarm population dropped during benchmark");
        for (const auto &target : world.targets())
            if (!std::isfinite(target.position.x) || !std::isfinite(target.position.y) ||
                !std::isfinite(target.position.z))
                throw std::runtime_error("Swarm simulation produced a non-finite position");
        const auto measuredFrames = frameTimes.size();
        const auto mean = [](const auto &values)
        { return std::accumulate(values.begin(), values.end(), 0.0) / values.size(); };
        auto ordered = frameTimes;
        std::sort(ordered.begin(), ordered.end());
        std::printf("SWARM %ux%u / %u frames / %zu enemies / %zu objects: average %.2f ms (%.2f "
                    "FPS), worst %.2f ms (%.2f FPS), p95 %.2f ms, CPU simulation+audio %.2f ms, "
                    "render+GPU %.2f ms\n",
                    window.getWidth(), window.getHeight(), unsigned(measuredFrames),
                    world.targets().size(), world.objectCount(), mean(frameTimes),
                    1000 / mean(frameTimes), ordered.back(), 1000 / ordered.back(),
                    ordered[(ordered.size() - 1) * 95 / 100], mean(simulationTimes),
                    mean(renderTimes));
        std::printf("SWARM minimum one-second average %.2f FPS / 30 FPS requirement %s; CPU "
                    "breakdown collision %.2f, world %.2f, effects %.2f, audio %.2f ms\n",
                    minimumWindowFps == 1e9 ? 0 : minimumWindowFps,
                    performancePassed ? "PASS" : "FAIL", collisionMs / measuredFrames,
                    worldMs / measuredFrames, effectsMs / measuredFrames, audioMs / measuredFrames);
        std::printf("SWARM world snapshot %.2f / actors %.2f / projectiles %.2f ms; traces %.0f, "
                    "receiver candidates %.0f, index builds %.2f per frame\n",
                    worldPhases[0] / measuredFrames, worldPhases[1] / measuredFrames,
                    worldPhases[2] / measuredFrames, traces / measuredFrames,
                    receivers / measuredFrames, indexBuilds / measuredFrames);
        std::printf("SWARM measured simulation %.2f seconds\n", simulatedSeconds);
        for (const auto &[type, elapsed] : controllerTimes)
            std::printf("SWARM controller %s: %.3f ms\n", type->name(), elapsed / measuredFrames);
        return profile || (performancePassed && minimumWindowFps < 1e9) ? 0 : 4;
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "Swarm benchmark: %s\n", e.what());
        return 1;
    }
}
