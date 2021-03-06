#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IEngine.h"

// Resource
class IResourceManager;

// Game
class CGameSystem;

// Graphics
class IGraphicsSystem;
class CGlfwWindow;

// Input
class IInputProvider;

/**
* \brief Demo application class.
*/
class CEngine : public IEngine
{
public:
	CEngine();
	~CEngine();

	/**
	* \brief Initialize demo from config file.
	*/
    bool init(const char* configFile);

	/**
	* \brief Run demo.
	*/
    void run();

private:
	/**
	* \brief Creates and initializes the application window.
	*/
    bool initWindow(unsigned int width, unsigned int height, const std::string& title);
	
	/**
	* \brief Creates and initializes the game system.
	*/
	bool initGameSystem(const std::string& gameFile);

	/**
	* \brief Creates and initializes game system in demo mode.
	* TODO Should be considered legacy and removed later.
	*/
	bool initDemo(const std::string& sceneFile);

	// TODO Should use interface instead of concrete class.
	std::shared_ptr<CGameSystem> m_gameSystem = nullptr; /**< Game system. */
	std::shared_ptr<IGraphicsSystem> m_graphicsSystem = nullptr; /**< Graphics system. */
    std::shared_ptr<IResourceManager> m_resourceManager = nullptr; /**< Resource loader and manager. */
    
	// TODO Should use interface instead of concrete class
	std::shared_ptr<CGlfwWindow> m_window = nullptr; /**< Application window. */
	std::shared_ptr<IInputProvider> m_inputProvider = nullptr; /**< User input provider. */
};
