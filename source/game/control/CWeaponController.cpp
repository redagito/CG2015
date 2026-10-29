#include "CWeaponController.h"
#include "CProjectileController.h"
#include "audio/GameAudio.h"
#include "game/combat/ProjectileBallistics.h"
#include "game/effects/CCombatEffects.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
CWeaponController::CWeaponController(IInputProvider *provider, CGameWorld *world, IScene *scene,
                                     ResourceId mesh, ResourceId material,
                                     IResourceManager *resources, CCollisionSystem *collisions,
                                     int group, GameAudio *audio, const WeaponTuning &tuning,
                                     CCombatEffects *effects, CPlayerCombatUpgrades *upgrades)
    : m_upgrades(upgrades), m_effects(effects), m_tuning(tuning), m_audio(audio),
      m_inputProvider(provider), m_gameWorld(world), m_scene(scene), m_mesh(mesh),
      m_material(material), m_collisionSystem(collisions), m_collisionGroup(group)
{
    std::vector<float> vertices, normals, uv;
    std::vector<unsigned> indices;
    EPrimitiveType primitive;
    if (tuning.muzzles.empty() || tuning.interval <= 0 ||
        !resources->getMesh(mesh, vertices, indices, normals, uv, primitive))
        throw std::runtime_error("Cannot prepare projectile mesh");
    m_projectileBounds = CAABBox::create(vertices);
}
CWeaponController::~CWeaponController() = default;
void CWeaponController::attach(CGameObject *object)
{
    m_object = object;
}
void CWeaponController::detach()
{
    m_object = nullptr;
}
void CWeaponController::setActive(bool active)
{
    m_active = active;
}
void CWeaponController::receiveMessage(Message)
{
}
WeaponAim CWeaponController::aim() const
{
    if (!m_object)
        return {};
    return {ProjectileBallistics::muzzlePosition(*m_object, m_tuning.muzzles[m_muzzleIndex]),
            m_object->getForward(), m_object->getVelocity(), m_tuning.projectileSpeed,
            m_tuning.projectileLifetime};
}
void CWeaponController::update(float seconds)
{
    if (!m_active || !m_object || m_object->isDead() || !std::isfinite(seconds) || seconds <= 0)
        return;
    m_weaponCooldown -= std::min(seconds, .25f);
    if (!(m_inputProvider ? m_inputProvider->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)
                          : m_trigger))
    {
        m_weaponCooldown = std::max(0.f, m_weaponCooldown);
        return;
    }
    // Preserve the remainder so cadence does not drift with the render frame rate.
    while (m_weaponCooldown <= 0)
    {
        fire();
        m_weaponCooldown += m_tuning.interval;
    }
}
void CWeaponController::fire()
{
    const auto center = aim();
    if (m_effects)
        m_effects->muzzle(center.muzzle, m_object->getVelocity(), m_tuning.color);
    // Multishot spreads a bounded volley budget instead of cloning full-strength shots.
    const float pelletDamage =
        m_tuning.damage * m_tuning.volleyDamageMultiplier / m_tuning.volleySize;
    auto modifiers = m_tuning.modifiers;
    modifiers.splashDamage = pelletDamage * modifiers.splashFraction;
    for (unsigned pellet = 0; pellet < m_tuning.volleySize; ++pellet)
    {
        auto shot = center;
        const float angle =
            (float(pellet) - float(m_tuning.volleySize - 1) * .5f) * glm::radians(2.5f);
        shot.forward =
            glm::angleAxis(angle, m_object->getOrientation() * glm::vec3(0, 1, 0)) * shot.forward;
        const auto launch = ProjectileBallistics::launch(shot);
        bool critical = false;
        if (m_tuning.criticalChance > 0)
        {
            m_randomState ^= m_randomState << 13;
            m_randomState ^= m_randomState >> 17;
            m_randomState ^= m_randomState << 5;
            critical = float(m_randomState) / float(UINT32_MAX) < m_tuning.criticalChance;
        }
        auto bullet = std::make_unique<CGameObject>();
        bullet->setProjectile(true);
        bullet->setPosition(launch.position);
        bullet->setOrientation(launch.orientation);
        bullet->setVelocity(launch.velocity);
        bullet->setCollidable(m_collisionSystem->add(m_projectileBounds, m_collisionGroup));
        bullet->getCollidable()->setDamage(pelletDamage *
                                           (critical ? m_tuning.criticalMultiplier : 1));
        bullet->getCollidable()->setCollidable(false);
        bullet->addController(std::make_shared<CProjectileController>(
            launch.velocity, m_tuning.projectileLifetime, *m_collisionSystem, m_collisionGroup,
            m_effects, m_inputProvider != nullptr, m_tuning.color * (critical ? 1.5f : 1.f),
            modifiers, m_upgrades));
        bullet->setSceneObject(new CSceneObjectProxy(
            m_scene, m_scene->createObject(m_mesh, m_material, launch.position, launch.orientation,
                                           glm::vec3(1))));
        m_gameWorld->addObject(bullet.release());
    }
    m_muzzleIndex = (m_muzzleIndex + 1) % m_tuning.muzzles.size();
    if (m_audio)
        m_audio->emit({GameAudioEventType::Shot, center.muzzle});
}

void CWeaponController::setTrigger(bool firing)
{
    m_trigger = firing;
}
