#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IEngine.h"

// Game
class CGameSystem;

// GLFW
struct GLFWwindow;

// Graphics
class IGraphicsSystem;
class CGlfwWindow;
class IRenderer;
class IScene;

// Resource
class IResourceManager;

// Animation
class IInputProvider;
class IControllableCamera;
class CCameraController;
class IGraphicsResourceManager;
class CAnimationWorld;

// Debug
class CDebugInfo;
class CDebugInfoDisplay;

/**
* \brief Demo application class.
*/
class CEngine : public IEngine
{
public:
	CEngine();
	~CEngine();

	/**
	* \brief nitialize demo with config file.
	*/
    bool init(const char* configFile);

	/**
	* \brief Run demo.
	*/
    void run();

private:
    bool initWindow(unsigned int width, unsigned int height, const std::string& title);
	bool initRenderer(const std::string& rendererType);
	bool initScene(const std::string& sceneFile);

	void updateAnimation(float timeDiff);

	// TODO Should use interface instead of concrete class.
	std::shared_ptr<CGameSystem> m_gameSystem; /**< Game system. */
	std::shared_ptr<IGraphicsSystem> m_graphicsSystem; /**< Graphics system. */

    std::shared_ptr<IResourceManager> m_resourceManager =
        nullptr; /**< Resource loader and manager. */
    
	// TODO Should actually be managed by the graphics system.
	std::shared_ptr<IGraphicsResourceManager> m_graphicsResourceManager =
        nullptr; /**< Resource manager for graphics resources. */
    
	std::shared_ptr<CGlfwWindow> m_window = nullptr;
	std::shared_ptr<IInputProvider> m_inputProvider = nullptr;

    std::shared_ptr<IRenderer> m_renderer = nullptr;                 /**< Active renderer. */
    std::shared_ptr<IRenderer> m_deferredRenderer = nullptr;         /**< Deferred renderer. */
    std::shared_ptr<IRenderer> m_forwardRenderer = nullptr;          /**< Forward renderer. */
    std::shared_ptr<IScene> m_scene = nullptr;                       /**< Active scene. */
    std::shared_ptr<IControllableCamera> m_camera = nullptr;         /**< Active camera. */
    std::shared_ptr<CCameraController> m_cameraController = nullptr; /**< Camera controller. */

    std::shared_ptr<CDebugInfo> m_debugInfo = nullptr; /**< Debug information storage. */
    std::shared_ptr<CDebugInfoDisplay> m_debugInfoDisplay =
        nullptr; /**< Debug information renderer. */

	// Animation
	std::shared_ptr<CAnimationWorld> m_animationWorld;
};
