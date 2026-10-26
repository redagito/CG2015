#include "graphics/resource/CTexture.h"
#include "graphics/resource/CShaderProgram.h"
#include "graphics/renderer/CFrameBuffer.h"
#include "graphics/renderer/CRenderBuffer.h"
#include <cstdio>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace
{
void check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
}
bool runRenderResourceTests()
{
    static_assert(!std::is_copy_constructible_v<CTexture>);
    static_assert(!std::is_copy_constructible_v<CFrameBuffer>);
    static_assert(!std::is_copy_constructible_v<CRenderBuffer>);
    static_assert(!std::is_copy_constructible_v<CShaderProgram>);
    try
    {
        std::vector<unsigned char> pixels(18);
        for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = static_cast<unsigned char>(i * 13);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
        CTexture rgb(pixels, 3, 2, EColorFormat::RGB24, false);
        GLint state = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &state);
        check(state == 8, "Texture upload changed unpack alignment");
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        std::vector<unsigned char> result(18);
        glGetTextureImage(rgb.getId(), 0, GL_RGB, GL_UNSIGNED_BYTE, static_cast<GLsizei>(result.size()), result.data());
        check(result == pixels, "Odd-width RGB upload corrupted rows");
        check(!rgb.init({1,2,3}, 10, 10, EColorFormat::GreyScale8), "Accepted a truncated upload");
        rgb.resize(3, 2);
        glGetTextureImage(rgb.getId(), 0, GL_RGB, GL_UNSIGNED_BYTE, static_cast<GLsizei>(result.size()), result.data());
        check(result == pixels, "Failed texture initialization corrupted the existing texture");
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        CTexture strip(std::vector<unsigned char>(24, 127), 1, 8, EColorFormat::RGB24, true);
        glGetTextureLevelParameteriv(strip.getId(), 3, GL_TEXTURE_HEIGHT, &state);
        check(state == 1, "One-pixel-wide texture is missing mipmaps");

        auto color = std::make_shared<CTexture>();
        check(color->init(8, 8, GL_RGB16F), "Color allocation failed");
        CFrameBuffer first, second;
        first.attach(color, GL_COLOR_ATTACHMENT0);
        first.setActive(GL_FRAMEBUFFER);
        auto other = std::make_shared<CTexture>();
        check(other->init(8, 8, GL_RGB16F), "Second color allocation failed");
        second.attach(other, GL_COLOR_ATTACHMENT0);
        second.getState();
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state);
        check(static_cast<GLuint>(state) == first.getId(), "Attachment/status query changed active framebuffer");
        auto depth = std::make_shared<CRenderBuffer>(8, 8, GL_DEPTH_COMPONENT24);
        first.attach(depth, GL_DEPTH_ATTACHMENT);
        first.resize(11, 7);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state);
        check(static_cast<GLuint>(state) == first.getId(), "Resize lost active framebuffer");
        check(glCheckNamedFramebufferStatus(first.getId(), GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Resized target is incomplete");
        first.attach(color, GL_COLOR_ATTACHMENT0);
        first.attach(color, GL_COLOR_ATTACHMENT0);
        glGetIntegerv(GL_DRAW_BUFFER1, &state);
        check(state == GL_NONE, "Reattachment duplicated draw buffers");
        CFrameBuffer sparse;
        sparse.attach(other, GL_COLOR_ATTACHMENT1);
        sparse.setActive(GL_FRAMEBUFFER);
        glGetIntegerv(GL_DRAW_BUFFER0, &state);
        check(state == GL_NONE, "Sparse attachment incorrectly maps shader output zero");
        glGetIntegerv(GL_DRAW_BUFFER1, &state);
        check(state == GL_COLOR_ATTACHMENT1, "Sparse attachment lost its output location");
        first.setActive(GL_FRAMEBUFFER);
        second.setActive(GL_READ_FRAMEBUFFER);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &state);
        check(static_cast<GLuint>(state) == first.getId(), "Read target changed draw target");
        CFrameBuffer::setDefaultActive();
        GLuint id = 0;
        {
            CRenderBuffer buffer(4, 4, GL_DEPTH_COMPONENT24);
            id = buffer.getId();
            check(glIsRenderbuffer(id), "Renderbuffer constructor did not allocate storage");
            buffer.resize(8, 8);
            check(buffer.getId() == id, "Renderbuffer resize replaced the object");
        }
        check(!glIsRenderbuffer(id), "Renderbuffer leaked on destruction");
        bool rejected = false;
        try { CFrameBuffer incomplete; incomplete.setActive(GL_FRAMEBUFFER); }
        catch (const std::runtime_error&) { rejected = true; }
        CFrameBuffer::setDefaultActive();
        check(rejected, "Incomplete framebuffer did not fail fast");
        std::puts("Render resource checks passed: alignment, mipmaps, resize, attachment state, ownership and completeness.");
        return true;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Render resource regression: %s\n", error.what());
        return false;
    }
}
