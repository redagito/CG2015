#pragma once
#include "graphics/IGraphicsSystem.h"
#include "graphics/scene/CScene.h"

class TestGraphics : public IGraphicsSystem
{
  public:
    explicit TestGraphics(const IGraphicsResourceManager *resources);
    CScene scene;
    UiDocument overlay;
    ICamera *activeCamera = nullptr;
    bool init(IResourceManager &) override;
    void setUiOverlay(const UiDocument &) override;
    void setActiveRenderer(const std::string &) override;
    IScene *createScene() override;
    void setActiveScene(IScene *) override;
    ICamera *createCamera() override;
    void setActiveCamera(ICamera *) override;
    void toggleDebugOverlay() override;
    void toggleDebugOverlayTransparency() override;
    void toggleViewFrustumCulling() override;
    void draw(IWindow &) override;
};
