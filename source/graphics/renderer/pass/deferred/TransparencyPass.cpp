#include "TransparencyPass.h"
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


void TransparencyPass::render(const IScene& scene,const ICamera& camera,const IWindow& window,const IGraphicsResourceManager& manager)
{
    struct Item {CMesh* mesh;CMaterial* material;glm::vec3 position,scale;glm::quat rotation;float distance;};
    CSceneQuery query;scene.getVisibleObjects(camera,query);std::vector<Item> objects;
    while(query.hasNextObject()) {
        ResourceId meshId,materialId;glm::vec3 position,scale;glm::quat rotation;bool visible;
        if(!scene.getObject(query.getNextObject(),meshId,materialId,position,rotation,scale,visible)) continue;
        auto* material=manager.getMaterial(materialId);auto* mesh=manager.getMesh(meshId);
        if(mesh && material && !material->settings.background && material->settings.alphaMode==AlphaMode::Blend)
            objects.push_back({mesh,material,position,scale,rotation,glm::dot(position-camera.getPosition(),position-camera.getPosition())});
    }
    if(objects.empty()) return;
    std::stable_sort(objects.begin(),objects.end(),[](const auto& a,const auto& b){return a.distance>b.distance;});
    auto* shader=manager.getShaderProgram(m_transparencyShader);
    shader->setUniform("view",camera.getView());shader->setUniform("projection",camera.getProjection());shader->setUniform("camera_position",camera.getPosition());
    shader->setUniform("normal_mapping_enabled",static_cast<int>(camera.getFeatureInfo().normalMappingActive));
    glm::vec3 ambient;float intensity;scene.getAmbientLight(ambient,intensity);shader->setUniform("ambient_light",ambient*intensity);
    struct Point {SceneObjectId id;glm::vec3 position,color;float radius;};
    struct Directional {SceneObjectId id;glm::vec3 direction,color;};
    std::vector<Point> points;std::vector<Directional> directions;
    while(query.hasNextPointLight()) {
        auto id=query.getNextPointLight();glm::vec3 position,color;float radius,power;bool shadow;
        if(scene.getPointLight(id,position,radius,color,power,shadow)&&radius>0.f&&power>0.f) points.push_back({id,position,color*power,radius});
    }
    while(query.hasNextDirectionalLight()) {
        auto id=query.getNextDirectionalLight();glm::vec3 direction,color;float power;bool shadow;
        if(scene.getDirectionalLight(id,direction,color,power,shadow)&&power>0.f&&glm::dot(direction,direction)>1e-8f) directions.push_back({id,glm::normalize(direction),color*power});
    }
    shader->setUniform("shadow_cube",5);shader->setUniform("shadow_map",6);
    m_shadows.pointCube->setActive(5);m_shadows.directionalDepth->setActive(6);
    m_sceneColor.framebuffer.setActive(GL_FRAMEBUFFER);
    glViewport(0,0,window.getWidth(),window.getHeight());glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glDisable(GL_CULL_FACE);
    for(const auto& item:objects) {
        CTransformer transform;transform.setPosition(item.position);transform.setRotation(item.rotation);transform.setScale(item.scale);
        const auto render=[&] {drawMaterialMesh(item.mesh,transform.getTranslationMatrix(),transform.getRotationMatrix(),transform.getScaleMatrix(),item.material,manager,shader);};
        // Complete each object's ambient + light contributions before compositing
        // the next nearer surface, preserving standard back-to-front alpha order.
        glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
        shader->setUniform("light_kind",0);shader->setUniform("shadows_enabled",0);render();
        glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE,GL_ZERO,GL_ONE);
        for(const auto& light:points) {
            shader->setUniform("light_kind",1);shader->setUniform("light_position",light.position);shader->setUniform("light_color",light.color);shader->setUniform("light_radius",light.radius);
            const bool shadows=m_shadows.renderedPoint.contains(light.id);
            shader->setUniform("shadows_enabled",static_cast<int>(shadows));if(shadows) m_shadows.point.at(light.id).cube->setActive(5);
            render();
        }
        for(const auto& light:directions) {
            shader->setUniform("light_kind",2);shader->setUniform("light_direction",light.direction);shader->setUniform("light_color",light.color);
            const bool shadows=m_shadows.renderedDirectional.contains(light.id);
            shader->setUniform("shadows_enabled",static_cast<int>(shadows));
            if(shadows) {const auto& cached=m_shadows.directional.at(light.id);cached.depth->setActive(6);shader->setUniform("shadow_view_projection_bias",cached.projectionBias);shader->setUniform("shadow_bias",cached.bias);}
            render();
        }
    }    glDepthMask(GL_TRUE);glDisable(GL_BLEND);
}

bool TransparencyPass::init(IResourceManager& manager)
{
    m_transparencyShader = manager.loadShader("data/shader/deferred/transparency.ini");
    return m_transparencyShader != invalidResource;
}
