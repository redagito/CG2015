#include "CGamePlayState.h"
#include "audio/GameAudio.h"
#include "game/combat/CombatStatus.h"
#include "game/control/CAudioEmitterController.h"
#include "game/control/CFlightAudioController.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

// GameObject
#include "game/CGameObject.h"

// Controller
#include "game/control/CHealthController.h"

#include "game/flight/FlightHud.h"

#include "game/control/CWeaponController.h"

#include "animation/CAnimationWorld.h"
#include "io/CSceneLoader.h"

// Collision
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"

const std::string exitStr = "lose";
const std::string exitStrW = "win";

CGamePlayState::CGamePlayState() = default;

CGamePlayState::~CGamePlayState()
{
    if (m_controlsCaptured)
        m_inputProvider->setFlightMode(false);
    // Controllers and proxies must release before the scene/physics owners.
    m_cinematic.reset();
    getGameWorld().clear();
}

bool CGamePlayState::init(IGraphicsSystem *graphicsSystem, IInputProvider *inputProvider,
                          IResourceManager *resourceManager)
{
    m_collisionSystem = std::make_unique<CCollisionSystem>();
    m_graphicsSystem = graphicsSystem;
    m_inputProvider = inputProvider;
    m_resourceManager = resourceManager;
    m_scene = m_graphicsSystem->createScene();

    // Collision group ids for static entities, enemy objects and the player model
    m_playerGroup = m_collisionSystem->getNewGroupId();
    m_enemyGroup = m_collisionSystem->getNewGroupId();

    CAnimationWorld animWorld;
    CSceneLoader loader(*resourceManager);
    loader.load("data/world/game_1.json", *m_scene, animWorld);

    m_effectsTuning = CombatEffectsTuning::load(*resourceManager);
    m_effects = std::make_unique<CCombatEffects>(getGameWorld(), *m_scene, m_effectsTuning);
    m_resultTimer = -1;
    m_arena = ArenaTuning::load(*resourceManager);
    m_combatTuning = CombatTuning::load(*resourceManager);
    m_targeting = std::make_unique<CTargetingSystem>(m_combatTuning.targeting);
    m_flightTuning = FlightTuning::load(*resourceManager);
    m_camera = std::make_shared<CChaseCamera>(m_flightTuning);

    // Disable god ray
    m_camera->getFeatureInfoForWrite().godRayActive = false;
    m_camera->getFeatureInfoForWrite().lenseFlareActive = false;
    m_camera->getFeatureInfoForWrite().dofActive = false;

    // Create player
    m_player = new CGameObject();

    m_flight = std::make_shared<CFlightController>(*inputProvider, m_flightTuning, &m_arena);
    m_player->addController(m_flight);
    m_playerHealth =
        std::make_shared<CHealthController>(m_arena.enemies.playerHealth, m_audio, m_effects.get());
    m_player->addController(m_playerHealth);

    // Load bullet
    ResourceId bulletMesh = m_resourceManager->loadMesh("data/mesh/bullet.obj");
    if (bulletMesh == invalidResource)
    {
        return false;
    }
    ResourceId bulletMaterial = m_resourceManager->loadMaterial("data/material/white_glowing.json");
    if (bulletMaterial == invalidResource)
    {
        return false;
    }

    m_projectileMesh = bulletMesh;
    m_projectileMaterial = bulletMaterial;
    m_weapon = std::make_shared<CWeaponController>(
        m_inputProvider, &getGameWorld(), m_scene, bulletMesh, bulletMaterial, m_resourceManager,
        m_collisionSystem.get(), m_playerGroup, m_audio, m_combatTuning.weapon, m_effects.get());
    m_player->addController(m_weapon);
    m_player->setPosition(m_arena.center + glm::vec3(0, -30, -130));
    m_player->setRotation(glm::vec3(0.f));
    m_player->setScale(glm::vec3(0.5f));
    m_player->addController(
        std::make_shared<CFlightAudioController>(m_audio, m_flight, m_combatTuning.engine));

    // Get model resources
    ResourceId playerShip = m_resourceManager->loadMesh("data/mesh/ship_2.obj");
    if (playerShip == invalidResource)
    {
        return false;
    }

    // Player collidable added to player collision group
    std::vector<float> playerVertices;
    std::vector<unsigned int> playerIndices;
    std::vector<float> playerNormals;
    std::vector<float> playerUvs;
    EPrimitiveType playerType;
    m_resourceManager->getMesh(playerShip, playerVertices, playerIndices, playerNormals, playerUvs,
                               playerType);
    m_player->setCollidable(m_collisionSystem->add(CAABBox::create(playerVertices), m_playerGroup));
    m_player->getCollidable()->setDamage(0);

    ResourceId playerShipMaterial =
        m_resourceManager->loadMaterial("data/material/line_metal.json");
    if (playerShipMaterial == invalidResource)
    {
        return false;
    }
    // Create scene object
    CSceneObjectProxy *playerSceneObject = new CSceneObjectProxy(
        m_scene, m_scene->createObject(playerShip, playerShipMaterial, glm::vec3(0.f),
                                       glm::quat(0.f, 0.f, 0.f, 0.f), glm::vec3(1.f)));
    m_player->setSceneObject(playerSceneObject);

    // Add player
    m_playerId = getGameWorld().addObject(m_player);
    m_effects->bindPlayer(m_playerId);

    // Create mothership
    m_mothership = new CGameObject();
    m_mothership->setPosition(glm::vec3(0.f, 68.f, 700.f));
    m_mothership->setRotation(glm::vec3(0.f));
    m_mothership->setScale(glm::vec3(60.f));
    m_mothershipApproach = std::make_shared<CMothershipController>(
        glm::vec3(0.f, 68.f, 0.f), m_arena.mothership,
        [this]
        {
            if (m_resultTimer < 0 && !m_player->isDead() && !m_progress.complete())
            {
                m_mothershipDeadline = true;
                getGameWorld().freezeSimulation();
            }
        });
    m_mothership->addController(m_mothershipApproach);
    m_mothership->addController(std::make_shared<CAudioEmitterController>(m_audio, "ufo_engine"));

    // Get model resources
    ResourceId motherShip = m_resourceManager->loadMesh("data/mesh/mothership.obj");
    if (motherShip == invalidResource)
    {
        return false;
    }
    std::vector<float> cinematicVertices, cinematicNormals, cinematicUvs;
    std::vector<unsigned> cinematicIndices;
    EPrimitiveType cinematicPrimitive;
    m_resourceManager->getMesh(motherShip, cinematicVertices, cinematicIndices, cinematicNormals,
                               cinematicUvs, cinematicPrimitive);
    const auto mothershipBounds = CAABBox::create(cinematicVertices);
    m_mothershipEmitterOffset =
        glm::vec3(0,
                  (mothershipBounds.getMid().y - mothershipBounds.getHalfWidths().y) *
                          m_mothership->getScale().y -
                      .2f,
                  0);
    ResourceId motherShipMaterial =
        m_resourceManager->loadMaterial("data/material/mothership.json");
    if (motherShipMaterial == invalidResource)
    {
        return false;
    }
    // Create scene object
    CSceneObjectProxy *motherSceneObject = new CSceneObjectProxy(
        m_scene, m_scene->createObject(motherShip, motherShipMaterial, glm::vec3(0.f, 68.f, 700.f),
                                       glm::quat(0.f, 0.f, 0.f, 0.f), glm::vec3(60.f)));
    m_mothership->setSceneObject(motherSceneObject);

    // Add player
    getGameWorld().addObject(m_mothership);

    // Create pyramide
    m_temple = new CGameObject();
    m_temple->setPosition(glm::vec3(0.f, 2.f, 0.f));
    m_temple->setRotation(glm::vec3(0.f, -90.f, 0.f));
    m_temple->setScale(glm::vec3(3.f));

    // Get model resources
    ResourceId pyramide = m_resourceManager->loadMesh("data/mesh/piramyde.obj");
    if (pyramide == invalidResource)
    {
        return false;
    }

    cinematicVertices.clear();
    cinematicIndices.clear();
    cinematicNormals.clear();
    cinematicUvs.clear();
    m_resourceManager->getMesh(pyramide, cinematicVertices, cinematicIndices, cinematicNormals,
                               cinematicUvs, cinematicPrimitive);
    const auto templeBounds = CAABBox::create(cinematicVertices);
    m_templeImpact = m_temple->getPosition() +
                     glm::vec3(0,
                               (templeBounds.getMid().y + templeBounds.getHalfWidths().y) *
                                   m_temple->getScale().y,
                               0);
    ResourceId pyramideMaterial = m_resourceManager->loadMaterial("data/material/sand.json");
    if (pyramideMaterial == invalidResource)
    {
        return false;
    }
    // Create scene object
    // glm::vec3(0.f, -90.f, 0.f)
    CSceneObjectProxy *pyramideSceneObject = new CSceneObjectProxy(
        m_scene, m_scene->createObject(pyramide, pyramideMaterial, glm::vec3(0.f, 2.f, 0.f),
                                       glm::quat(0.f, 0.f, 0.f, 0.f), glm::vec3(3.f)));
    m_temple->setSceneObject(pyramideSceneObject);

    // Add player
    getGameWorld().addObject(m_temple);

    const auto roster = EnemyRoster::load(*resourceManager, m_arena, m_combatTuning.weapon);
    m_progress = {roster.spawns.size(), 0};
    m_spawner = std::make_unique<CEnemySpawner>(
        roster, m_arena, getGameWorld(), *m_scene, *resourceManager, *m_collisionSystem, m_playerId,
        m_enemyGroup, m_projectileMesh, m_projectileMaterial, m_audio,
        [this] { m_progress.recordDefeat(); }, m_effects.get());
    m_spawner->update(0);

    return true;
}

