#include "TestInput.h"
#include <GLFW/glfw3.h>
void TestInput::addInputListener(IInputListener *)
{
}
void TestInput::removeInputListener(IInputListener *)
{
}
bool TestInput::isKeyPressed(int key)
{
    return (key == GLFW_KEY_ENTER && enter) || (key == GLFW_KEY_SPACE && space) ||
           (key == GLFW_KEY_W && throttleUp) || (key == GLFW_KEY_S && throttleDown) ||
           (key == GLFW_KEY_A && rollLeft) || (key == GLFW_KEY_D && rollRight) ||
           (key == GLFW_KEY_LEFT_SHIFT && boost);
}

bool TestInput::isMouseButtonPressed(int button)
{
    return button == GLFW_MOUSE_BUTTON_LEFT && fire;
}
glm::vec2 TestInput::pointerOffset()
{
    return pointer;
}
glm::vec2 TestInput::viewportSize() const
{
    return viewport;
}
void TestInput::setFlightMode(bool enabled, float)
{
    flightMode = enabled;
    pointer = glm::vec2(0);
}
