#pragma once
#include "graphics/renderer/DeferredRenderTargets.h"
class IResourceManager;
class IScene;
class ICamera;
class IWindow;
class IGraphicsResourceManager;
class ISceneQuery;

class BackgroundPass
{
public:
    BackgroundPass(const BackgroundPass&) = delete;
    BackgroundPass& operator=(const BackgroundPass&) = delete;
    BackgroundPass(
        ColorTarget& sceneColor,
        SunState& sun,
        FullscreenResources& fullscreen)
        : m_sceneColor(sceneColor),
          m_sun(sun),
          m_fullscreen(fullscreen) {}
    bool init(IResourceManager& manager);
    void render(
        const IScene& scene,
        const ICamera& camera,
        const IWindow& window,
        const IGraphicsResourceManager& manager);
private:

    ColorTarget& m_sceneColor;
    SunState& m_sun;
    FullscreenResources& m_fullscreen;
    ResourceId m_backgroundShader = invalidResource;
};
