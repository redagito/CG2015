#include <cassert>

#include "CGlfwWindow.h"

#include "graphics/renderer/core/RendererCoreConfig.h"
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include "graphics/renderer/debug/RendererDebug.h"

std::unordered_map<GLFWwindow*, CGlfwWindow*> CGlfwWindow::s_windows; /**< Window mapping. */

CGlfwWindow::CGlfwWindow() : m_window(nullptr), m_width(0), m_height(0), m_mouseCaptured(false)
{
    return;
}

CGlfwWindow::~CGlfwWindow()
{
    // Remove mapping
    s_windows.erase(m_window);
    if (m_window) glfwDestroyWindow(m_window);
    if (s_windows.empty()) glfwTerminate();
}

bool CGlfwWindow::init(unsigned int width, unsigned int height, const std::string& name, bool visible)
{
    glfwSetErrorCallback([](int code, const char* message) {
        SPDLOG_ERROR("GLFW error {}: {}", code, message);
    });
    if (!glfwInit())
    {
        SPDLOG_ERROR("Failed to initialize GLFW.");
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window from parameters
	SPDLOG_INFO("Creating OpenGL 4.6 core debug context.");
    m_window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
    if (m_window == nullptr)
    {
        SPDLOG_ERROR("Failed to create GLFW window.");
        glfwTerminate();
        return false;
    }

    int framebufferWidth;
    int framebufferHeight;

    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

    m_width = framebufferWidth;
    m_height = framebufferHeight;

    glfwMakeContextCurrent(m_window);
    glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);

    // glad is generated for OpenGL 4.6 core and resolves functions through GLFW.
    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
    if (version == 0 || !GLAD_GL_VERSION_4_6 || !initializeOpenGLDebug())
    {
        SPDLOG_ERROR("Failed to initialize OpenGL 4.6 core debug support.");
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        if (s_windows.empty()) glfwTerminate();
        return false;
    }
    glfwSwapInterval(1);

    // Add mapping
    s_windows[m_window] = this;

    // Set window resize callback
    glfwSetFramebufferSizeCallback(m_window, &CGlfwWindow::resizeCallback);

    return true;
}

void CGlfwWindow::setWidth(unsigned int width) { m_width = width; }

void CGlfwWindow::setHeight(unsigned int height) { m_height = height; }

unsigned int CGlfwWindow::getWidth() const { return m_width; }

unsigned int CGlfwWindow::getHeight() const { return m_height; }

void CGlfwWindow::setActive() const { glfwMakeContextCurrent(m_window); }

bool CGlfwWindow::isOpen() const { return !glfwWindowShouldClose(m_window); }

void CGlfwWindow::swapBuffer() { glfwSwapBuffers(m_window); }

void CGlfwWindow::toggleMouseCapture()
{
    // Set capture mode
    if (m_mouseCaptured)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    // Toggle
    m_mouseCaptured = !m_mouseCaptured;
}

GLFWwindow* CGlfwWindow::getGlfwHandle() const { return m_window; }

void CGlfwWindow::addListener(IGlfwWindowListener* l) { m_listeners.insert(l); }

void CGlfwWindow::removeListener(IGlfwWindowListener* l) { m_listeners.erase(l); }

void CGlfwWindow::resizeCallback(GLFWwindow* window, int width, int height)
{
    // Minimized windows have zero-sized framebuffers.
    if (width <= 0 || height <= 0) return;
    if (s_windows.count(window) != 0)
    {
        CGlfwWindow* win = s_windows.at(window);
        win->handleResize(width, height);
    }
}

void CGlfwWindow::handleResize(int width, int height)
{
    setWidth(width);
    setHeight(height);

    for (auto& l : m_listeners)
    {
        l->handleResizeEvent(width, height);
    }
}
