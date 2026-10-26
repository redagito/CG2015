#include "CGlfwInputProvider.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glm/geometric.hpp>

#include <GLFW/glfw3.h>

#include "input/IInputListener.h"
#include <spdlog/spdlog.h>

std::unordered_map<GLFWwindow *, CGlfwInputProvider *> CGlfwInputProvider::s_instances;

void CGlfwInputProvider::glfwKeyCallback(GLFWwindow *window, int key, int scancode, int action,
                                         int mods)
{
    auto it = s_instances.find(window);
    if (it != s_instances.end())
    {
        EKeyEventType eventType;

        switch (action)
        {
        case GLFW_PRESS:
            eventType = EKeyEventType::KEY_PRESSED;
            break;
        case GLFW_REPEAT:
            eventType = EKeyEventType::KEY_REPEAT;
            break;
        case GLFW_RELEASE:
            eventType = EKeyEventType::KEY_RELEASED;
            break;
        default:
            SPDLOG_WARN("Ignored unknown action '{}' for glfwKeyCallback.", action);
            return;
        }

        for (IInputListener *listener : it->second->m_listeners)
        {
            listener->handleKeyEvent(eventType, key);
        }
    }
}

void CGlfwInputProvider::glfwCursorPositionCallback(GLFWwindow *window, double xpos, double ypos)
{
    auto it = s_instances.find(window);
    if (it != s_instances.end())
    {
        for (IInputListener *listener : it->second->m_listeners)
        {
            listener->handleMouseMovementEvent((int)xpos, (int)ypos);
        }
    }
}

// void glfwCursorEnterCallback(GLFWwindow* window, int entered)
//{
//    auto it = s_instances.find(window);
//    if (it != s_instances.end())
//    {
//        for (IInputListener* listener : it->second.m_listeners)
//        {
//            listener->handleMouseMovementEvent(xpos, ypos);
//        }
//    }
//}

void CGlfwInputProvider::glfwMouseButtonCallback(GLFWwindow *window, int button, int action,
                                                 int mods)
{
    auto it = s_instances.find(window);
    if (it != s_instances.end())
    {
        EMouseButtonEventType eventType;

        switch (action)
        {
        case GLFW_PRESS:
            eventType = EMouseButtonEventType::MOUSE_BUTTON_PRESSED;
            break;
        case GLFW_RELEASE:
            eventType = EMouseButtonEventType::MOUSE_BUTTON_RELEASED;
            break;
        default:
            SPDLOG_WARN("Ignored unknown action '{}' for glfwMouseButtonCallback.", action);
            return;
        }

        for (IInputListener *listener : it->second->m_listeners)
        {
            listener->handleMouseButtonEvent(eventType, button);
        }
    }
}

// void glfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
//{
//    auto it = s_instances.find(window);
//    if (it != s_instances.end())
//    {
//        for (IInputListener* listener : it->second.m_listeners)
//        {
//            listener->handleScrollEvent(xoffset, yoffset);
//        }
//    }
//}

// ===
// class CGlfwInputProvider
// ===

CGlfwInputProvider::CGlfwInputProvider(GLFWwindow *window) : m_window(window)
{
    assert(m_window != nullptr);
    assert(s_instances.find(m_window) == s_instances.end());

    // Set callbacks
    glfwSetKeyCallback(m_window, CGlfwInputProvider::glfwKeyCallback);
    glfwSetCursorPosCallback(m_window, CGlfwInputProvider::glfwCursorPositionCallback);
    glfwSetMouseButtonCallback(m_window, CGlfwInputProvider::glfwMouseButtonCallback);
    // Create mapping for callbacks
    s_instances[m_window] = this;
}

CGlfwInputProvider::~CGlfwInputProvider()
{
    setFlightMode(false, m_pointerRadius);
    // Remove mapping
    s_instances.erase(m_window);
}

void CGlfwInputProvider::addInputListener(IInputListener *listener)
{
    m_listeners.push_back(listener);
}

void CGlfwInputProvider::removeInputListener(IInputListener *listener)
{
    m_listeners.remove(listener);
}

bool CGlfwInputProvider::isKeyPressed(int keyCode)
{
    if (m_window != nullptr)
    {
        return (!m_flightMode || glfwGetWindowAttrib(m_window, GLFW_FOCUSED)) &&
               glfwGetKey(m_window, keyCode) == GLFW_PRESS;
    }
    return false;
}
bool CGlfwInputProvider::isMouseButtonPressed(int button)
{
    return glfwGetWindowAttrib(m_window, GLFW_FOCUSED) &&
           glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}
glm::vec2 CGlfwInputProvider::viewportSize() const
{
    int width = 1, height = 1;
    glfwGetWindowSize(m_window, &width, &height);
    return glm::vec2(std::max(1, width), std::max(1, height));
}
void CGlfwInputProvider::updateCapture()
{
    const bool capture = m_flightMode && glfwGetWindowAttrib(m_window, GLFW_FOCUSED);
    if (capture == m_captured)
        return;
    glfwSetInputMode(m_window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, capture ? GLFW_TRUE : GLFW_FALSE);
    m_captured = capture;
    m_pointer = glm::vec2(0);
    glfwGetCursorPos(m_window, &m_lastCursor.x, &m_lastCursor.y);
}
void CGlfwInputProvider::setFlightMode(bool enabled, float radius)
{
    m_flightMode = enabled;
    m_pointerRadius = std::clamp(radius, .05f, .4f);
    m_pointer = glm::vec2(0);
    updateCapture();
}
glm::vec2 CGlfwInputProvider::pointerOffset()
{
    // Release on focus loss; reacquiring starts at neutral with a fresh baseline.
    updateCapture();
    if (!m_captured)
        return glm::vec2(0);
    glm::dvec2 cursor;
    glfwGetCursorPos(m_window, &cursor.x, &cursor.y);
    const auto size = viewportSize();
    m_pointer += glm::vec2(cursor - m_lastCursor) / std::min(size.x, size.y);
    m_lastCursor = cursor;
    if (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS)
        m_pointer = glm::vec2(0);
    const float length = glm::length(m_pointer);
    if (length > m_pointerRadius)
        m_pointer *= m_pointerRadius / length;
    return m_pointer;
}
