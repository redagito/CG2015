#include "CGraphicsSystem.h"
#include "graphics/IWindow.h"

#include "graphics/renderer/core/RendererCoreConfig.h"
#include "graphics/scene/CScene.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/camera/CCamera.h"
#include "graphics/camera/IControllableCamera.h"
#include "graphics/renderer/CDeferredRenderer.h"


#include "resource/IResourceManager.h"

#include "util/Time.h"
#include <spdlog/spdlog.h>
#include "debug/Logging.h"

CGraphicsSystem::CGraphicsSystem()
{
    logging::initialize();
}

CGraphicsSystem::~CGraphicsSystem()
{
}

bool CGraphicsSystem::init(IResourceManager& manager)
{
	// Create graphics resource manager and register as listener to resource manager
	CGraphicsResourceManager* grm = new CGraphicsResourceManager;
	m_resourceManager.reset(grm);
	manager.addResourceListener(grm);

	// Create renderers
	// Deferred renderer
	m_deferredRenderer.reset(CDeferredRenderer::create(manager));
	if (m_deferredRenderer == nullptr)
	{
		SPDLOG_ERROR("Failed to initialize deferred renderer.");
		return false;
	}

	// Set default active renderer
	m_activeRenderer = m_deferredRenderer.get();

	// Set debug overlay renderer
	m_debugInfoDisplay.reset(new CDebugInfoDisplay(manager));
    if (!m_uiRenderer.init(manager)) return false;

	// Init ok
	return true;
}

void CGraphicsSystem::setActiveRenderer(const std::string& renderer)
{
    if (renderer != "deferred")
        SPDLOG_WARN("Renderer '{}' is unavailable; using deferred.", renderer.c_str());
    m_activeRenderer = m_deferredRenderer.get();
}
IScene* CGraphicsSystem::createScene()
{
	// TODO Refactor
	IScene* scene = new CScene(m_resourceManager.get());
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

void CGraphicsSystem::setActiveCamera(ICamera* camera)
{
	m_activeCamera = camera;
}

void CGraphicsSystem::toggleDebugOverlay()
{
	m_drawDebugOverlay = !m_drawDebugOverlay;
}

void CGraphicsSystem::toggleDebugOverlayTransparency()
{
	m_transparentDebugOverlay = !m_transparentDebugOverlay;
}

void CGraphicsSystem::toggleViewFrustumCulling()
{
	CScene::setViewFrustumCulling(!CScene::getViewFrustumCulling());
	if (CScene::getViewFrustumCulling())
	{
		SPDLOG_INFO("View frustum culling enabled.");
	}
	else
	{
		SPDLOG_INFO("View frustum culling disabled.");
	}
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
		if (auto* camera = dynamic_cast<IControllableCamera*>(m_activeCamera))
            if (window.getWidth() && window.getHeight())
                camera->setProjection(camera->getFieldOfView(), static_cast<float>(window.getWidth()) / window.getHeight(), camera->getZNear(), camera->getZFar());
        m_activeRenderer->draw(*m_activeScene, *m_activeCamera, window, *m_resourceManager);
	}

    m_uiRenderer.draw(m_uiDocument,window.getWidth(),window.getHeight(),*m_resourceManager);
	// Debug overlay draw
	if (m_drawDebugOverlay && m_activeCamera != nullptr)
	{
		// Update debug info
        m_debugInfo.setLog(logging::overlaySink()->last_formatted(m_debugInfo.getLogBufferSize()));
		m_debugInfo.setValue("TimeDiff", std::to_string(timeDiff));
		m_debugInfo.setValue("Camera x", std::to_string(m_activeCamera->getPosition().x));
		m_debugInfo.setValue("Camera y", std::to_string(m_activeCamera->getPosition().y));
		m_debugInfo.setValue("Camera z", std::to_string(m_activeCamera->getPosition().z));
		m_debugInfo.setValue("FPS", std::to_string(m_lastFrameCount));
        const auto& profiler = static_cast<CDeferredRenderer*>(m_deferredRenderer.get())->profiler();
        for (const auto& [name,ms] : profiler.gpuMilliseconds()) if (name.find('.')==std::string::npos) m_debugInfo.setValue("GPU ms: " + name,std::to_string(ms));
        for (const auto& [name,ms] : profiler.cpuMilliseconds()) if (name.find('.')==std::string::npos) m_debugInfo.setValue("CPU ms: " + name,std::to_string(ms));
		m_debugInfo.setValue("Culled objects", std::to_string(m_activeCamera->getFeatureInfo().culledObjectCount));
		m_debugInfoDisplay->draw(m_debugInfo, m_transparentDebugOverlay);
	}
}
