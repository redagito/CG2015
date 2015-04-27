#pragma once

#include <string>

class IScene;
class IWindow;
class IResourceManager;
class ICamera;

class IGraphicsSystem
{
public:
    virtual ~IGraphicsSystem();
	
	/**
	* \brief Initializes graphics system and sets resource manager listener.
	*/
	virtual bool init(IResourceManager& manager) = 0;

	virtual void setActiveRenderer(const std::string& renderer) = 0;

    /**
    * \brief Create scene.
    */
    virtual IScene* createScene() = 0;

	/**
	* \brief Set active scene.
	*/
	virtual void setActiveScene(IScene* scene) = 0;

	virtual ICamera* createCamera() = 0;

	/**
	* \brief Sets active camera.
	*/
	virtual void setActiveCamera(const ICamera* camera) = 0;

	/**
	* \brief Draws active scene.
	*/
	virtual void draw(IWindow& window) = 0;
};