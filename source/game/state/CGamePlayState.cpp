#include "CGamePlayState.h"

#include <glm/glm.hpp>

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

//GameObject
#include "game/CGameObject.h"

//Controller
#include "game/control/CPlayerMovementController.h"
#include "game/control/CCameraController.h"
#include "game/control/CRestrictPositionController.h"

#include "graphics/camera/CFirstPersonCamera.h"

#include "io/CSceneLoader.h"
#include "animation/CAnimationWorld.h"

const std::string exitStr = "exit";

CGamePlayState::CGamePlayState()
{

}

CGamePlayState::~CGamePlayState()
{
	// Empty
}

bool CGamePlayState::init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager)
{
	m_graphicsSystem = graphicsSystem;
	m_inputProvider = inputProvider;
	m_resourceManager = resourceManager;
	m_scene = m_graphicsSystem->createScene();

	CAnimationWorld animWorld;
	CSceneLoader loader(*resourceManager);
	loader.load("data/world/game_1.json", *m_scene, animWorld);

	m_camera = std::make_shared<CFirstPersonCamera>(
		glm::vec3(90.f, 70.f, 90.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), 45.f,
		4.f / 3.f, 0.01f, 1000.f);

	// Disable god ray
	m_camera->getFeatureInfoForWrite().godRayActive = false;

	// Create player
	m_player = new CGameObject();
	m_player->addController(std::make_shared<CPlayerMovementController>(inputProvider, 20.f));
	m_player->addController(std::make_shared<CCameraController>(m_camera.get()));
	m_player->addController(std::make_shared<CRestrictPositionController>(glm::vec2(-100.f, -100.f), glm::vec2(100.f, 100.f)));
	m_player->setPosition(glm::vec3(0.f));
	m_player->setRotation(glm::vec3(0.f));
	m_player->setScale(glm::vec3(1.f));

	// Get model resources
	ResourceId playerShip = m_resourceManager->loadMesh("data/mesh/ship_2.obj");
	if (playerShip == invalidResource)
	{
		return false;
	}
	ResourceId playerShipMaterial = m_resourceManager->loadMaterial("data/material/metallic_galvanized.json");
	if (playerShipMaterial == invalidResource)
	{
		return false;
	}
	// Create scene object
	CSceneObjectProxy* playerSceneObject = new CSceneObjectProxy(m_scene, m_scene->createObject(playerShip, playerShipMaterial, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f)));
	m_player->setSceneObject(playerSceneObject);

	// Add player
	getGameWorld().addObject(m_player);

	// Create mothership
	CGameObject* m_mothership = new CGameObject();
	m_mothership->setPosition(glm::vec3(0.f, 68.f,0.f));
	m_mothership->setRotation(glm::vec3(0.f));
	m_mothership->setScale(glm::vec3(60.f));

	// Get model resources
	ResourceId motherShip = m_resourceManager->loadMesh("data/mesh/mothership.obj");
	if (motherShip == invalidResource)
	{
		return false;
	}
	ResourceId motherShipMaterial = m_resourceManager->loadMaterial("data/material/mothership.json");
	if (playerShipMaterial == invalidResource)
	{
		return false;
	}
	// Create scene object
	CSceneObjectProxy* motherSceneObject = new CSceneObjectProxy(m_scene, m_scene->createObject(motherShip, motherShipMaterial, glm::vec3(0.f, 65.f, 0.f), glm::vec3(0.f), glm::vec3(60.f)));
	m_mothership->setSceneObject(motherSceneObject);

	// Add player
	getGameWorld().addObject(m_mothership);


	return true;
}

void CGamePlayState::onEnter()
{
	m_graphicsSystem->setActiveCamera(m_camera.get());
	m_graphicsSystem->setActiveScene(m_scene);
}

bool CGamePlayState::update(float dtime)
{
	// Update gameworld
	AGameState::update(dtime);
	return true;
}

void CGamePlayState::onExit()
{
	// Cleanup?
}

const std::string& CGamePlayState::getNextState() const
{
	return exitStr;
}