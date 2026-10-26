#include <GLFW/glfw3.h>

#include "CWinState.h"

#include "graphics/camera/CFirstPersonCamera.h"
#include "io/CSceneLoader.h"

#include <spdlog/spdlog.h>

const std::string gameStr = "game";

CWinState::CWinState(const std::string &sceneFile) : m_sceneFile(sceneFile)
{
}

CWinState::~CWinState()
{
    // Empty
}

bool CWinState::init(IGraphicsSystem *graphicsSystem, IInputProvider *inputProvider,
                     IResourceManager *resourceManager)
{
    m_graphicsSystem = graphicsSystem;
    m_inputProvider = inputProvider;
    m_resourceManager = resourceManager;

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

void CWinState::onEnter()
{

    m_graphicsSystem->setActiveCamera(m_camera.get());
    m_graphicsSystem->setActiveScene(m_scene);
    // Keep the result readable and independent of camera movement and post-processing.
    const glm::vec4 accent(0.38f, 0.86f, 0.93f, 1.f);
    UiDocument ui;
    ui.rectangles = {{{0.5f, 0.105f}, {780, 148}, {0.015f, 0.025f, 0.04f, 0.78f}},
                     {{0.5f, 0.218f}, {120, 2}, accent}};
    ui.texts = {{"YOU WON", {0.5f, 0.075f}, 66, {0.95f, 0.97f, 1.f, 1.f}, 7, 0.88f},
                {"CTHEDRA IS SAFE", {0.5f, 0.16f}, 28, accent, 3, 0.8f},
                {"ESC  /  EXIT", {0.5f, 0.956f}, 16, {0.6f, 0.67f, 0.73f, 1.f}, 2, 0.8f}};
    m_graphicsSystem->setUiOverlay(ui);
}

bool CWinState::update(float dtime)
{
    m_animationWorld.update(dtime);
    return true;
}

void CWinState::onExit()
{
    m_graphicsSystem->setUiOverlay({});
}

const std::string &CWinState::getNextState() const
{
    return gameStr;
}