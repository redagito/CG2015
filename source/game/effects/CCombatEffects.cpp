#include "CCombatEffects.h"
#include "collision/CCollidable.h"
#include "game/CGameWorld.h"
#include "graphics/IScene.h"
#include <algorithm>
#include <cmath>
CCombatEffects::CCombatEffects(const CGameWorld &world, IScene &scene,
                               const CombatEffectsTuning &tuning)
    : m_world(world), m_scene(scene), m_tuning(tuning)
{
    m_particles.reserve(tuning.maxParticles);
    m_frame.reserve(tuning.maxParticles + 1);
}
void CCombatEffects::bindPlayer(GameObjectId id)
{
    m_player = id;
}
bool CCombatEffects::isPlayer(const CGameObject &object) const
{
    return m_world.getObject(m_player) == &object;
}
float CCombatEffects::random(float low, float high)
{
    return std::uniform_real_distribution<float>(low, high)(m_random);
}
glm::vec3 CCombatEffects::direction()
{
    glm::vec3 value;
    do
    {
        value = {random(-1, 1), random(-1, 1), random(-1, 1)};
    } while (glm::dot(value, value) < .001f);
    return glm::normalize(value);
}
void CCombatEffects::add(Particle particle)
{
    if (m_particles.size() < m_tuning.maxParticles)
    {
        m_particles.push_back(particle);
        return;
    }
    // Bound saturation work per emission, even when thousands of ships explode together.
    for (std::size_t probe = 0; probe < std::min<std::size_t>(32, m_particles.size()); ++probe)
    {
        auto &candidate = m_particles[m_replacementCursor++ % m_particles.size()];
        if (candidate.priority <= particle.priority)
        {
            candidate = particle;
            return;
        }
    }
}
void CCombatEffects::muzzle(glm::vec3 position, glm::vec3 velocity, glm::vec3 color)
{
    Particle flash;
    flash.render = {position, velocity * .03f,   glm::vec4(color * 3.f, 1), {.6f, .6f},
                    0,        ParticleKind::Glow};
    flash.lifetime = .07f;
    flash.priority = 1;
    add(flash);
}
void CCombatEffects::trail(glm::vec3 position, glm::vec3 velocity, glm::vec3 color)
{
    Particle particle;
    particle.render = {position, velocity * .025f,    glm::vec4(color * 3.f, .7f), {.8f, .065f},
                       0,        ParticleKind::Streak};
    // Keep the direction for streak orientation, but only a little residual drift.
    particle.render.velocity = velocity;
    particle.motionScale = .025f;
    particle.drag = 12;
    particle.lifetime = m_tuning.trailLifetime;
    add(particle);
}
void CCombatEffects::impact(glm::vec3 position, glm::vec3 normal, glm::vec3 color, bool playerShot)
{
    if (glm::dot(normal, normal) < 1e-6f)
        normal = {0, 1, 0};
    normal = glm::normalize(normal);
    Particle flash;
    flash.render = {position + normal * .08f, {0, 0, 0}, {4, 2, .7f, 1}, {.7f, .7f}, 0,
                    ParticleKind::Glow};
    flash.lifetime = .09f;
    flash.priority = 2;
    add(flash);
    for (int i = 0; i < 14; ++i)
    {
        auto spread = direction();
        if (glm::dot(spread, normal) < 0)
            spread = -spread;
        Particle spark;
        spark.render = {position + normal * .06f,
                        spread * random(5, 18) + normal * 3.f,
                        glm::vec4(glm::mix(color, glm::vec3(3, 1.4f, .3f), .6f), 1),
                        {random(.25f, .7f), .045f},
                        0,
                        ParticleKind::Streak};
        spark.lifetime = random(.2f, .5f);
        spark.drag = 2;
        spark.gravity = 4;
        spark.priority = 2;
        add(spark);
    }
    if (playerShot)
        m_feedback.hitConfirm = .14f;
}
void CCombatEffects::explode(glm::vec3 position, glm::vec3 inherited, float radius)
{
    Particle flash;
    flash.render = {position, inherited * .15f,  {7, 4, 1.5f, 1}, {radius, radius},
                    0,        ParticleKind::Glow};
    flash.lifetime = .16f;
    flash.growth = radius * 5;
    flash.priority = 4;
    add(flash);
    Particle ring;
    ring.render = {position, inherited * .1f,        {3, 1, .15f, .8f}, {radius, radius},
                   0,        ParticleKind::Shockwave};
    ring.lifetime = .65f;
    ring.growth = radius * 12;
    ring.priority = 3;
    add(ring);
    for (int i = 0; i < 48; ++i)
    {
        Particle particle;
        const auto outward = direction();
        particle.render = {position + outward * radius * .2f,
                           inherited * .25f + outward * random(7, 25),
                           {random(1.5f, 4), random(.3f, 1.3f), .08f, 1},
                           {random(.2f, .7f), .05f},
                           random(0, 6.28f),
                           ParticleKind::Streak};
        particle.lifetime = random(.4f, 1.3f);
        particle.drag = 1.2f;
        particle.gravity = 5;
        particle.priority = 2;
        add(particle);
    }
    for (int i = 0; i < 12; ++i)
    {
        Particle debris;
        debris.render = {position + direction() * radius * .4f,
                         inherited * .35f + direction() * random(4, 16),
                         {1.6f, .6f, .15f, 1},
                         {random(.1f, .35f), random(.1f, .35f)},
                         random(0, 6.28f),
                         ParticleKind::Debris};
        debris.lifetime = random(.8f, 1.8f);
        debris.spin = random(-8, 8);
        debris.gravity = 9;
        debris.drag = .5f;
        debris.priority = 2;
        add(debris);
    }
    for (int i = 0; i < 10; ++i)
    {
        Particle smoke;
        smoke.render = {position + direction() * radius * .5f,
                        inherited * .1f + direction() * 3.f + glm::vec3(0, 3, 0),
                        {.10f, .12f, .16f, .38f},
                        {radius * .7f, radius * .7f},
                        random(0, 6.28f),
                        ParticleKind::Smoke};
        smoke.lifetime = random(1, 2);
        smoke.growth = radius;
        smoke.drag = 1;
        smoke.priority = 1;
        add(smoke);
    }
}
void CCombatEffects::damage(const CGameObject &object, float amount, bool lethal, glm::vec3 source)
{
    if (isPlayer(object))
    {
        const float strength = std::clamp(amount / 40.f, .2f, 1.f);
        m_feedback.damage = std::min(1.f, m_feedback.damage + strength);
        m_shieldTime = m_tuning.shieldDuration;
        const auto direction = source - object.getPosition();
        if (glm::dot(direction, direction) > 1e-6f)
            m_feedback.incomingDirection = glm::normalize(direction);
        m_cameraImpulse = std::min(1.f, m_cameraImpulse + strength * m_tuning.cameraKick);
    }
    if (lethal)
    {
        float radius = 2;
        if (object.hasCollidable())
            radius = std::clamp(
                glm::length(object.getCollidable()->getAABBox().getHalfWidths()) * .45f, 1.f, 5.f);
        explode(object.getPosition(), object.getVelocity(), radius);
    }
}
void CCombatEffects::update(float seconds)
{
    if (!std::isfinite(seconds) || seconds < 0)
        return;
    const float dt = std::min(seconds, .25f);
    m_feedback.damage *= std::exp(-6 * dt);
    m_feedback.hitConfirm = std::max(0.f, m_feedback.hitConfirm - dt);
    m_shieldTime = std::max(0.f, m_shieldTime - dt);
    m_feedback.shield = m_shieldTime / m_tuning.shieldDuration;
    for (auto &particle : m_particles)
    {
        particle.age += dt;
        particle.render.velocity *= std::exp(-particle.drag * dt);
        particle.render.velocity.y -= particle.gravity * dt;
        particle.render.position += particle.render.velocity * dt * particle.motionScale;
        particle.render.halfSize += glm::vec2(particle.growth * dt);
        particle.render.rotation += particle.spin * dt;
    }
    std::erase_if(m_particles,
                  [](const auto &particle) { return particle.age >= particle.lifetime; });
    // Publish values rather than object pointers: bursts outlive the destroyed ship.
    m_frame.clear();
    for (const auto &particle : m_particles)
    {
        auto render = particle.render;
        const float remaining = 1 - particle.age / particle.lifetime;
        render.color.a *= remaining * remaining;
        m_frame.push_back(render);
    }
    if (m_shieldTime > 0)
    {
        if (const auto *player = m_world.getObject(m_player); player && !player->isDead())
        {
            const auto center = player->hasCollidable()
                                    ? player->getCollidable()->getAABBox().getMid()
                                    : player->getPosition();
            m_frame.push_back({center,
                               {0, 0, 0},
                               {.4f, 6.f, 12.f, m_feedback.shield * .8f},
                               {m_tuning.shieldRadius, m_tuning.shieldRadius},
                               0,
                               ParticleKind::Shield});
        }
    }
    m_scene.setParticles(m_frame);
}
void CCombatEffects::clear()
{
    m_particles.clear();
    m_frame.clear();
    m_feedback = {};
    m_shieldTime = 0;
    m_cameraImpulse = 0;
    m_scene.setParticles(m_frame);
}
const PlayerFeedback &CCombatEffects::feedback() const
{
    return m_feedback;
}
float CCombatEffects::consumeCameraImpulse()
{
    const float value = m_cameraImpulse;
    m_cameraImpulse = 0;
    return value;
}
std::size_t CCombatEffects::particleCount() const
{
    return m_particles.size();
}

float CCombatEffects::trailInterval() const
{
    return m_tuning.trailInterval;
}

void CCombatEffects::burst(glm::vec3 position, float radius)
{
    Particle glow;
    glow.render = {position, {0, 0, 0},         {4, 1.8f, .3f, .8f}, {radius * .35f, radius * .35f},
                   0,        ParticleKind::Glow};
    glow.lifetime = .15f;
    glow.priority = 3;
    add(glow);
    Particle ring;
    ring.render = {position,
                   {0, 0, 0},
                   {3, .8f, .12f, .7f},
                   {radius * .4f, radius * .4f},
                   0,
                   ParticleKind::Shockwave};
    ring.lifetime = .35f;
    ring.growth = radius * 2;
    ring.priority = 3;
    add(ring);
}
