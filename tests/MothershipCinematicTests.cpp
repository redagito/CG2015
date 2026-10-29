#include "audio/GameAudio.h"
#include "game/CGameWorld.h"
#include "game/control/CMothershipController.h"
#include "game/control/CSimpleWaypointController.h"
#include "game/effects/CMothershipCinematic.h"
#include "graphics/scene/CScene.h"
#include "support/RecordingAudioSystem.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
int main()
{
    try
    {
        CGameWorld world;
        auto *ufo = new CGameObject();
        ufo->setPosition({0, 68, 700});
        ufo->addController(std::make_shared<CMothershipController>(
            glm::vec3(0, 68, 0), MothershipTuning{30, 5}, [&] { world.freezeSimulation(); }));
        world.addObject(ufo);
        auto *enemy = new CGameObject();
        enemy->setPosition({10, 20, 30});
        enemy->addController(
            std::make_shared<CSimpleWaypointController>(glm::vec3(100, 20, 30), 20, nullptr));
        world.addObject(enemy);
        auto *shot = new CGameObject();
        shot->setProjectile(true);
        shot->setVelocity({0, 0, 100});
        world.addObject(shot);
        world.update(35);
        require(world.simulationFrozen() && enemy->getPosition() == glm::vec3(10, 20, 30),
                "Deadline freeze allowed later actors to update");
        require(glm::length(shot->getVelocity()) == 0, "In-flight shot retained motion");
        world.update(10);
        require(enemy->getPosition() == glm::vec3(10, 20, 30),
                "Enemy kept moving during cinematic");
        CScene scene(nullptr);
        CGameObject temple;
        RecordingAudioSystem recorder;
        GameAudio audio(recorder);
        {
            CMothershipCinematic cinematic(scene, temple, {0, 53, 0}, {0, 16, 0}, &audio);
            cinematic.update(0, {1280, 720});
            audio.update(0);
            require(recorder.lastCue == "mothership_charge" && !cinematic.beamActive() &&
                        !cinematic.finished(),
                    "Cinematic did not start with the charge-up");
            cinematic.update(1.25f, {1280, 720});
            audio.update(.016f);
            require(cinematic.beamActive() && recorder.lastCue == "mothership_beam",
                    "Beam/sound was not fired");
            require(std::count_if(scene.particles().begin(), scene.particles().end(),
                                  [](const auto &p) { return p.kind == ParticleKind::Beam; }) == 2,
                    "Beam core and halo are missing");
            for (const auto &particle : scene.particles())
                if (particle.kind == ParticleKind::Beam)
                    require(glm::distance(particle.position + particle.velocity,
                                          glm::vec3(0, 16, 0)) < .001f,
                            "Beam misses temple impact point");
            for (glm::vec2 viewport :
                 {glm::vec2(1280, 720), glm::vec2(360, 800), glm::vec2(2560, 720)})
            {
                cinematic.update(0, viewport);
                for (glm::vec3 point : {glm::vec3(0, 53, 0), glm::vec3(0, 16, 0),
                                        glm::vec3(59, 68, 0), glm::vec3(-59, 68, 0)})
                {
                    const auto clip = cinematic.camera().getProjection() *
                                      cinematic.camera().getView() * glm::vec4(point, 1);
                    require(clip.w > 0 && std::abs(clip.x / clip.w) < .95f &&
                                std::abs(clip.y / clip.w) < .84f,
                            "Cinematic camera cropped the UFO or temple");
                }
            }
            cinematic.update(2.5f, {1280, 720});
            audio.update(.016f);
            require(!cinematic.beamActive() && !cinematic.finished() &&
                        recorder.lastCue == "explosion",
                    "Beam did not end with destruction before defeat");
            cinematic.update(2.25f, {1280, 720});
            require(cinematic.finished(), "Cinematic did not finish after six seconds");
            const auto overlay = cinematic.overlay({1280, 720});
            require(!overlay.rectangles.empty() && overlay.rectangles.back().color.a == 1,
                    "Cinematic did not fade to black");
        }
        require(scene.particles().empty(), "Cinematic particles leaked after exit");
        // The enlarged disc, temple base, apex and laser must fit at every supported aspect.
        temple.setScale(glm::vec3(24));
        const glm::vec3 emitter(0, 650.0236f, 0), impact(0, 122.9552f, 0);
        {
            CMothershipCinematic cinematic(scene, temple, emitter, impact, nullptr);
            for (glm::vec2 viewport :
                 {glm::vec2(1280, 720), glm::vec2(360, 800), glm::vec2(2560, 720)})
            {
                cinematic.update(1.25f, viewport);
                for (glm::vec3 point :
                     {emitter, impact, glm::vec3(0), glm::vec3(392, 750, 0),
                      glm::vec3(-392, 750, 0), glm::vec3(392, 909, 392), glm::vec3(-392, 909, -392),
                      glm::vec3(392, 909, -392), glm::vec3(-392, 909, 392), glm::vec3(0, 909, 0),
                      glm::vec3(167, 0, 167)})
                {
                    const auto clip = cinematic.camera().getProjection() *
                                      cinematic.camera().getView() * glm::vec4(point, 1);
                    require(clip.w > 0 && std::abs(clip.x / clip.w) < .95f &&
                                std::abs(clip.y / clip.w) < .84f && std::abs(clip.z / clip.w) < 1,
                            "Scaled cinematic cropped the mothership or temple");
                }
            }
        }

        std::cout << "Mothership cinematic: simulation freeze, charge/beam/destruction timing, "
                     "endpoint alignment, camera framing and cleanup passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
