#include "CEnemySpawner.h"
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
#include "game/CGameWorld.h"
#include "game/arena/ArenaBoundary.h"
#include "game/control/CArenaEnemyController.h"
#include "game/control/CAudioEmitterController.h"
#include "game/control/CFollowObjectController.h"
#include "game/control/CHealthController.h"
#include "game/control/CRemoveOnDeathController.h"
#include "game/control/CWeaponController.h"
#include "graphics/IScene.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>
CEnemySpawner::CEnemySpawner(const EnemyRoster &roster, const ArenaTuning &arena, CGameWorld &world,
                             IScene &scene, IResourceManager &resources,
                             CCollisionSystem &collisions, GameObjectId player, unsigned group,
                             ResourceId projectileMesh, ResourceId projectileMaterial,
                             GameAudio *audio, std::function<void()> onDefeat,
                             CCombatEffects *effects)
    : m_effects(effects), m_roster(roster), m_arena(arena), m_world(world), m_scene(scene),
      m_collisions(collisions), m_player(player), m_group(group), m_projectileMesh(projectileMesh),
      m_projectileMaterial(projectileMaterial), m_resources(resources), m_audio(audio),
      m_onDefeat(std::move(onDefeat))
{
    for (const auto &type : roster.types)
    {
        PreparedType prepared;
        prepared.mesh = resources.loadMesh(type.mesh);
        prepared.material = resources.loadMaterial(type.material);
        std::vector<float> vertices, normals, uv;
        std::vector<unsigned> indices;
        EPrimitiveType primitive;
        if (prepared.material == invalidResource ||
            !resources.getMesh(prepared.mesh, vertices, indices, normals, uv, primitive))
            throw std::runtime_error("Unprepared enemy assets: " + type.id);
        prepared.bounds = CAABBox::create(vertices);
        if (!type.decoration.mesh.empty())
        {
            prepared.decorationMesh = resources.loadMesh(type.decoration.mesh);
            prepared.decorationMaterial = resources.loadMaterial(type.decoration.material);
            if (prepared.decorationMesh == invalidResource ||
                prepared.decorationMaterial == invalidResource)
                throw std::runtime_error("Unprepared enemy decoration: " + type.id);
        }
        m_prepared.push_back(prepared);
    }
}
std::size_t CEnemySpawner::pending() const
{
    return m_roster.spawns.size() - m_next;
}
bool CEnemySpawner::positionFor(const EnemySpawn &spawn, glm::vec3 &position) const
{
    const auto *player = m_world.getObject(m_player);
    const auto neighbors = m_world.targets();
    for (int attempt = 0; attempt < 32; ++attempt)
    {
        auto offset = spawn.offset;
        if (attempt)
        {
            const float angle = (spawn.phase + attempt) * 2.39996323f;
            offset = {std::cos(angle) * .7f, std::sin(angle * 1.7f) * .45f, std::sin(angle) * .7f};
        }
        auto desired = m_arena.center + offset * m_arena.halfExtents;
        if (player && spawn.playerDistance > 0)
            desired = player->getPosition() +
                      glm::normalize(offset + glm::vec3(.001f, 0, 0)) * spawn.playerDistance;
        position = ArenaBoundary::safePoint(desired, m_arena);
        if (player && glm::distance(position, player->getPosition()) < 60)
            continue;
        bool free = true;
        const float spacing = m_roster.types[spawn.type].behavior.separationDistance;
        for (const auto &other : neighbors)
            if (glm::distance(position, other.position) < spacing * 1.4f)
            {
                free = false;
                break;
            }
        if (free)
            return true;
    }
    return false; // Crowded wave waits for room instead of spawning inside another ship.
}
void CEnemySpawner::update(float seconds)
{
    if (!std::isfinite(seconds) || seconds < 0)
        return;
    m_elapsed += std::min(seconds, .25f);
    while (m_next < m_roster.spawns.size() && m_roster.spawns[m_next].wave <= m_wave &&
           m_roster.spawns[m_next].time <= m_elapsed)
    {
        if (m_world.targets().size() >= m_roster.maxActive)
            break;
        glm::vec3 position;
        if (!positionFor(m_roster.spawns[m_next], position))
            break;
        spawn(m_roster.spawns[m_next], position);
        ++m_next;
    }
}
void CEnemySpawner::spawn(const EnemySpawn &request, glm::vec3 position)
{
    const auto &type = m_roster.types[request.type];
    const auto &prepared = m_prepared[request.type];
    auto enemy = std::make_unique<CGameObject>();
    enemy->setTargetName(std::format("{} {}", type.name, request.ordinal));
    enemy->setPosition(position);
    enemy->setScale(type.scale);
    if (const auto *player = m_world.getObject(m_player))
    {
        const auto direction = player->getPosition() - position;
        if (glm::dot(direction, direction) > 1e-6f)
            enemy->setOrientation(glm::rotation(glm::vec3(0, 0, 1), glm::normalize(direction)));
    }
    enemy->setCollidable(m_collisions.add(prepared.bounds, m_group));
    auto weapon = std::make_shared<CWeaponController>(
        nullptr, &m_world, &m_scene, m_projectileMesh, m_projectileMaterial, &m_resources,
        &m_collisions, m_group, m_audio, type.weapon, m_effects);
    enemy->addController(std::make_shared<CHealthController>(type.health, m_audio, m_effects));
    enemy->addController(std::make_shared<CRemoveOnDeathController>(m_onDefeat));
    auto behavior = m_arena;
    behavior.enemies = type.behavior;
    enemy->addController(std::make_shared<CArenaEnemyController>(m_world, m_player, behavior,
                                                                 weapon, request.phase));
    enemy->addController(weapon);
    enemy->addController(std::make_shared<CAudioEmitterController>(m_audio, type.engineCue));
    enemy->setSceneObject(new CSceneObjectProxy(
        &m_scene, m_scene.createObject(prepared.mesh, prepared.material, position,
                                       enemy->getOrientation(), type.scale)));
    const auto owner = m_world.addObject(enemy.release());
    if (prepared.decorationMesh != invalidResource)
    {
        auto decoration = std::make_unique<CGameObject>();
        decoration->setPosition(position);
        decoration->setScale(type.decoration.scale);
        decoration->addController(std::make_shared<CFollowObjectController>(m_world, owner));
        decoration->setSceneObject(new CSceneObjectProxy(
            &m_scene,
            m_scene.createObject(prepared.decorationMesh, prepared.decorationMaterial, position,
                                 glm::quat(1, 0, 0, 0), type.decoration.scale)));
        m_world.addObject(decoration.release());
    }
}

void CEnemySpawner::setWave(std::size_t wave)
{
    m_wave = wave;
    m_elapsed = 0;
}
