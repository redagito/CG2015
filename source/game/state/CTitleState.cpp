#include <GLFW/glfw3.h>

#include "CTitleState.h"

#include "graphics/camera/CFirstPersonCamera.h"
#include "io/CSceneLoader.h"

#include "debug/Log.h"

const std::string gameStr = "game";

CTitleState::CTitleState(const std::string& sceneFile)
	:
	m_sceneFile(sceneFile)
{

}

CTitleState::~CTitleState()
{
	// Empty
}

bool CTitleState::init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager)
{
	m_graphicsSystem = graphicsSystem;
	m_inputProvider = inputProvider;
	m_resourceManager = resourceManager;

	// TODO Refactor, camera movement should be implemented with a single camera and camera controllers.
	m_camera = std::make_shared<CFirstPersonCamera>(
		glm::vec3(-4.4f, 5.f, 14.5), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), 45.f,
		4.f / 3.f, 0.01f, 1000.f);

	m_camera->getFeatureInfoForWrite().godRayActive = false;

	// Create new scene object
	m_scene = m_graphicsSystem->createScene();

	// Load scene from file	
	LOG_INFO("Loading scene from file %s.", m_sceneFile.c_str());
	CSceneLoader loader(*resourceManager);
	if (!loader.load(m_sceneFile, *m_scene, m_animationWorld))
	{
		LOG_ERROR("Failed to load scene file %s.", m_sceneFile.c_str());
		return false;
	}
	return true;
}

void CTitleState::onEnter()
{
	m_fadeInTime = 3.f;
	m_graphicsSystem->setActiveCamera(m_camera.get());
	m_graphicsSystem->setActiveScene(m_scene);
}

bool CTitleState::update(float dtime)
{
	// Ignore user input for some time
	if (m_fadeInTime > 0.0f)
	{
		m_fadeInTime -= dtime;
	}
	else
	{
		// Start game on enter key
		if (m_inputProvider->isKeyPressed(GLFW_KEY_ENTER))
		{
			
			return false;
		}
	}
	m_animationWorld.update(dtime);
	return true;
}

void CTitleState::onExit()
{
	// Nothing
}

const std::string& CTitleState::getNextState() const
{
	return gameStr;
}