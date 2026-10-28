#include "debug/Logging.h"
#include "graphics/renderer/core/RendererCoreConfig.h"
#ifdef GL_KHR_debug
#error "Generate glad for core OpenGL only; debug output is already in core."
#endif
#include "graphics/renderer/core/CIndexBuffer.h"
#include "graphics/renderer/core/CVertexBuffer.h"
#include "graphics/window/CGlfwWindow.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <string_view>

bool runCombatEffectsGraphicsTests(const CGlfwWindow &window);
bool runCombatGraphicsTests(const CGlfwWindow &window);
bool runPostProcessTests(const CGlfwWindow &window);
bool runRenderResourceTests();
bool runShaderTests(const CGlfwWindow &window);
bool runRemainingRenderingTests(const CGlfwWindow &window);
bool runUiTests();
bool runLoadingScreenTests(CGlfwWindow &window);

int main(int argc, char **argv)
{
    logging::initialize();
#ifdef _WIN32
    // The intentional death test must never display a CRT reporting dialog.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    CGlfwWindow window;
    if (!window.init(65, 65, "CG2015 graphics test", false))
        return 1;
    if (argc > 1 && std::string_view(argv[1]) == "--effects")
        return runCombatEffectsGraphicsTests(window) ? 0 : 12;
    if (argc > 1 && std::string_view(argv[1]) == "--combat")
        return runCombatGraphicsTests(window) ? 0 : 11;
    if (argc > 1 && std::string_view(argv[1]) == "--post-process")
        return runPostProcessTests(window) ? 0 : 5;
    if (argc > 1 && std::string_view(argv[1]) == "--resources")
        return runRenderResourceTests() ? 0 : 6;
    if (argc > 1 && std::string_view(argv[1]) == "--shaders")
        return runShaderTests(window) ? 0 : 7;
    if (argc > 1 && std::string_view(argv[1]) == "--remaining")
        return runRemainingRenderingTests(window) ? 0 : 8;
    if (argc > 1 && std::string_view(argv[1]) == "--ui")
        return runUiTests() ? 0 : 9;
    if (argc > 1 && std::string_view(argv[1]) == "--loading")
        return runLoadingScreenTests(window) ? 0 : 10;
    if (argc > 1 && std::string_view(argv[1]) == "--inject-error")
    {
        glEnable(0xFFFFFFFF); // Must terminate synchronously through the real driver callback.
        return 0;
    }
    GLint flags = 0, profile = 0, vao = -1;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT) || !(profile & GL_CONTEXT_CORE_PROFILE_BIT) ||
        vao != 0 || !glIsEnabled(GL_DEBUG_OUTPUT_SYNCHRONOUS))
        return 2;
    {
        CIndexBuffer indices({0, 1, 2}); // Upload must work with VAO zero in core profile.
        CVertexBuffer vertices;
        vertices.setData({0.f, 1.f, 2.f, 3.f, 4.f, 5.f});
        if (!indices.isValid() || vertices.getSize() != 6)
            return 3;
    }
    if (glGetError() != GL_NO_ERROR)
        return 4;
    std::puts("OpenGL core/debug/DSA checks passed.");
    return 0;
}
