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

	// Add player
	m_world.addObject(m_player);

	CGameObject* o = new CGameObject;
	o->setRotation(glm::vec3(0.f));
	o->setPosition(glm::vec3(0.f));
	o->setScale(glm::vec3(1.f));
	getGameWorld().addObject(o);
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
	// Update game world
	getGameWorld().update(dtime);
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