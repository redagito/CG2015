#include "CFrameBuffer.h"

#include <cassert>
#include <algorithm>
#include <stdexcept>

#include "graphics/resource/CTexture.h"
#include "graphics/renderer/CRenderBuffer.h"

CFrameBuffer::CFrameBuffer() : m_fboId(0), m_valid(false) { init(); }

CFrameBuffer::~CFrameBuffer()
{
    if (m_valid)
    {
        glDeleteFramebuffers(1, &m_fboId);
    }
}

bool CFrameBuffer::init()
{
    GLuint bufferId;
    if (m_valid) return true;
    glCreateFramebuffers(1, &bufferId);
    glNamedFramebufferDrawBuffer(bufferId, GL_NONE);
    glNamedFramebufferReadBuffer(bufferId, GL_NONE);

    m_fboId = bufferId;
    m_valid = true;

    return m_valid;
}

bool CFrameBuffer::isValid() const { return m_valid; }

std::string CFrameBuffer::getState()
{
    assert(m_valid);

    GLenum state = glCheckNamedFramebufferStatus(m_fboId, GL_FRAMEBUFFER);

    switch (state)
    {
    case GL_FRAMEBUFFER_UNDEFINED:
        return "The current FBO binding is 0, but no default framebuffer exists";
        break;
    case GL_FRAMEBUFFER_COMPLETE:
        return "A user-defined FBO is bound and is complete. OK to render";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "One of the buffers enabled for rendering is incomplete";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "No buffers are attached to the FBO.";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "One of the buffer attachments enabled for rendering does not have a buffer "
               "attached";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "One of the buffer attachments enabled for reading does not have a buffer attached";
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "The combination of internal buffer formats is not supported";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "The number of samples or the value for TEXTURE_FIXED_SAMPLE_LOCATIONS for all "
               "buffers does not match";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "Not all color attachments are layered textures or bound to the same target";
    }
    return "Invalid state encountered";
}

void CFrameBuffer::setActive(GLenum target)
{
    assert(m_valid);
    glBindFramebuffer(target, m_fboId);
    if (glCheckNamedFramebufferStatus(m_fboId, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer: " + getState());
}

void CFrameBuffer::setInactive(GLenum target) { glBindFramebuffer(target, 0); }

void CFrameBuffer::resize(unsigned int width, unsigned int height)
{
    if (!width || !height) return;
    for (auto& entry : m_textures)
    {
        entry.second->resize(width, height);
    }
    for (auto& entry : m_renderBuffers)
        entry.second->resize(width, height);
}

void CFrameBuffer::attach(const std::shared_ptr<CTexture>& texture, GLenum attachment, ETextureSemantic semantic)
{
	// Bind
    assert(m_valid);

    // Attach
    glNamedFramebufferTexture(m_fboId, attachment, texture->getId(), 0);
    // Add color attachments to draw buffers
    if (attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT &&
        attachment != GL_DEPTH_STENCIL_ATTACHMENT)
    {
        const size_t index = attachment - GL_COLOR_ATTACHMENT0;
        if (m_drawBuffers.size() <= index) m_drawBuffers.resize(index + 1, GL_NONE);
        m_drawBuffers[index] = attachment;
        glNamedFramebufferDrawBuffers(m_fboId, static_cast<GLsizei>(m_drawBuffers.size()), m_drawBuffers.data());
        glNamedFramebufferReadBuffer(m_fboId, attachment);
    }
    m_renderBuffers.erase(attachment);
    m_textures[attachment] = texture;
	if (semantic != ETextureSemantic::Unknown)
	{
		m_texturesBySemantic[semantic] = texture;
	}

}

void CFrameBuffer::attach(const std::shared_ptr<CTexture>& texture, GLenum attachment)
{
	ETextureSemantic semantic = ETextureSemantic::Unknown;
	// On depth attachment automatically set semantic
    if (attachment == GL_DEPTH_ATTACHMENT)
	{
		semantic = ETextureSemantic::Depth;
	}
	attach(texture, attachment, semantic);
}

void CFrameBuffer::setDefaultActive()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CFrameBuffer::attach(const std::shared_ptr<CRenderBuffer>& renderBuffer, GLenum attachment)
{
    // Bind
    assert(m_valid);

    // Attach
    glNamedFramebufferRenderbuffer(m_fboId, attachment, GL_RENDERBUFFER, renderBuffer->getId());
    // Add color attachments to draw buffers
    if (attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT &&
        attachment != GL_DEPTH_STENCIL_ATTACHMENT)
    {
        const size_t index = attachment - GL_COLOR_ATTACHMENT0;
        if (m_drawBuffers.size() <= index) m_drawBuffers.resize(index + 1, GL_NONE);
        m_drawBuffers[index] = attachment;
        glNamedFramebufferDrawBuffers(m_fboId, static_cast<GLsizei>(m_drawBuffers.size()), m_drawBuffers.data());
        glNamedFramebufferReadBuffer(m_fboId, attachment);
    }
    m_textures.erase(attachment);
    m_renderBuffers[attachment] = renderBuffer;

}

std::shared_ptr<CTexture> CFrameBuffer::getTexture(ETextureSemantic semantic)
{
	// TODO Slow, fix this
	if (m_texturesBySemantic.count(semantic) == 0)
	{
		return nullptr;
	}
	return m_texturesBySemantic.at(semantic);
}
