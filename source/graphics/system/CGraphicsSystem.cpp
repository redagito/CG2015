#include "CGraphicsSystem.h"

#include "graphics/scene/CScene.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/camera/CCamera.h"
#include "graphics/renderer/CDeferredRenderer.h"
#include "graphics/renderer/CForwardRenderer.h"

#include "resource/IResourceManager.h"

#include "debug/Log.h"

bool CGraphicsSystem::init(IResourceManager& manager)
{
	// Create graphics resource manager and register as listener to resource manager
	CGraphicsResourceManager* grm = new CGraphicsResourceManager;
	manager.addResourceListener(grm);
	m_resourceManager.reset(grm);
	
	// Create renderers
	// Deferred renderer
	CDeferredRenderer* deferredRenderer = new CDeferredRenderer;
	if (!deferredRenderer->init(manager))
	{
		LOG_ERROR("Failed to initialize deferred renderer.");
		delete deferredRenderer;
		return false;
	}
	m_deferredRenderer.reset(deferredRenderer);
	
	// Forward renderer
	CForwardRenderer* forwardRenderer = new CForwardRenderer;
	if (!forwardRenderer->init(manager))
	{
		LOG_ERROR("Failed to initialize forward renderer.");
		delete forwardRenderer;
		return false;
	}
	m_forwardRenderer.reset(forwardRenderer);
	
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

void CGraphicsSystem::draw(IWindow& window)
{
	if (m_activeScene != nullptr && m_activeCamera != nullptr)
	{
		m_activeRenderer->draw(*m_activeScene, *m_activeCamera, window, *m_resourceManager);
	}
}