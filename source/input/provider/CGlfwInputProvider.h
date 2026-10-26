#pragma once

#include <list>
#include <unordered_map>

#include "input/IInputProvider.h"

struct GLFWwindow;

/**
 * \brief Provides user input.
 * Uses the glfw api.
 */
class CGlfwInputProvider : public IInputProvider
{
  public:
    CGlfwInputProvider(GLFWwindow *window);
    ~CGlfwInputProvider();

    virtual void addInputListener(IInputListener *listener) override;
    virtual void removeInputListener(IInputListener *listener) override;

    virtual bool isKeyPressed(int keyCode) override;
    std::list<IInputListener *> m_listeners;

    bool isMouseButtonPressed(int button) override;
    glm::vec2 pointerOffset() override;
    glm::vec2 viewportSize() const override;
    void setFlightMode(bool enabled, float radius) override;

  private:
    void updateCapture();
    bool m_flightMode = false, m_captured = false;
    float m_pointerRadius = .22f;
    glm::dvec2 m_lastCursor{0};
    glm::vec2 m_pointer{0};

    // TODO this and the static methods should be extracted into their own class
    static void glfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfwCursorPositionCallback(GLFWwindow *window, double xpos, double ypos);
    static void glfwCursorEnterCallback(GLFWwindow *window, int entered);
    static void glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    GLFWwindow *m_window;

    static std::unordered_map<GLFWwindow *, CGlfwInputProvider *> s_instances;
};
