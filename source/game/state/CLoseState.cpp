#include <GLFW/glfw3.h>

#include "CLoseState.h"
#include "game/progression/CPlayerProgression.h"

#include "graphics/camera/CFirstPersonCamera.h"
#include "io/CSceneLoader.h"

#include <spdlog/spdlog.h>

const std::string gameStr = "game";

CLoseState::CLoseState(const std::string &sceneFile) : m_sceneFile(sceneFile)
{
}

CLoseState::~CLoseState()
{
    // Empty
}

bool CLoseState::init(IGraphicsSystem *graphicsSystem, IInputProvider *inputProvider,
                      IResourceManager *resourceManager)
{
    m_graphicsSystem = graphicsSystem;
    m_inputProvider = inputProvider;
    m_resourceManager = resourceManager;
    if (!m_progression)
        m_progression = std::make_shared<CPlayerProgression>();
    m_tree = std::make_unique<CSkillTreeScreen>(*m_progression);

    // TODO Refactor, camera movement should be implemented with a single camera and camera
    // controllers.
    m_camera = std::make_shared<CFirstPersonCamera>(
        glm::vec3(-4.4f, 5.f, 14.5), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), 45.f,
        4.f / 3.f, 0.01f, 1000.f);

    m_camera->getFeatureInfoForWrite().godRayActive = false;
    m_camera->getFeatureInfoForWrite().lenseFlareActive = false;
    m_camera->getFeatureInfoForWrite().dofActive = false;

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

void CLoseState::onEnter()
{

    m_graphicsSystem->setActiveCamera(m_camera.get());
    m_graphicsSystem->setActiveScene(m_scene);
    m_inputProvider->setFlightMode(false);
    m_tree->onEnter(*m_inputProvider);
    m_graphicsSystem->setUiOverlay(
        m_tree->build(m_inputProvider->viewportSize(), m_inputProvider->cursorPosition()));
}

bool CLoseState::update(float dtime)
{
    m_animationWorld.update(dtime);
    const bool keep = m_tree->update(*m_inputProvider);
    m_graphicsSystem->setUiOverlay(
        m_tree->build(m_inputProvider->viewportSize(), m_inputProvider->cursorPosition()));
    return keep;
}

void CLoseState::onExit()
{
    m_graphicsSystem->setUiOverlay({});
}

const std::string &CLoseState::getNextState() const
{
    return gameStr;
}