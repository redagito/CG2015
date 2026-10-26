#include <GLFW/glfw3.h>
#include <algorithm>

#include "CLoadState.h"

#include "graphics/camera/CFirstPersonCamera.h"
#include "io/CSceneLoader.h"

#include <spdlog/spdlog.h>

const std::string titleStr = "title";

CLoadState::CLoadState(const std::string& sceneFile, const std::string& manifest)
	:
	m_manifest(manifest),m_sceneFile(sceneFile)
{

}

CLoadState::~CLoadState()
{
	// Empty
}

bool CLoadState::init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager)
{
	m_graphicsSystem = graphicsSystem;
	m_resourceManager = resourceManager;
    m_inputProvider=inputProvider;

	// TODO Refactor, camera movement should be implemented with a single camera and camera controllers.
	m_camera = std::make_shared<CFirstPersonCamera>(
		glm::vec3(6.f, 0.f, -1.5f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), 45.f,
		4.f / 3.f, 0.01f, 1000.f);

	// Disable lense flare
	m_camera->getFeatureInfoForWrite().lenseFlareActive = false;

	// Create new scene object
	m_scene = m_graphicsSystem->createScene();

	// Load scene from file	
	SPDLOG_INFO("Loading scene from file {}.", m_sceneFile.c_str());
	CSceneLoader loader(*resourceManager);
	if (!loader.load(m_sceneFile, *m_scene, m_animationWorld))
	{
		SPDLOG_ERROR("Failed to load scene file {}.", m_sceneFile.c_str());
		return false;
	}
	return true;
}

void CLoadState::onEnter()
{
    m_ready=false;
    m_enterWasDown=m_inputProvider->isKeyPressed(GLFW_KEY_ENTER);
    m_request=m_resourceManager->loadManifestAsync(m_manifest);
	m_graphicsSystem->setActiveCamera(m_camera.get());
	m_graphicsSystem->setActiveScene(m_scene);
}

bool CLoadState::update(float dtime)
{
    const auto status=m_resourceManager->getLoadStatus(m_request);
    const bool ready=status.phase==LoadPhase::Ready;
    const bool enter=m_inputProvider->isKeyPressed(GLFW_KEY_ENTER);
    // Require a fresh press after the ready prompt has actually been shown.
    const bool proceed=m_ready && ready && enter && !m_enterWasDown;
    m_ready=ready;m_enterWasDown=enter;
    if(proceed) return false;
    const bool failed=status.phase==LoadPhase::Failed || status.phase==LoadPhase::Cancelled;
    const bool uploading=status.phase==LoadPhase::Uploading || status.phase==LoadPhase::AwaitingUpload;
    const float progress=uploading ? 0.8f+0.2f*status.published/std::max(1u,status.uploadTotal)
        : status.phase==LoadPhase::Ready?1.f:0.8f*status.prepared/std::max(1u,status.total);
    UiDocument ui;
    ui.rectangles={{{0.5f,0.91f},{300,3},{0.08f,0.12f,0.15f,1.f}},
        {{0.5f,0.91f},{300*progress,3},{0.38f,0.86f,0.93f,1.f},{-(1.f-progress)*150.f,0}}};
    ui.texts={{failed?"LOADING FAILED":ready?"READY  /  PRESS ENTER TO CONTINUE":uploading?"UPLOADING RESOURCES":"PREPARING RESOURCES",
        {0.5f,0.855f},24,{0.85f,0.93f,1.f,1.f},2,0.86f},
        {failed?status.error:ready?"ALL RESOURCES LOADED":std::to_string(uploading?status.published:status.prepared)+" / "+std::to_string(uploading?status.uploadTotal:status.total),
        {0.5f,0.955f},16,{0.6f,0.7f,0.8f,1.f},1,0.9f}};
    m_graphicsSystem->setUiOverlay(ui);
	m_animationWorld.update(dtime);
	return true;
}

void CLoadState::onExit()
{
    m_graphicsSystem->setUiOverlay({});
}

const std::string& CLoadState::getNextState() const
{
	return titleStr;
}
