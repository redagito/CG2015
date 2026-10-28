#pragma once

#include "AGameState.h"
#include "game/arena/ArenaTuning.h"
#include "game/combat/CTargetingSystem.h"
#include "game/combat/CombatTuning.h"
#include "game/control/CFlightController.h"
#include "game/control/CHealthController.h"
#include "game/control/CMothershipController.h"
#include "game/control/CWeaponController.h"
#include "game/effects/CCombatEffects.h"
#include "game/effects/CMothershipCinematic.h"
#include "game/encounter/CEnemySpawner.h"
#include "game/encounter/EncounterProgress.h"
#include "game/flight/CChaseCamera.h"

#include "animation/CAnimationWorld.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"
#include "graphics/camera/IControllableCamera.h"

#include "input/IInputProvider.h"
#include "resource/IResourceManager.h"

#include "game/CGameObject.h"
#include "game/CGameWorld.h"

#include "collision/CAABBox.h"
#include "collision/CCollisionSystem.h"
class GameAudio;

class CGamePlayState : public AGameState
{
  public:
    CGamePlayState();
    ~CGamePlayState();

    bool init(IGraphicsSystem *graphicsSystem, IInputProvider *inputProvider,
              IResourceManager *resourceManager);
    void onEnter();
    bool update(float dtime);
    void onExit();
    const std::string &getNextState() const;
    void setAudio(GameAudio *audio);

  private:
    void updateFlightPresentation(float seconds);
    void beginMothershipCinematic();
    bool updateMothershipCinematic(float seconds);
    GameAudio *m_audio = nullptr;
    IGraphicsSystem *m_graphicsSystem = nullptr;
    IScene *m_scene = nullptr;
    IResourceManager *m_resourceManager = nullptr;
    IInputProvider *m_inputProvider = nullptr;
    std::shared_ptr<CChaseCamera> m_camera;
    std::shared_ptr<CFlightController> m_flight;
    FlightTuning m_flightTuning;
    CombatTuning m_combatTuning;
    ArenaTuning m_arena;
    CombatEffectsTuning m_effectsTuning;
    std::unique_ptr<CCombatEffects> m_effects;
    std::unique_ptr<CMothershipCinematic> m_cinematic;
    bool m_mothershipDeadline = false;
    CGameObject *m_temple = nullptr;
    glm::vec3 m_templeImpact{0}, m_mothershipEmitterOffset{0};
    float m_resultTimer = -1;
    GameObjectId m_playerId = 0;
    std::shared_ptr<CHealthController> m_playerHealth;
    ResourceId m_projectileMesh = invalidResource, m_projectileMaterial = invalidResource;
    std::shared_ptr<CWeaponController> m_weapon;
    std::unique_ptr<CTargetingSystem> m_targeting;
    bool m_controlsCaptured = false;
    std::unique_ptr<CCollisionSystem> m_collisionSystem; /**< Collision system. */

    CGameObject *m_player = nullptr; /**< Player object. */
    std::shared_ptr<CMothershipController> m_mothershipApproach;
    CGameObject *m_mothership = nullptr; /**< Mothership object. */
    unsigned int m_playerGroup = 0;      /**< Player collision group. */
    unsigned int m_enemyGroup = 0;       /**< Enemy collision group. */

    std::unique_ptr<CEnemySpawner> m_spawner;
    EncounterProgress m_progress;
};