void CGamePlayState::onEnter()
{
    m_inputProvider->setFlightMode(true, m_flightTuning.pointerRadius);
    m_controlsCaptured = true;
    m_camera->reset();
    updateFlightPresentation(0);
    m_graphicsSystem->setActiveCamera(m_camera.get());
    m_graphicsSystem->setActiveScene(m_scene);
}

bool CGamePlayState::update(float dtime)
{
    if (m_cinematic)
        return updateMothershipCinematic(dtime);

    if (m_resultTimer < 0)
        m_spawner->update(dtime);

    if (m_inputProvider->isKeyPressed(GLFW_KEY_K))
    {
        m_camera->getFeatureInfoForWrite().toonActive = true;
    }
    if (m_inputProvider->isKeyPressed(GLFW_KEY_L))
    {
        m_camera->getFeatureInfoForWrite().toonActive = false;
    }
    // Update collision system
    m_collisionSystem->update();
    m_collisionSystem->beginStep();
    // Update gameworld
    const bool running = AGameState::update(dtime);
    if (m_mothershipDeadline)
    {
        beginMothershipCinematic();
        return updateMothershipCinematic(0);
    }
    m_effects->update(dtime);
    m_camera->addImpact(m_effects->consumeCameraImpulse());
    if ((m_player->isDead() || m_progress.complete()) && m_resultTimer < 0)
    {
        m_resultTimer = m_effectsTuning.resultDelay;
        m_mothershipApproach->setActive(false);
        m_flight->setActive(false);
        m_weapon->setActive(false);
        m_inputProvider->setFlightMode(false);
        m_controlsCaptured = false;
    }
    if (m_resultTimer >= 0)
        m_resultTimer = std::max(0.f, m_resultTimer - std::min(dtime, .25f));
    updateFlightPresentation(dtime);
    if (m_audio)
    {
        // The listener follows the player, while camera orientation controls pan.
        const auto inverseView = glm::inverse(m_camera->getView());
        m_audio->setListener(m_player->getPosition(), -glm::vec3(inverseView[2]),
                             glm::vec3(inverseView[1]));
    }
    return running && m_resultTimer != 0;
}

