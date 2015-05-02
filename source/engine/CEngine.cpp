#include "CEngine.h"

#include <string>
#include <vector>

// Debug
#include "debug/CDebugInfo.h"
#include "debug/Log.h"
#include "debug/CLogger.h"
#include "debug/Profile.h"

// Graphics
#include "graphics/system/CGraphicsSystem.h"
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
#include "util/StringUtil.h"

#include "graphics/CDebugInfoDisplay.h"
#include "input/provider/CGlfwInputProvider.h"

// IO
#include "io/CIniFile.h"
#include "io/JsonUtil.h"
#include "io/CSceneLoader.h"
#include "CCameraController.h"

// Animation
#include "animation/CAnimationWorld.h"

// Game
#include "game/CGameSystem.h"
#include "game/state/CTitleState.h"
#include "game/state/CGamePlayState.h"

CEngine::CEngine() {}

CEngine::~CEngine() 
{
	m_gameSystem = nullptr;

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

	// Config data with default values
	std::string modeType = "demo"; // Startup mode for the application
	std::string sceneFile = "data/world/test_1.json"; // Scene file to load and render if mode is demo
	std::string gameFile = "data/game/defenders_of_cthedra/game.json"; // Game file to load if mode is game
	std::string rendererType = "deferred"; // Renderer type to use
	
	// Window parameters
	unsigned int windowWidth = 800;
	unsigned int windowHeight = 600;
	std::string windowTitle = "CG 2015";

	// Load config file based on extension
	bool loadSuccess = false;
	if (getFileExtension(configFile) == "ini")
	{
		CIniFile configIni;
		if (configIni.load(configFile))
		{
			// Load values
			modeType = configIni.getValue("mode", "type", "demo");
			sceneFile = configIni.getValue("scene", "file", "data/world/test_1.json");
			gameFile = configIni.getValue("game", "file", "data/game/defenders_of_cthedra/game.json");
			rendererType = configIni.getValue("renderer", "type", "forward");
			windowWidth = configIni.getValue("window", "width", 800);
			windowHeight = configIni.getValue("window", "height", 600);
			windowTitle = configIni.getValue("window", "type", "CG 2015");
			loadSuccess = true;
		}
	}
	else if (getFileExtension(configFile) == "json")
	{
		Json::Value root;
		if (load(configFile, root))
		{
			// Sub nodes
			Json::Value game = root["game"];
			Json::Value renderer = root["renderer"];
			Json::Value window = root["window"];

			// Load values
			modeType = "game"; // Json only supports game mode
			sceneFile = ""; // Scene file not supported/legacy
			load(game, "file", gameFile);
			load(renderer, "type", rendererType);
			load(window, "width", windowWidth);
			load(window, "height", windowHeight);
			load(window, "title", windowTitle);
			loadSuccess = true;
		}
	}
	else
	{
		LOG_WARNING("The config file %s has an unknown file extension.");
	}

	// Check if config loaded successfully
	if (!loadSuccess)
	{
		LOG_WARNING("Failed to load config file %s. Starting with default settings.", configFile);
		// TODO Return if no config exists?
	}

    // Create window for rendering
    if (!initWindow(windowWidth, windowHeight, windowTitle))
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

	// Create and initialize graphics system
	m_graphicsSystem = std::make_shared<CGraphicsSystem>();
	if (!m_graphicsSystem->init(*m_resourceManager))
	{
		LOG_ERROR("Failed to initialize graphics system.");
		return false;
	}

	// Create and initialize game system
	if (!initGameSystem(gameFile))
	{
		LOG_ERROR("Failed to initialize game system.");
		return false;
	}

	// Initialize scene
	// TODO Move to graphics system, scenes should not be managed by the user.
	if (!initScene(sceneFile))
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

	// TODO Hacky, camera and scene should be created by the graphics system
	m_graphicsSystem->setActiveCamera(m_camera.get());
	m_graphicsSystem->setActiveScene(m_scene.get());

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
	bool running = true;

	// Activate mouse capture as default
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

		// Key 1 turns debug overlay on/off
        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_1) == GLFW_PRESS && k1Cooldown <= 0.f)
        {
            k1Cooldown = 0.3f;
			m_graphicsSystem->toggleDebugOverlay();
        }

		// Key F1 turns mouse capture on/off
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F1) == GLFW_PRESS && f1Cooldown <= 0.f)
		{
			// Reset cooldown
			f1Cooldown = 0.5f;
			// Capure/uncapture mouse
			m_window->toggleMouseCapture();
		}

		// Key F2 sets active rendering device to deferred renderer
        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F2) == GLFW_PRESS && f2Cooldown <= 0.f)
        {
            f2Cooldown = 0.3f;
			m_graphicsSystem->setActiveRenderer("deferred");
        }

		// Key F3 sets active rendering device to forward renderer
        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F3) == GLFW_PRESS && f3Cooldown <= 0.f)
        {
            f3Cooldown = 0.3f;
			m_graphicsSystem->setActiveRenderer("forward");
        }
        
		// Key F5 starts replay of camera movement
		// Disabled and to be moved into game state
        // if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F5) == GLFW_PRESS && f5Cooldown <= 0.f)
        // {
        //     f5Cooldown = 0.3f;
        //     m_cameraController->loadSequence("data/democam.json");
        // }

		// Game system update
		if (!m_gameSystem->update((float)timeDiff))
		{
			// On return false quit application
			running = false;
		}

		// Update camera movement
        m_cameraController->animate((float) timeDiff);

		// Draw active scene from active camera with active rendering device
		m_graphicsSystem->draw(*m_window);

		// Perform animation update
		// m_animationWorld->update((float) timeDiff);

		// Swap buffers after draw
        m_window->swapBuffer();

		// Update input
		glfwPollEvents();

		// Frame time
        timeDiff = glfwGetTime() - startTime;

    } while (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(m_window->getGlfwHandle()) == 0 && running);

    glfwTerminate();
    return;
}

bool CEngine::initWindow(unsigned int width, unsigned int height, const std::string& title)
{
    // Check if already initialized
    if (m_window != nullptr)
    {
        return true;
    }

    LOG_INFO("Initializing application window.");
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

bool CEngine::initScene(const std::string& sceneFile)
{
	// Create new scene object
	// TODO Should be done in graphics system
    m_scene = std::make_shared<CScene>();
    CSceneLoader loader(*m_resourceManager);

    // Get startup scene from config
    LOG_INFO("Loading initial scene from file %s.", sceneFile.c_str());
    if (!loader.load(sceneFile, *m_scene, *m_animationWorld))
    {
        LOG_ERROR("Failed to load scene file %s.", sceneFile.c_str());
        m_scene = nullptr;
        return false;
    }
    return true;
}

bool CEngine::initGameSystem(const std::string& gameFile)
{
	// Create and set game system
	m_gameSystem = std::make_shared<CGameSystem>();

	// TODO Load game file
	// TODO Initialize states

	m_gameSystem->addState("title", new CTitleState);
	m_gameSystem->addState("gameplay", new CGamePlayState);
	if (!m_gameSystem->init("title", m_graphicsSystem.get()))
	{
		LOG_ERROR("Failed to initialize game system.");
		return false;
	}
	return true;
}