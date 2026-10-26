#include "MaterialDraw.h"

#include <algorithm>

#include <cassert>
#include <string>

#include <glm/ext.hpp>

#include "graphics/IScene.h"
#include "graphics/scene/CSceneQuery.h"
#include "graphics/ICamera.h"
#include "graphics/IWindow.h"

#include "resource/IResourceManager.h"
#include "graphics/camera/CStaticCamera.h"

#include "graphics/IGraphicsResourceManager.h"
#include "graphics/resource/CMaterial.h"
#include "graphics/resource/CMesh.h"
#include "graphics/resource/CTexture.h"
#include "graphics/resource/CShaderProgram.h"

#include "graphics/renderer/core/RendererCoreConfig.h"
#include "graphics/renderer/Draw.h"

#include "graphics/renderer/debug/RendererDebug.h"
#include <spdlog/spdlog.h>


void drawMaterialMesh(CMesh* mesh, const glm::mat4& translation, const glm::mat4& rotation,
	const glm::mat4& scale, CMaterial* material, const IGraphicsResourceManager& manager, CShaderProgram* shader)
{
	// Shared binding path for geometry, shadow, background and translucent draws.
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    shader->setActive();

    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    if (!mesh) { SPDLOG_ERROR("Cannot render a missing mesh."); return; }

    // Transformation matrices
    //shader->setUniform(translationMatrixUniformName, translation);
    shader->setUniform(rotationMatrixUniformName, rotation);
    //shader->setUniform(scaleMatrixUniformName, scale);
    shader->setUniform(modelMatrixUniformName, translation * rotation * scale);

    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

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
    shader->setUniform("alpha_mode",static_cast<int>(settings.alphaMode));
    shader->setUniform("alpha_cutoff",settings.alphaCutoff);
    shader->setUniform("opacity",settings.opacity);
    glFrontFace(scale[0][0]*scale[1][1]*scale[2][2]<0.f?GL_CW:GL_CCW);

    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    // Draw mesh
    // TODO Consider custom shader bindings for meshes
    ::draw(*mesh);
    glFrontFace(GL_CCW);

    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }
    // TODO Cleanup?
}