void CGamePlayState::onExit()
{
    m_inputProvider->setFlightMode(false);
    m_controlsCaptured = false;
    m_cinematic.reset();
    m_graphicsSystem->setUiOverlay({});
    if (m_effects)
        m_effects->clear();
    if (m_audio)
        m_audio->stopEmitters();
}

void CGamePlayState::setAudio(GameAudio *audio)
{
    m_audio = audio;
}

const std::string &CGamePlayState::getNextState() const
{
    if (!m_mothershipDeadline && m_progress.complete() && m_player && !m_player->isDead())
    {
        return exitStrW;
    }
    else
    {
        return exitStr;
    }
}

void CGamePlayState::updateFlightPresentation(float seconds)
{
    const auto viewport = m_inputProvider->viewportSize();
    const float aspect = viewport.x / std::max(viewport.y, 1.f);
    m_camera->follow(m_player->getPosition(), m_flight->state(), seconds, aspect);
    const auto aim = m_weapon->aim();
    m_targeting->update(getGameWorld().targets(), m_player->getPosition(), m_player->getForward(),
                        aim, *m_inputProvider);
    const auto &solution = m_targeting->solution();
    const CombatStatus status{m_playerHealth->health(),
                              m_arena.enemies.playerHealth,
                              m_progress.defeated,
                              m_progress.total,
                              m_mothershipApproach->approachRemaining(),
                              m_mothershipApproach->totalRemaining()};
    m_graphicsSystem->setUiOverlay(FlightHud::build(
        *m_camera, m_player->getPosition(), m_flight->state(), viewport, &aim,
        solution ? &*solution : nullptr, &m_arena, &status, &m_effects->feedback()));
}

void CGamePlayState::beginMothershipCinematic()
{
    m_flight->setActive(false);
    m_weapon->setActive(false);
    m_playerHealth->setActive(false);
    m_mothershipApproach->setActive(false);
    m_inputProvider->setFlightMode(false);
    m_controlsCaptured = false;
    m_effects->clear();
    if (m_audio)
        m_audio->enterState("cinematic");
    m_cinematic = std::make_unique<CMothershipCinematic>(
        *m_scene, *m_temple, m_mothership->getPosition() + m_mothershipEmitterOffset,
        m_templeImpact, m_audio);
    m_graphicsSystem->setActiveCamera(&m_cinematic->camera());
}
bool CGamePlayState::updateMothershipCinematic(float seconds)
{
    const auto viewport = m_inputProvider->viewportSize();
    m_cinematic->update(seconds, viewport);
    m_graphicsSystem->setUiOverlay(m_cinematic->overlay(viewport));
    return !m_cinematic->finished();
}
