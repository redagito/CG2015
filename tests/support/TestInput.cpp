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
    return (key == GLFW_KEY_ENTER && enter) || (key == GLFW_KEY_SPACE && space);
}
