#include "MaterialDraw.h"

#include <algorithm>

#include <cassert>
#include <string>

#include <glm/ext.hpp>

#include "graphics/ICamera.h"
#include "graphics/IScene.h"
#include "graphics/IWindow.h"
#include "graphics/scene/CSceneQuery.h"

#include "graphics/camera/CStaticCamera.h"
#include "resource/IResourceManager.h"

#include "graphics/IGraphicsResourceManager.h"
#include "graphics/resource/CMaterial.h"
#include "graphics/resource/CMesh.h"
#include "graphics/resource/CShaderProgram.h"
#include "graphics/resource/CTexture.h"

#include "graphics/renderer/Draw.h"
#include "graphics/renderer/core/RendererCoreConfig.h"

#include "graphics/renderer/debug/RendererDebug.h"
#include <spdlog/spdlog.h>

void bindMaterial(CMaterial *material, const IGraphicsResourceManager &manager,
                  CShaderProgram *shader)
{
    shader->setActive();
    // Send material textures to shader
    if (material && material->hasDiffuse())
    {
        material->getDiffuse()->setActive(diffuseTextureUnit);
    }
    else
    {
        manager.getDefaultDiffuseTexture()->setActive(diffuseTextureUnit);
    }
    shader->setUniform(diffuseTextureUniformName, diffuseTextureUnit);

    if (material && material->hasNormal())
    {
        material->getNormal()->setActive(normalTextureUnit);
    }
    else
    {
        // Use default texture
        manager.getDefaultNormalTexture()->setActive(normalTextureUnit);
    }
    shader->setUniform(normalTextureUniformName, normalTextureUnit);

    if (material && material->hasSpecular())
    {
        material->getSpecular()->setActive(specularTextureUnit);
    }
    else
    {
        // Use default texture
        manager.getDefaultSpecularTexture()->setActive(specularTextureUnit);
    }
    shader->setUniform(specularTextureUniformName, specularTextureUnit);

    if (material && material->hasGlow())
    {
        material->getGlow()->setActive(glowTextureUnit);
    }
    else
    {
        // Use default texture
        manager.getDefaultGlowTexture()->setActive(glowTextureUnit);
    }
    shader->setUniform(glowTextureUniformName, glowTextureUnit);

    // Alpha mode is selected consistently by geometry and shadow shaders.
    if (material && material->hasAlpha())
    {
        material->getAlpha()->setActive(alphaTextureUnit);
    }
    else
    {
        // Use default texture
        manager.getDefaultAlphaTexture()->setActive(alphaTextureUnit);
    }
    shader->setUniform(alphaTextureUniformName, alphaTextureUnit);
    const auto settings = material ? material->settings : SMaterialSettings{};
    shader->setUniform("alpha_mode", static_cast<int>(settings.alphaMode));
    shader->setUniform("alpha_cutoff", settings.alphaCutoff);
    shader->setUniform("opacity", settings.opacity);
    const auto &accents = settings.emissiveAccents;
    shader->setUniform("accent_color_strength", glm::vec4(accents.color, accents.strength));
    shader->setUniform("accent_hull_strips",
                       glm::vec4(accents.hullStripOffset, accents.hullStripWidth,
                                 accents.hullZRange.x, accents.hullZRange.y));
    shader->setUniform("accent_engines",
                       glm::vec4(accents.engineZMax, accents.engineHalfWidth, accents.engineCenterY,
                                 accents.engineHalfHeight));
}
void drawMaterialMesh(CMesh *mesh, const glm::mat4 &translation, const glm::mat4 &rotation,
                      const glm::mat4 &scale, CMaterial *material,
                      const IGraphicsResourceManager &manager, CShaderProgram *shader)
{
    if (!mesh)
    {
        SPDLOG_ERROR("Cannot render a missing mesh.");
        return;
    }
    bindMaterial(material, manager, shader);
    shader->setUniform("instanced", 0);
    shader->setUniform(rotationMatrixUniformName, rotation);
    shader->setUniform(modelMatrixUniformName, translation * rotation * scale);
    glFrontFace(scale[0][0] * scale[1][1] * scale[2][2] < 0 ? GL_CW : GL_CCW);
    ::draw(*mesh);
    glFrontFace(GL_CCW);
}
