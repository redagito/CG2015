#include "CGamePlayState.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

//GameObject
#include "game/CGameObject.h"

//Controller
#include "game/control/CPlayerMovementController.h"
#include "game/control/CCameraController.h"
#include "game/control/CRestrictPositionController.h"
#include "game/control/CWeaponController.h"
#include "game/control/CSimpleWaypointController.h"

#include "graphics/camera/CFirstPersonCamera.h"

#include "io/CSceneLoader.h"
#include "animation/CAnimationWorld.h"

const std::string exitStr = "lose";

CGamePlayState::CGamePlayState() 
:
m_enemyCount(7),
m_enemyTime(4.f),
m_enemyXPosition(-75.f)
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
	m_player->addController(std::make_shared<CPlayerMovementController>(inputProvider, 10.f));
	m_player->addController(std::make_shared<CCameraController>(m_camera.get()));
	m_player->addController(std::make_shared<CRestrictPositionController>(glm::vec2(-100.f, -100.f), glm::vec2(100.f, 100.f)));

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
	m_player->addController(std::make_shared<CWeaponController>(m_inputProvider, &getGameWorld(), m_scene, bulletMesh, bulletMaterial));
	m_player->setPosition(glm::vec3(0.f, 20.f, -100.f));
	m_player->setRotation(glm::vec3(0.f));
	m_player->setScale(glm::vec3(0.5f));

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
	m_mothership = new CGameObject();
	m_mothership->setPosition(glm::vec3(0.f, 68.f, 700.f));
	m_mothership->setRotation(glm::vec3(0.f));
	m_mothership->setScale(glm::vec3(60.f));
	m_mothership->addController(std::make_shared<CSimpleWaypointController>(m_mothership->getPosition(), glm::vec3(0.f, 68.f, 0.f), 10.f));

	// Get model resources
	ResourceId motherShip = m_resourceManager->loadMesh("data/mesh/mothership.obj");
	if (motherShip == invalidResource)
	{
		return false;
	}
	ResourceId motherShipMaterial = m_resourceManager->loadMaterial("data/material/mothership.json");
	if (motherShipMaterial == invalidResource)
	{
		return false;
	}
	// Create scene object
	CSceneObjectProxy* motherSceneObject = new CSceneObjectProxy(m_scene, m_scene->createObject(motherShip, motherShipMaterial, glm::vec3(0.f, 68.f, 700.f), glm::vec3(0.f), glm::vec3(60.f)));
	m_mothership->setSceneObject(motherSceneObject);

	// Add player
	getGameWorld().addObject(m_mothership);

	// Create pyramide
	CGameObject* m_pyramide = new CGameObject();
	m_pyramide->setPosition(glm::vec3(0.f, 2.f, 0.f));
	m_pyramide->setRotation(glm::vec3(0.f,-90.f,0.f));
	m_pyramide->setScale(glm::vec3(3.f));

	// Get model resources
	ResourceId pyramide = m_resourceManager->loadMesh("data/mesh/piramyde.obj");
	if (pyramide == invalidResource)
	{
		return false;
	}
	ResourceId pyramideMaterial = m_resourceManager->loadMaterial("data/material/sand.json");
	if (pyramideMaterial == invalidResource)
	{
		return false;
	}
	// Create scene object
	CSceneObjectProxy* pyramideSceneObject = new CSceneObjectProxy(m_scene, m_scene->createObject(pyramide, pyramideMaterial, glm::vec3(0.f, 2.f, 0.f), glm::vec3(0.f, -90.f, 0.f), glm::vec3(3.f)));
	m_pyramide->setSceneObject(pyramideSceneObject);

	// Add player
	getGameWorld().addObject(m_pyramide);


	//LOAD ENEMY RESOURCE
	// Get model resources
	enemyShip = m_resourceManager->loadMesh("data/mesh/enemy.obj");
	if (enemyShip == invalidResource)
	{
		return false;
	}
	enemyShipMaterial = m_resourceManager->loadMaterial("data/material/enemy.json");
	if (enemyShipMaterial == invalidResource)
	{
		return false;
	}

	return true;
}

void CGamePlayState::onEnter()
{
	m_graphicsSystem->setActiveCamera(m_camera.get());
	m_graphicsSystem->setActiveScene(m_scene);
}

bool CGamePlayState::update(float dtime)
{
	
	m_enemyTime -= dtime;
	if (m_enemyCount > 0.f && m_enemyTime <= 0.f)
	{
		m_enemyTime = 2.f;
		// Create new enemy
		CGameObject* enemy = new CGameObject();
		enemy->setPosition(glm::vec3(m_enemyXPosition, 25.f, 0.f));
		enemy->setRotation(glm::vec3(0.f));
		enemy->setScale(glm::vec3(0.4f));
		enemy->addController(std::make_shared<CRestrictPositionController>(glm::vec2(-100.f, -100.f), glm::vec2(100.f, 100.f)));
		enemy->addController(std::make_shared<CSimpleWaypointController>(enemy->getPosition(), glm::vec3(m_enemyXPosition, 25.f, 101.f), 10.f));


		// Create scene object
		CSceneObjectProxy* enemySceneObject = new CSceneObjectProxy(m_scene, m_scene->createObject(enemyShip, enemyShipMaterial, glm::vec3(m_enemyXPosition, 25.f, 0.f), glm::vec3(0.f), glm::vec3(0.4f)));
		enemy->setSceneObject(enemySceneObject);

        // Add enemy
		getGameWorld().addObject(enemy);
		m_enemyCount -= 1;
		m_enemyXPosition += 25.f;
	}

	if (m_inputProvider->isKeyPressed(GLFW_KEY_P))
	{

		return false;
	}
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