#include "BackgroundPass.h"
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


void BackgroundPass::render(const IScene& scene, const ICamera& camera, const IWindow& window, const IGraphicsResourceManager& manager)
{
    CStaticCamera all(camera.getView(),camera.getProjection(),camera.getPosition());
    all.getFeatureInfoForWrite().frustumCullingActive=false;
    CSceneQuery query; scene.getVisibleObjects(all,query);
    m_sun.visible=false; m_sun.color=glm::vec3(0); float strongest=0.f;
    while(query.hasNextDirectionalLight()) {
        glm::vec3 direction,color;float intensity;bool shadow;
        if(!scene.getDirectionalLight(query.getNextDirectionalLight(),direction,color,intensity,shadow) || glm::dot(direction,direction)<1e-8f) continue;
        float strength=intensity*glm::dot(color,glm::vec3(0.2126f,0.7152f,0.0722f));
        if(strength<=strongest) continue;
        strongest=strength; m_sun.color=color*intensity;
        glm::vec4 clip=camera.getProjection()*glm::vec4(glm::mat3(camera.getView())*-glm::normalize(direction),0.f);
        if(clip.w<=1e-6f) {m_sun.visible=false;continue;}
        m_sun.screen=glm::vec2(clip)/clip.w*0.5f+0.5f;
        m_sun.visible=glm::all(glm::greaterThanEqual(m_sun.screen,glm::vec2(0))) && glm::all(glm::lessThanEqual(m_sun.screen,glm::vec2(1)));
    }
    auto* shader=manager.getShaderProgram(m_backgroundShader);
    m_sceneColor.framebuffer.setActive(GL_FRAMEBUFFER);
    glViewport(0,0,window.getWidth(),window.getHeight());
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
    shader->setUniform("sun_active",static_cast<int>(m_sun.visible));shader->setUniform("sun_position",m_sun.screen);shader->setUniform("sun_color",m_sun.color);
    shader->setUniform("screen_width",static_cast<float>(window.getWidth()));shader->setUniform("screen_height",static_cast<float>(window.getHeight()));
    shader->setUniform("view",camera.getView());shader->setUniform("projection",camera.getProjection());
    bool found=false;
    while(query.hasNextObject()) {
        ResourceId meshId,materialId;glm::vec3 position,scale;glm::quat rotation;bool visible;
        if(!scene.getObject(query.getNextObject(),meshId,materialId,position,rotation,scale,visible)) continue;
        auto* material=manager.getMaterial(materialId);auto* mesh=manager.getMesh(meshId);
        if(!mesh || !material || !material->settings.background) continue;
        found=true;shader->setUniform("fullscreen",0);shader->setUniform("has_texture",static_cast<int>(material->hasDiffuse()));
        CTransformer transform;transform.setPosition(camera.getPosition()+position);transform.setRotation(rotation);transform.setScale(scale);
        drawMaterialMesh(mesh,transform.getTranslationMatrix(),transform.getRotationMatrix(),transform.getScaleMatrix(),material,manager,shader);
        break; // One explicit environment background, independent of light count.
    }
    if(!found) {
        shader->setUniform("fullscreen",1);shader->setUniform("has_texture",0);shader->setUniform("model",glm::mat4(1.f));
        ::draw(*manager.getMesh(m_fullscreen.quad));
    }
    glDepthMask(GL_TRUE);
}

bool BackgroundPass::init(IResourceManager& manager)
{
    m_backgroundShader = manager.loadShader("data/shader/deferred/background.ini");
    return m_backgroundShader != invalidResource;
}
