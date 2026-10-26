#include "TestGraphics.h"
TestGraphics::TestGraphics(const IGraphicsResourceManager *resources) : scene(resources)
{
}
bool TestGraphics::init(IResourceManager &)
{
    return true;
}
void TestGraphics::setUiOverlay(const UiDocument &value)
{
    overlay = value;
}
void TestGraphics::setActiveRenderer(const std::string &)
{
}
IScene *TestGraphics::createScene()
{
    return &scene;
}
void TestGraphics::setActiveScene(IScene *)
{
}
ICamera *TestGraphics::createCamera()
{
    return nullptr;
}
void TestGraphics::setActiveCamera(ICamera *camera)
{
    activeCamera = camera;
}
void TestGraphics::toggleDebugOverlay()
{
}
void TestGraphics::toggleDebugOverlayTransparency()
{
}
void TestGraphics::toggleViewFrustumCulling()
{
}
void TestGraphics::draw(IWindow &)
{
}
