#pragma once
#include "EnemyRoster.h"
#include "collision/CAABBox.h"
#include "game/GameObjectId.h"
#include "resource/ResourceConfig.h"
#include <functional>
class CGameWorld;
class IScene;
class IResourceManager;
class CCollisionSystem;
class GameAudio;
class CCombatEffects;
class CEnemySpawner
{
  public:
    CEnemySpawner(const EnemyRoster &roster, const ArenaTuning &arena, CGameWorld &world,
                  IScene &scene, IResourceManager &resources, CCollisionSystem &collisions,
                  GameObjectId player, unsigned group, ResourceId projectileMesh,
                  ResourceId projectileMaterial, GameAudio *audio, std::function<void()> onDefeat,
                  CCombatEffects *effects = nullptr);
    void update(float seconds);
    void setWave(std::size_t wave);
    std::size_t pending() const;

  private:
    struct PreparedType
    {
        ResourceId mesh = invalidResource, material = invalidResource,
                   decorationMesh = invalidResource, decorationMaterial = invalidResource;
        CAABBox bounds;
    };
    bool positionFor(const EnemySpawn &spawn, glm::vec3 &position) const;
    void spawn(const EnemySpawn &request, glm::vec3 position);
    CCombatEffects *m_effects;
    EnemyRoster m_roster;
    ArenaTuning m_arena;
    CGameWorld &m_world;
    IScene &m_scene;
    CCollisionSystem &m_collisions;
    GameObjectId m_player;
    unsigned m_group;
    ResourceId m_projectileMesh, m_projectileMaterial;
    IResourceManager &m_resources;
    GameAudio *m_audio;
    std::function<void()> m_onDefeat;
    std::vector<PreparedType> m_prepared;
    std::size_t m_next = 0;
    std::size_t m_wave = static_cast<std::size_t>(-1);
    float m_elapsed = 0;
};
