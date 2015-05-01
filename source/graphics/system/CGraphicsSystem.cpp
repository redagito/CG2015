#include "CGraphicsSystem.h"

#include "graphics/scene/CScene.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/camera/CCamera.h"
#include "graphics/renderer/CDeferredRenderer.h"
#include "graphics/renderer/CForwardRenderer.h"

#include "resource/IResourceManager.h"

#include "util/Time.h"
#include "debug/Log.h"

CGraphicsSystem::CGraphicsSystem()
{
	// Add debug info as log listener
	CLogger::addListener(&m_debugInfo);
}

CGraphicsSystem::~CGraphicsSystem()
{
	// Remove debug info log listener
	CLogger::removeListener(&m_debugInfo);
}

bool CGraphicsSystem::init(IResourceManager& manager)
{
	// Create graphics resource manager and register as listener to resource manager
	CGraphicsResourceManager* grm = new CGraphicsResourceManager;
	manager.addResourceListener(grm);
	m_resourceManager.reset(grm);
	
	// Create renderers
	// Deferred renderer
	m_deferredRenderer.reset(CDeferredRenderer::create(manager));
	if (m_deferredRenderer == nullptr)
	{
		LOG_ERROR("Failed to initialize deferred renderer.");
		return false;
	}
	
	// Forward renderer
	m_forwardRenderer.reset(CForwardRenderer::create(manager));
	if (m_forwardRenderer == nullptr)
	{
		LOG_ERROR("Failed to initialize forward renderer.");
		return false;
	}
	
	// Set default active renderer
	m_activeRenderer = m_deferredRenderer.get();

	// Set debug overlay renderer
	m_debugInfoDisplay.reset(new CDebugInfoDisplay(manager));

	// Init ok
	return true;
}

void CGraphicsSystem::setActiveRenderer(const std::string& renderer)
{
	if (renderer == "deferred")
	{
		m_activeRenderer = m_deferredRenderer.get();
	}
	else if (renderer == "forward")
	{
		m_activeRenderer = m_forwardRenderer.get();
	}
	else
	{
		// Use default
		LOG_WARNING("Invalid or unknown renderer type %s. Setting renderer to default.", renderer.c_str());
		m_activeRenderer = m_deferredRenderer.get();
	}
}

IScene* CGraphicsSystem::createScene()
{
	IScene* scene = new CScene;
	m_scenes.push_back(std::unique_ptr<IScene>(scene));
	return scene;
}

void CGraphicsSystem::setActiveScene(IScene* scene)
{
	m_activeScene = scene;
}

ICamera* CGraphicsSystem::createCamera()
{
	ICamera* camera = new CCamera;
	m_cameras.push_back(std::unique_ptr<ICamera>(camera));
	return camera;
}

void CGraphicsSystem::setActiveCamera(const ICamera* camera)
{
	m_activeCamera = camera;
}

void CGraphicsSystem::toggleDebugOverlay()
{
	m_drawDebugOverlay = !m_drawDebugOverlay;
}

void CGraphicsSystem::draw(IWindow& window)
{
	// Current calling time
	double callTime = getTime();
	double timeDiff = 0.0;

	// Check for first call to draw
	// TODO Actually necessary?
	if (m_lastCallTime == 0.0)
	{
		m_lastCallTime = callTime;
	}
	else
	{
		// Time difference between calls
		timeDiff = callTime - m_lastCallTime;
		// Set last call time to current call time
		m_lastCallTime = callTime;
	}

	// Update time accumulator
	m_timeAccum += timeDiff;
	// Check if time accumulator passed 1 sec
	if (m_timeAccum >= 1.0)
	{
		// Set last frame count for fps calculation
		m_lastFrameCount = m_currentFrameCount;
		// Reset current frame count
		m_currentFrameCount = 0;
		// Reset time accumulator
		m_timeAccum = 0.0;
	}

	// Update frame count
	++m_currentFrameCount;

	// Scene draw
	if (m_activeScene != nullptr && m_activeCamera != nullptr)
	{
		m_activeRenderer->draw(*m_activeScene, *m_activeCamera, window, *m_resourceManager);
	}

	// Debug overlay draw
	if (m_drawDebugOverlay)
	{
		// Update debug info
		m_debugInfo.setValue("TimeDiff", std::to_string(timeDiff));
		m_debugInfo.setValue("Camera x", std::to_string(m_activeCamera->getPosition().x));
		m_debugInfo.setValue("Camera y", std::to_string(m_activeCamera->getPosition().y));
		m_debugInfo.setValue("Camera z", std::to_string(m_activeCamera->getPosition().z));
		m_debugInfo.setValue("FPS", std::to_string(m_lastFrameCount));
		m_debugInfoDisplay->draw(m_debugInfo);
	}
}