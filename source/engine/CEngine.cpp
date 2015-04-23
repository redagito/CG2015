#include "CEngine.h"

#include <string>
#include <vector>

// Debug
#include "debug/CDebugInfo.h"
#include "debug/Log.h"
#include "debug/CLogger.h"
#include "debug/Profile.h"

// Graphics
#include "graphics/renderer/core/RendererCoreConfig.h"
#include "graphics/renderer/debug/RendererDebug.h"

#include <GLFW/glfw3.h>

#include "graphics/camera/CFirstPersonCamera.h"
#include "graphics/camera/CFreeCamera.h"

// Renderer types
#include "graphics/renderer/CForwardRenderer.h"
#include "graphics/renderer/CDeferredRenderer.h"

// Resource system
#include "resource/Resource.h"

#include "graphics/scene/CScene.h"
#include "graphics/window/CGlfwWindow.h"
#include "graphics/resource/CGraphicsResourceManager.h"

#include "util/TimeStamp.h"
#include "graphics/CDebugInfoDisplay.h"
#include "input/provider/CGlfwInputProvider.h"
#include "io/CSceneLoader.h"
#include "CCameraController.h"

// Animation
#include "animation/CAnimationWorld.h"

CEngine::CEngine() {}

CEngine::~CEngine() 
{
	LOG_DEBUG("Profiler info: %s", CProfiler::toString().c_str());
}

bool CEngine::init(const char* configFile)
{
    // Init log file
    std::string logFile = "log/" + createTimeStamp() + ".log";
    if (!CLogger::initLogFile(logFile))
    {
        LOG_WARNING("Failed to create log file at %s.", logFile.c_str());
    }

	// Create debug info container and register as listener
    m_debugInfo = std::make_shared<CDebugInfo>();
    CLogger::addListener(m_debugInfo.get());

	// Load config file
    if (!m_config.load(configFile))
    {
        LOG_WARNING("Failed to load config file %s, starting with default settings.",
                    configFile);
		// TODO Return if no config exists?
    }

    // Create window for rendering
    if (!initWindow())
    {
        LOG_ERROR("Failed to initialize window.");
        return false;
    }
	// TODO GLFW handle not properly wrapped away, GFLW should not be used directly
	m_inputProvider = std::make_shared<CGlfwInputProvider>(m_window->getGlfwHandle());

    // Create central resource manager
    m_resourceManager.reset(createResourceManager());
    if (m_resourceManager == nullptr)
    {
        LOG_ERROR("Failed to initialize resource manager.");
        return false;
    }

	// Create animation world
	// TODO Move into gameworld or remove entirely? Only used for simple animations.
	m_animationWorld = std::make_shared<CAnimationWorld>();

	// TODO Everything graphics related shgould be managed/initialized by a graphics system object.
    // Graphics resource manager, listens to resource manager
    CGraphicsResourceManager* manager = new CGraphicsResourceManager;
    m_resourceManager->addResourceListener(manager);
    m_graphicsResourceManager.reset(manager);

    // Create renderer
	// TODO Move to graphics system, user should not create his own renderer.
    if (!initRenderer())
    {
        LOG_ERROR("Failed to initialize renderer.");
        return false;
    }

	// Initialize scene
	// TODO Move to graphics system, scenes should not be managed by the user.
    if (!initScene())
    {
        LOG_ERROR("Failed to initialize scene.");
        return false;
    }

	// TODO Refactor, camera movement should be implemented with a single camera and camera controllers.
    m_camera = std::make_shared<CFirstPersonCamera>(
        glm::vec3(0.5f, 0.f, 0.5f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f), 45.f,
        4.f / 3.f, 0.01f, 1000.f);

    m_cameraController = std::make_shared<CCameraController>();
    m_cameraController->setCamera(m_camera);
    m_cameraController->setInputProvider(m_inputProvider.get());

	// TODO Debug info display is a simple text overlay which prints stats and logs, needs refactoring.
    m_debugInfoDisplay = std::make_shared<CDebugInfoDisplay>(m_resourceManager);

	// Adds input listener
    m_window->addListener(m_cameraController.get());

    return true;
}

