#pragma once

#include <vector>

#include "graphics/core/CoreConfig.h"
#include "graphics/api/PrimitiveType.h"
#include "graphics/api/ColorFormat.h"

/**
 * \brief Texture class.
 */
class CTexture
{
public:
    CTexture();

    /**
     * \brief Create from image data.
     */
    CTexture(const std::vector<unsigned char>& imageData, unsigned int width, unsigned int height,
             EColorFormat format, bool createMipmaps = true);

    /**
     * \brief Creates empty texture.
     */
    CTexture(unsigned int width, unsigned int height, EColorFormat format,
             bool createMipmaps = true);

    ~CTexture();

    /**
    * \brief Initializes texture data.
    */
    bool init(const std::vector<unsigned char>& imageData, unsigned int width, unsigned int height,
              EColorFormat format, bool createMipmaps = true);

    bool init(unsigned int width, unsigned int height, GLint format);

    /**
    * \brief Resizes texture.
    */
    void resize(unsigned int width, unsigned int height);

    /**
    * \brief Returns texture id.
    */
    GLuint getId() const;

    /**
    * \brief Returns texture validity.
    */
    bool isValid() const;

    /**
    * \bref Sets texture active as texture unit.
    */
    void setActive(GLint textureUnit) const;

protected:
    bool init(const std::vector<unsigned char>& imageData, unsigned int width, unsigned int height,
              GLint format, bool createMipmaps);

private:
    bool m_valid = false;
    bool m_hasMipmaps = false;
    GLuint m_textureId = 0;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    GLint m_format = 0;
    GLenum m_externalFormat = 0;
};
