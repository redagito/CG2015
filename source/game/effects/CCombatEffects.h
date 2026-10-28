#pragma once
#include "CombatEffectsTuning.h"
#include "PlayerFeedback.h"
#include "game/GameObjectId.h"
#include "graphics/effects/RenderParticle.h"
#include <random>
#include <vector>
class CGameWorld;
class CGameObject;
class IScene;
class CCombatEffects
{
  public:
    CCombatEffects(const CGameWorld &world, IScene &scene, const CombatEffectsTuning &tuning);
    void bindPlayer(GameObjectId id);
    bool isPlayer(const CGameObject &object) const;
    void muzzle(glm::vec3 position, glm::vec3 velocity, glm::vec3 color);
    void trail(glm::vec3 position, glm::vec3 velocity, glm::vec3 color);
    void impact(glm::vec3 position, glm::vec3 normal, glm::vec3 color, bool playerShot);
    void damage(const CGameObject &object, float amount, bool lethal, glm::vec3 source);
    void update(float seconds);
    void clear();
    const PlayerFeedback &feedback() const;
    float consumeCameraImpulse();
    std::size_t particleCount() const;
    float trailInterval() const;

  private:
    struct Particle
    {
        RenderParticle render;
        float age = 0, lifetime = 1, growth = 0, drag = 0, gravity = 0, spin = 0, motionScale = 1;
        int priority = 0;
    };
    void add(Particle particle);
    void explode(glm::vec3 position, glm::vec3 inherited, float radius);
    glm::vec3 direction();
    float random(float low, float high);
    const CGameWorld &m_world;
    IScene &m_scene;
    CombatEffectsTuning m_tuning;
    GameObjectId m_player = 0;
    PlayerFeedback m_feedback;
    float m_cameraImpulse = 0, m_shieldTime = 0;
    std::vector<Particle> m_particles;
    std::vector<RenderParticle> m_frame;
    std::mt19937 m_random{0xC7ED2A};
};
