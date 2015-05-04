#include "CGamePlayState.h"

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

//GameObject
#include "game/CGameObject.h"

//Controller
#include "game/control/CPlayerMovementController.h"
#include "game/control/CCameraController.h"
#include "graphics/camera/CFirstPersonCamera.h"

#include "io/CSceneLoader.h"
#include "animation/CAnimationWorld.h"

#include <glm/glm.hpp>

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
	m_player->addController(std::shared_ptr<IController>(new CPlayerMovementController(inputProvider, 20.f)));
	m_player->addController(std::make_shared<CCameraController>(m_camera.get()));
	m_player->setPosition(glm::vec3(0.f));
	m_player->setRotation(glm::vec3(0.f));
	m_player->setScale(glm::vec3(1.f));

	// Get model resources
	ResourceId playerShip = m_resourceManager->loadMesh("data/mesh/ship_1.obj");
	ResourceId playerShipMaterial = m_resourceManager->loadMaterial("data/material/metallic_galvanized.json");
	// Create scene object
	CSceneObjectProxy* playerSceneObject = new CSceneObjectProxy(m_scene, m_scene->createObject(playerShip, playerShipMaterial, glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f)));
	m_player->setSceneObject(playerSceneObject);

	// Add player
	getGameWorld().addObject(m_player);

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

	// Restrict ship to flight height 5 to 20
	if (m_player->getPosition().y >= 20.f) 
	{
		m_player->setPosition(glm::vec3(m_player->getPosition().x, 20.f, m_player->getPosition().z));
	}
	else if (m_player->getPosition().y <= 5.f) 
	{
		m_player->setPosition(glm::vec3(m_player->getPosition().x, 5.f, m_player->getPosition().z));
	}

	// Game field is -100 to +100
	// If player exits o side, transport to other side
	// Check X
	if (m_player->getPosition().x < -100.f)
	{
		m_player->setPosition(glm::vec3(100.f, m_player->getPosition().y, m_player->getPosition().z));
	}
	else if (m_player->getPosition().x > 100.f)
	{
		m_player->setPosition(glm::vec3(-100.f, m_player->getPosition().y, m_player->getPosition().z));
	}
	// Check Z
	if (m_player->getPosition().z < -100.f)
	{
		m_player->setPosition(glm::vec3(m_player->getPosition().x, m_player->getPosition().y, 100));
	}
	else if (m_player->getPosition().z > 100.f)
	{
		m_player->setPosition(glm::vec3(m_player->getPosition().x, m_player->getPosition().y, -100));
	}
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