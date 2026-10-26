#include "CIndexBuffer.h"

#include <cassert>

#include "graphics/renderer/debug/RendererDebug.h"
#include <spdlog/spdlog.h>

CIndexBuffer::CIndexBuffer(const std::vector<unsigned int>& indices, GLenum usage)
    : m_bufferId(0), m_size(0), m_valid(false)
{
    if (indices.empty())
    {
        SPDLOG_ERROR("Index buffer data is empty.");
        return;
    }
    glCreateBuffers(1, &m_bufferId);
    // Direct state access does not mutate a VAO's element-buffer binding.
    glNamedBufferData(m_bufferId, indices.size() * sizeof(unsigned int), indices.data(),
                 usage);
    m_size = (unsigned int)indices.size();
    // TODO Check error
    // Unbind

    // Error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return;
    }
    m_valid = true;
}

CIndexBuffer::~CIndexBuffer() { glDeleteBuffers(1, &m_bufferId); }

void CIndexBuffer::setActive() const
{
    assert(isValid());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferId);
}

void CIndexBuffer::setInactive() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

bool CIndexBuffer::isValid() const { return m_valid; }

GLuint CIndexBuffer::getId() const { return m_bufferId; }

unsigned int CIndexBuffer::getSize() const { return m_size; }
