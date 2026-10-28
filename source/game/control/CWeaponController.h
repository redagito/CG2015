#pragma once

#include "collision/CAABBox.h"
#include "game/CGameWorld.h"
#include "game/IController.h"
#include "game/Message.h"
#include "game/combat/WeaponAim.h"
#include "game/combat/WeaponTuning.h"
#include "graphics/IScene.h"
#include "input/IInputProvider.h"
#include "resource/IResourceManager.h"

// Collision
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
class GameAudio;
class CCombatEffects;

/**
 * \brief Controls weapons, creates projectiles.
 */
class CWeaponController : public IController
{
  public:
    CWeaponController(IInputProvider *provider, CGameWorld *gameWorld, IScene *scene,
                      ResourceId mesh, ResourceId material, IResourceManager *resourceManager,
                      CCollisionSystem *collisionSystem, int collisionGroup,
                      GameAudio *audio = nullptr, const WeaponTuning &tuning = WeaponTuning{},
                      CCombatEffects *effects = nullptr);
    ~CWeaponController();
    WeaponAim aim() const;
    void setTrigger(bool firing);

    void attach(CGameObject *object);
    void detach();
    void update(float dtime);
    void setActive(bool state);
    void receiveMessage(Message msg);

  private:
    void fire();
    CCombatEffects *m_effects;
    WeaponTuning m_tuning;
    CAABBox m_projectileBounds;
    std::size_t m_muzzleIndex = 0;
    GameAudio *m_audio = nullptr;
    CGameObject *m_object = nullptr;         /**< Controlled game object. */
    bool m_active = true, m_trigger = false; /**< Active state flag. */
    IInputProvider *m_inputProvider = nullptr;
    float m_weaponCooldown = 0.f;
    CGameWorld *m_gameWorld = nullptr;
    IResourceManager *m_resourceManager = nullptr;
    IScene *m_scene = nullptr;
    ResourceId m_mesh = invalidResource;
    ResourceId m_material = invalidResource;
    CCollisionSystem *m_collisionSystem = nullptr;

    unsigned int m_collisionGroup; /**< Player collision group. */
};
