#include "CRenderBuffer.h"

CRenderBuffer::CRenderBuffer() = default;
CRenderBuffer::CRenderBuffer(unsigned int width, unsigned int height, GLenum format)
{
    init(width, height, format);
}
CRenderBuffer::~CRenderBuffer()
{
    if (m_bufferId) glDeleteRenderbuffers(1, &m_bufferId);
}
bool CRenderBuffer::init(unsigned int width, unsigned int height, GLenum format)
{
    if (!width || !height) return false;
    if (!m_bufferId) glCreateRenderbuffers(1, &m_bufferId);
    if (format == GL_DEPTH_COMPONENT) format = GL_DEPTH_COMPONENT24;
    glNamedRenderbufferStorage(m_bufferId, format, width, height);
    m_width = width;
    m_height = height;
    m_format = format;
    m_init = true;
    return true;
}
bool CRenderBuffer::resize(unsigned int width, unsigned int height)
{
    if (!m_init || (m_width == width && m_height == height)) return false;
    return init(width, height, m_format);
}
GLuint CRenderBuffer::getId() const { return m_bufferId; }
GLenum CRenderBuffer::getFormat() const { return m_format; }
void CRenderBuffer::setActive() const { glBindRenderbuffer(GL_RENDERBUFFER, m_bufferId); }
