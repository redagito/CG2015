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
#include "CFreeFlightCameraController.h"

// Animation
#include "animation/CAnimationWorld.h"

// Game
#include "game/CGameSystem.h"
#include "game/state/CLoadState.h"
#include "game/state/CTitleState.h"
#include "game/state/CGamePlayState.h"
#include "game/state/CDemoState.h"
#include "game/state/CLoseState.h"
#include "game/state/CWinState.h"

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

	// Create and initialize graphics system
	m_graphicsSystem = std::make_shared<CGraphicsSystem>();
	if (!m_graphicsSystem->init(*m_resourceManager))
	{
		LOG_ERROR("Failed to initialize graphics system.");
		return false;
	}

	// Legacy stuff to keep demo mode working
	// TODO Should be removed
	if (modeType == "demo")
	{
		if (!initDemo(sceneFile))
		{
			LOG_ERROR("Failed to initialize demo mode.");
			return false;
		}
	}
	else // Create and initialize game system
	{
		if (!initGameSystem(gameFile))
		{
			LOG_ERROR("Failed to initialize game system.");
			return false;
		}
	}
    return true;
}

void CEngine::run()
{
	// Fn key cooldown values
    double f1Cooldown = 0.0;
	double f2Cooldown = 0.0;
	double f3Cooldown = 0.0;
	double f4Cooldown = 0.0;
	double f5Cooldown = 0.0;
	double f6Cooldown = 0.0;
	double f7Cooldown = 0.0;
	double f8Cooldown = 0.0;
	double f9Cooldown = 0.0;

	// Numeric key cooldown values
	double k1Cooldown = 0.0;
	double k2Cooldown = 0.0;
	double k3Cooldown = 0.0;
	double k4Cooldown = 0.0;
	double k5Cooldown = 0.0;

    double timeDiff = 0.0;
	bool running = true;

	// Activate mouse capture as default
    m_window->toggleMouseCapture();
    
    do
    {
        double startTime = glfwGetTime();

        // Cooldown calculations
        f1Cooldown -= timeDiff;
		f2Cooldown -= timeDiff;
		f3Cooldown -= timeDiff;
		f4Cooldown -= timeDiff;
		f5Cooldown -= timeDiff;
		f6Cooldown -= timeDiff;
		f7Cooldown -= timeDiff;
		f8Cooldown -= timeDiff;
		f9Cooldown -= timeDiff;

		k1Cooldown -= timeDiff;
		k2Cooldown -= timeDiff;
		k3Cooldown -= timeDiff;
		k4Cooldown -= timeDiff;
		k5Cooldown -= timeDiff;

		// Global key mappings
		// Key F1
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F1) == GLFW_PRESS && f1Cooldown <= 0.f)
		{
			f1Cooldown = 0.3f;
			// Not implemented
		}

		// Key F2
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F2) == GLFW_PRESS && f2Cooldown <= 0.f)
		{
			f2Cooldown = 0.3f;
			// Turns debug overlay on / off
			m_graphicsSystem->toggleDebugOverlay();
		}

		// Key F3
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F3) == GLFW_PRESS && f3Cooldown <= 0.f)
		{
			f3Cooldown = 0.3f;
			// Turns wireframe mode on / off
			m_graphicsSystem->toggleWireframeMode();
		}

		// Key F4
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F4) == GLFW_PRESS && f4Cooldown <= 0.f)
		{
			f4Cooldown = 0.3f;
			// Toggles global texture filtering between nearest neighbor and bilinear
			// TODO Implement
		}

		// Key F5
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F5) == GLFW_PRESS && f5Cooldown <= 0.f)
		{
			f5Cooldown = 0.3f;
			// Toggles global texture mip map filtering between off, nearest neighbor and bilinear
			// TODO Implement
		}

		// Key F6
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F6) == GLFW_PRESS && f6Cooldown <= 0.f)
		{
			f6Cooldown = 0.3f;
			// Unused
		}

		// Key F7
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F7) == GLFW_PRESS && f7Cooldown <= 0.f)
		{
			f7Cooldown = 0.3f;
			// Unused
		}

		// Key F8
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_F8) == GLFW_PRESS && f8Cooldown <= 0.f)
		{
			f8Cooldown = 0.3f;
			// Toggles view frustum culling on/off
			m_graphicsSystem->toggleViewFrustumCulling();
		}

		// Key 1
		if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_1) == GLFW_PRESS && k1Cooldown <= 0.f)
		{
			k1Cooldown = 0.3f;
			// Turns mouse capture on/off
			m_window->toggleMouseCapture();
		}

		// Key 2
        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_2) == GLFW_PRESS && k2Cooldown <= 0.f)
        {
            k2Cooldown = 0.3f;
			// Sets active rendering device to deferred renderer
			m_graphicsSystem->setActiveRenderer("deferred");
        }

		// Key 3
        if (glfwGetKey(m_window->getGlfwHandle(), GLFW_KEY_3) == GLFW_PRESS && k3Cooldown <= 0.f)
        {
            k3Cooldown = 0.3f;
			// Sets active rendering device to forward renderer
			m_graphicsSystem->setActiveRenderer("forward");
        }

		// Game system update
		if (!m_gameSystem->update((float)timeDiff))
		{
			// On return false quit application
			running = false;
		}

		// Draw active scene from active camera with active rendering device
		m_graphicsSystem->draw(*m_window);

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

bool CEngine::initGameSystem(const std::string& gameFile)
{
	// Create and set game system
	m_gameSystem = std::make_shared<CGameSystem>();

	// TODO Load from game file
	m_gameSystem->addState("load", new CLoadState("data/world/load_1.json", 10.f));
	m_gameSystem->addState("title", new CTitleState("data/world/intro_1.json"));
	m_gameSystem->addState("game", new CGamePlayState());
	m_gameSystem->addState("lose", new CLoseState("data/world/lose.json"));
	m_gameSystem->addState("win", new CWinState("data/world/win.json"));
	if (!m_gameSystem->init("load", m_graphicsSystem.get(), m_inputProvider.get(), m_resourceManager.get()))
	{
		LOG_ERROR("Failed to initialize game system.");
		return false;
	}
	
	return true;
}

bool CEngine::initDemo(const std::string& sceneFile)
{
	// Create and set game system
	m_gameSystem = std::make_shared<CGameSystem>();

	// Create demo state from with scene file
	m_gameSystem->addState("demo", new CDemoState(sceneFile));
	if (!m_gameSystem->init("demo", m_graphicsSystem.get(), m_inputProvider.get(), m_resourceManager.get()))
	{
		LOG_ERROR("Failed to initialize game system.");
		return false;
	}
	return true;
}