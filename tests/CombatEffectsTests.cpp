#include "audio/GameAudio.h"
#include "collision/CCollidable.h"
#include "game/CGameWorld.h"
#include "game/control/CHealthController.h"
#include "game/effects/CCombatEffects.h"
#include "game/flight/CChaseCamera.h"
#include "game/flight/FlightHud.h"
#include "graphics/scene/CScene.h"
#include "support/RecordingAudioSystem.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main()
{
    try
    {
        CScene scene(nullptr);
        CGameWorld world;
        CombatEffectsTuning tuning;
        tuning.maxParticles = 96;
        CCombatEffects effects(world, scene, tuning);
        CCollidable playerCollider(0, CAABBox());
        auto *player = new CGameObject();
        player->setCollidable(&playerCollider);
        const auto playerId = world.addObject(player);
        effects.bindPlayer(playerId);
        effects.muzzle({0, 0, 0}, {0, 0, 100}, {.25f, .85f, 2});
        effects.trail({0, 0, 1}, {0, 0, 100}, {.25f, .85f, 2});
        effects.impact({0, 0, 10}, {0, 0, -1}, {1, .4f, .1f}, true);
        effects.update(.016f);
        require(scene.particles().size() > 10 && effects.feedback().hitConfirm > 0,
                "Shots lack muzzle/trail/impact feedback");
        for (const auto &particle : scene.particles())
            require(std::isfinite(particle.position.x) && std::isfinite(particle.color.a),
                    "Impact particle became non-finite");
        RecordingAudioSystem recorder;
        GameAudio audio(recorder);
        CHealthController health(100, &audio, &effects);
        health.attach(player);
        playerCollider.receiveDamage(12, {-20, 0, 0});
        health.update(.016f);
        audio.update(.016f);
        effects.update(.016f);
        require(recorder.lastCue == "player_hit", "Player hit used the ordinary enemy-hit sound");
        require(effects.feedback().damage > .1f && effects.feedback().shield > .5f &&
                    effects.feedback().incomingDirection.x < -.9f,
                "Player damage has no shield pulse or directional feedback");
        require(std::any_of(scene.particles().begin(), scene.particles().end(),
                            [](const auto &p) { return p.kind == ParticleKind::Shield; }),
                "Player shield was not published to the renderer");
        const auto kick = effects.consumeCameraImpulse();
        require(kick > 0 && effects.consumeCameraImpulse() == 0,
                "Camera impulse repeated or was absent");
        CChaseCamera camera(FlightTuning{});
        FlightState flight;
        camera.follow({0, 0, 0}, flight, 0, 16.f / 9);
        const auto neutral = camera.getView();
        camera.addImpact(kick);
        camera.follow({0, 0, 0}, flight, .016f, 16.f / 9);
        float difference = 0;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
            {
                require(std::isfinite(camera.getView()[c][r]), "Camera impact made view singular");
                difference += std::abs(camera.getView()[c][r] - neutral[c][r]);
            }
        require(difference > .0001f, "Player impact did not reach the camera");
        const auto hud = FlightHud::build(camera, {0, 0, 0}, flight, {1280, 720}, nullptr, nullptr,
                                          nullptr, nullptr, &effects.feedback());
        require(std::any_of(hud.rectangles.begin(), hud.rectangles.end(), [](const auto &rect)
                            { return rect.color.r > .9f && rect.color.g < .3f; }),
                "Directional HUD hit indicator is missing");
        for (int i = 0; i < 180; ++i)
            camera.follow({0, 0, 0}, flight, 1.f / 60, 16.f / 9);
        difference = 0;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                difference += std::abs(camera.getView()[c][r] - neutral[c][r]);
        require(difference < .001f, "Hit impulse permanently changed camera framing");
        auto *enemy = new CGameObject();
        enemy->setPosition({10, 0, 20});
        const auto enemyId = world.addObject(enemy);
        CCollidable enemyCollider(0, CAABBox());
        enemy->setCollidable(&enemyCollider);
        CHealthController enemyHealth(100, &audio, &effects);
        enemyHealth.attach(enemy);
        enemyCollider.receiveDamage(100, {0, 0, 0});
        enemyHealth.update(.016f);
        require(enemy->isDead(), "Lethal damage did not destroy the enemy");
        enemy->markDeleted();
        world.update(0);
        effects.update(.016f);
        require(!world.getObject(enemyId) &&
                    std::any_of(scene.particles().begin(), scene.particles().end(),
                                [](const auto &p) { return p.kind == ParticleKind::Debris; }),
                "Destroyed enemy vanished without persistent debris");
        require(std::any_of(scene.particles().begin(), scene.particles().end(),
                            [](const auto &p) { return p.kind == ParticleKind::Shockwave; }),
                "Explosion shockwave missing");
        for (int i = 0; i < 100; ++i)
            effects.damage(*player, 100, true, {0, 0, 0});
        effects.update(.016f);
        require(effects.particleCount() <= tuning.maxParticles &&
                    scene.particles().size() <= tuning.maxParticles + 1,
                "Explosion storm exceeded the particle budget");
        for (int i = 0; i < 180; ++i)
            effects.update(1.f / 60);
        require(effects.particleCount() == 0 && scene.particles().empty() &&
                    effects.feedback().damage < .001f,
                "Effects did not expire or feedback did not settle");
        effects.clear();
        require(scene.particles().empty(), "Effects leaked into the next state");
        std::cout << "Impact effects: shot feedback, player-hit sound/shield/direction, camera "
                     "recovery, persistent explosions and bounded lifetime passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Effects regression: " << e.what() << '\n';
        return 1;
    }
}
