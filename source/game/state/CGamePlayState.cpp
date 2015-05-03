#include "CGamePlayState.h"

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

//GameObject
#include "game\CGameObject.h"

//Controller
#include "game\control\CPlayerMovementController.h"

#include <glm/glm.hpp>

const std::string exitStr = "exit";

CGamePlayState::CGamePlayState(const std::string& sceneFile)
:
m_sceneFile(sceneFile)
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

	//Create player
	m_player = new CGameObject();
	m_player->addController(std::shared_ptr<IController>(new CPlayerMovementController(20.f)));
	m_player->setPosition(glm::vec3(0.f));
	m_player->setRotation(glm::vec3(0.f));
	m_player->setScale(glm::vec3(1.f));

	// Add player
	m_world.addObject(m_player);

	return true;
}

void CGamePlayState::onEnter()
{
	m_fadeInTime = 3.f;
	m_graphicsSystem->setActiveCamera(m_camera.get());
	m_graphicsSystem->setActiveScene(m_scene);
}

bool CGamePlayState::update(float dtime)
{
	
	if (m_player->getPosition().x > 20.f) {
		m_player->setPosition(glm::vec3(5.f, m_player->getPosition().y, m_player->getPosition().z));
	}
	if (m_player->getPosition().y > 20.f) {
		m_player->setPosition(glm::vec3(m_player->getPosition().x, 5.f, m_player->getPosition().z));
	}
	if (m_player->getPosition().x < 5.f) {
		m_player->setPosition(glm::vec3(20.f, m_player->getPosition().y, m_player->getPosition().z));
	}
	if (m_player->getPosition().y < 5.f) {
		m_player->setPosition(glm::vec3(m_player->getPosition().x, 20.f, m_player->getPosition().z));
	}
	// Update game world
	m_world.update(dtime);
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