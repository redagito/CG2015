#include "CVertexBuffer.h"

#include <cassert>

#include "graphics/renderer/debug/RendererDebug.h"
#include <spdlog/spdlog.h>

CVertexBuffer::CVertexBuffer(const std::vector<float>& data, GLenum usage)
    : m_bufferId(0), m_valid(false), m_size((unsigned int)data.size()), m_usage(usage)
{
    if (data.empty())
    {
        SPDLOG_ERROR("Vertex buffer data is empty.");
        return;
    }
    // Create GL buffer resource
    glGenBuffers(1, &m_bufferId);
    // Unchecked bind
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
    // Set data
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), usage);
    setInactive();
    // TODO Check error
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return;
    }
    m_valid = true;
}

CVertexBuffer::~CVertexBuffer() { glDeleteBuffers(1, &m_bufferId); }

void CVertexBuffer::setActive() const
{
    assert(isValid());
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
}

void CVertexBuffer::setInactive() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

bool CVertexBuffer::isValid() const { return m_valid; }

GLuint CVertexBuffer::getId() const { return m_bufferId; }

unsigned int CVertexBuffer::getSize() const { return m_size; }

CVertexBuffer::CVertexBuffer(GLenum usage)
    : m_bufferId(0), m_valid(false), m_size(0), m_usage(usage)
{
    glGenBuffers(1, &m_bufferId);
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return;
    }
}

void CVertexBuffer::setData(const std::vector<float>& data)
{
    // Unchecked bind
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
    // Set data
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), m_usage);
    m_size = static_cast<unsigned int>(data.size());
    setInactive();
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return;
    }
    m_valid = true;
}
