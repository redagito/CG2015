#pragma once

#include <vector>
#include <cstdint>

#include "CoreConfig.h"

/**
* \brief Represents an index buffer.
*/
class CIndexBuffer
{
public:
    CIndexBuffer(const std::vector<uint32_t>& indices, GLenum usage = GL_STATIC_DRAW);

	~CIndexBuffer();

	/**
	* \brief Disable copy and assign.
	*/
	CIndexBuffer(const CIndexBuffer& rhs) = delete;
	CIndexBuffer& operator=(const CIndexBuffer& rhs) = delete;



    /**
    * \brief Sets the VBO as active object.
    * Binds the VBO to GL_INDEX_ARRAY.
    */
    void setActive() const;
    void setInactive() const;

    /**
    * \brief Returns buffer validity.
    */
    bool isValid() const;

    /**
    * \brief Access to internal buffer id.
    * TODO Should/can this be hidden?
    */
    GLuint getId() const;

    unsigned int getSize() const;

private:
	GLuint m_bufferId = 0; /**< Internal GL id. */
	unsigned int m_size = 0; 
	bool m_valid = false;
};