void CEngine::run()
{
    double f1Cooldown = 0.0;
    double f2Cooldown = 0.0;
    double f3Cooldown = 0.0;
    double f5Cooldown = 0.0;
    double k1Cooldown = 0.0;
    double timeDiff = 0.0;

    double fpsCoolDown = 1.f;
    unsigned int lastFrameCount = 0;
    unsigned int currentFrameCount = 0;

    bool displayDebugInfo = false;

    m_window->toggleMouseCapture();
    
    do
    {
        double startTime = glfwGetTime();

        // Cooldowns
        f1Cooldown -= timeDiff;
        f2Cooldown -= timeDiff;
        f3Cooldown -= timeDiff;
        f5Cooldown -= timeDiff;
        k1Cooldown -= timeDiff;
        fpsCoolDown -= timeDiff;

        if (fpsCoolDown < 0)
        {
            fpsCoolDown += 1.f;
            lastFrameCount = currentFrameCount;
            currentFrameCount = 0;
        }

        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_1) == GLFW_PRESS && k1Cooldown <= 0.f)
        {
            k1Cooldown = 0.3f;
            displayDebugInfo = !displayDebugInfo;
        }

        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F2) == GLFW_PRESS && f2Cooldown <= 0.f)
        {
            f2Cooldown = 0.3f;
            m_renderer = m_deferredRenderer;
        }

        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F3) == GLFW_PRESS && f3Cooldown <= 0.f)
        {
            f3Cooldown = 0.3f;
            m_renderer = m_forwardRenderer;
        }
        
        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F5) == GLFW_PRESS && f5Cooldown <= 0.f)
        {
            f5Cooldown = 0.3f;
            m_cameraController->loadSequence("data/democam.json");
        }

        m_cameraController->animate((float)timeDiff);

        m_renderer->draw(*m_scene.get(), *m_camera.get(), *m_window.get(),
                         *m_graphicsResourceManager.get());

        if (displayDebugInfo)
        {
            m_debugInfo->setValue("TimeDiff", std::to_string(timeDiff));
            m_debugInfo->setValue("Camera x", std::to_string(m_camera->getPosition().x));
            m_debugInfo->setValue("Camera y", std::to_string(m_camera->getPosition().y));
            m_debugInfo->setValue("Camera z", std::to_string(m_camera->getPosition().z));
            m_debugInfo->setValue("FPS", std::to_string(lastFrameCount));

            m_debugInfoDisplay->draw(*m_debugInfo.get());
        }

		// Perform animation update
		m_animationWorld->update((float) timeDiff);

        m_window->swapBuffer();
		// Update input
		glfwPollEvents();

        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F1) == GLFW_PRESS && f1Cooldown <= 0.f)
        {
            // Reset cooldown
            f1Cooldown = 0.5f;
            // Capure/uncapture mouse
            m_window->toggleMouseCapture();
        }

        timeDiff = glfwGetTime() - startTime;
        ++currentFrameCount;

    } while (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(m_window->getGlfwHandle()) == 0);

    glfwTerminate();

    return;
}

bool CEngine::initWindow()
{
    // Check if already initialized
    if (m_window != nullptr)
    {
        return true;
    }

    LOG_INFO("Initializing application window.");

    // Read config values
    unsigned int width = m_config.getValue("window", "width", 800);
    unsigned int height = m_config.getValue("window", "height", 600);
    std::string title = m_config.getValue("window", "title", "Demo");

    LOG_INFO("Window width: %u.", width);
    LOG_INFO("Window height: %u.", height);
    LOG_INFO("Window title: %s.", title.c_str());

    // Create window
    CGlfwWindow* window = new CGlfwWindow;
    if (!window->init(width, height, title))
    {
        LOG_ERROR("Failed to initialize GLFW window wrapper.");
        delete window;
        return false;
    }

    // Set pointer
    m_window.reset(window);
    return true;
}

bool CEngine::initRenderer()
{
    if (m_renderer != nullptr || m_deferredRenderer != nullptr || m_forwardRenderer != nullptr)
    {
        LOG_INFO("Renderer already initialized and re-initialization is not supported.");
        return true;
    }

    // Initialize deferred renderer
    LOG_INFO("Initializing deferred renderer.");
    m_deferredRenderer.reset(CDeferredRenderer::create(m_resourceManager.get()));
    if (m_deferredRenderer == nullptr)
    {
        LOG_ERROR("Failed to initialize deferred renderer.");
        return false;
    }

    // Initialize forward renderer
    LOG_INFO("Initializing forward renderer.");
    m_forwardRenderer.reset(CForwardRenderer::create(m_resourceManager.get()));
    if (m_forwardRenderer == nullptr)
    {
        LOG_ERROR("Failed to initialize forward renderer.");
        return false;
    }

    // Set renderer
    std::string rendererType = m_config.getValue("renderer", "type", "forward");
    LOG_INFO("Initial renderer type set to %s.", rendererType.c_str());

    // Set renderer object
    if (rendererType == "forward")
    {
        m_renderer = m_forwardRenderer;
    }
    else if (rendererType == "deferred")
    {
        m_renderer = m_deferredRenderer;
    }
    else
    {
        // Should not happen as default renderer is set to forward in config call
        LOG_WARNING("Invalid renderer type %s specified. Fallback to forward renderer.",
                    rendererType.c_str());
        m_renderer = m_forwardRenderer;
    }

    // Initialize renderer resources
    if (m_renderer == nullptr)
    {
        LOG_ERROR("Failed to initialize renderer.");
        return false;
    }
    return true;
}

bool CEngine::initScene()
{
    m_scene = std::make_shared<CScene>();
    CSceneLoader loader(*m_resourceManager);

    // Get startup scene from config
    std::string sceneFile = m_config.getValue("scene", "file", "data/world/test_1.json");
    LOG_INFO("Loading initial scene from file %s.", sceneFile.c_str());
    if (!loader.load(sceneFile, *m_scene, *m_animationWorld))
    {
        LOG_ERROR("Failed to load scene file %s.", sceneFile.c_str());
        m_scene = nullptr;
        return false;
    }
    return true;
}