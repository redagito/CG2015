#include "TestViewport.h"
#include "graphics/window/CGlfwWindow.h"
TestViewport::TestViewport(const CGlfwWindow &value) : context(value)
{
}
unsigned TestViewport::getWidth() const
{
    return width;
}
unsigned TestViewport::getHeight() const
{
    return height;
}
void TestViewport::setActive() const
{
    context.setActive();
}
