#pragma once

#include <memory>

#include "CTexture.h"
#include "CShaderProgram.h"

/**
 * \brief Material class.
 *
 * Stores visual properties of a surface.
 */
class CMaterial
{
public:
    CMaterial(const CTexture* diffuse, const CTexture* normal, const CTexture* specular,
              const CTexture* glow, const CTexture* alpha);

    bool init(const CTexture* diffuse, const CTexture* normal, const CTexture* specular,
              const CTexture* glow, const CTexture* alpha);

    bool hasDiffuse() const;
    bool hasNormal() const;
    bool hasSpecular() const;
    bool hasGlow() const;
    bool hasAlpha() const;

    const CTexture* getDiffuse() const;
    const CTexture* getNormal() const;
    const CTexture* getSpecular() const;
    const CTexture* getGlow() const;
    const CTexture* getAlpha() const;

private:
    const CTexture* m_diffuseTexture = nullptr;  /**< Base color. */
	const CTexture* m_normalTexture = nullptr;   /**< Normal map. */
	const CTexture* m_specularTexture = nullptr; /**< Specular highlights. */
	const CTexture* m_glowTexture = nullptr;     /**< Glow map. */
	const CTexture* m_alphaTexture = nullptr;    /**< Alpha map. */
};
