#include "graphics/renderer/CDeferredRenderer.h"
#include "graphics/renderer/Draw.h"
#include "graphics/renderer/pass/deferred/MaterialDraw.h"
#include "graphics/renderer/debug/RendererDebug.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/resource/CModel.h"
#include "graphics/scene/CScene.h"
#include "graphics/scene/CSceneQuery.h"
#include "graphics/camera/CCamera.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include <glm/ext.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <lodepng.h>

namespace {
void require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
class Probe:public CDeferredRenderer {
public:
    using CDeferredRenderer::draw;
    using CDeferredRenderer::geometryPass;
    using CDeferredRenderer::lightPass;
    using CDeferredRenderer::illuminationPass;
    using CDeferredRenderer::backgroundPass;
    using CDeferredRenderer::transparencyPass;
    using CDeferredRenderer::godRayPass1;
};
}

bool runRemainingRenderingTests(const CGlfwWindow& window)
{
    try {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        auto gray=resources->createImage({128,128,128},1,1,EColorFormat::RGB24);
        auto white=resources->createImage({255,255,255},1,1,EColorFormat::RGB24);
        auto blackAlpha=resources->createImage({0},1,1,EColorFormat::GreyScale8);
        auto whiteAlpha=resources->createImage({255},1,1,EColorFormat::GreyScale8);
        const std::vector<float> vertices={-1,-1,0, 1,-1,0, 0,1,0};
        const std::vector<float> normals={0,0,1, 0,0,1, 0,0,1};
        auto meshId=resources->createMesh(vertices,{},normals,std::vector<float>(6,0),EPrimitiveType::Triangle);
        auto materialId=resources->createMaterial(gray,invalidResource,invalidResource,invalidResource,invalidResource);
        auto modelId=resources->createModel(meshId,materialId);
        CGraphicsResourceManager graphics;
        resources->addResourceListener(&graphics); // All CPU resources already exist.
        require(graphics.getMesh(meshId)&&graphics.getModel(modelId),"Late resource replay lost a mesh/model");
        {
            CScene scene(&graphics);auto id=scene.createObject(modelId,glm::vec3(0),glm::quat(1,0,0,0),glm::vec3(1));
            ResourceId mesh,material;glm::vec3 position,scale;glm::quat rotation;bool visible;
            require(scene.getObject(id,mesh,material,position,rotation,scale,visible)&&mesh==meshId&&material==materialId,"Model handle did not resolve draw resources");
        }
        GLint internal=0;
        glGetTextureLevelParameteriv(graphics.getMaterial(materialId)->getDiffuse()->getId(),0,GL_TEXTURE_INTERNAL_FORMAT,&internal);
        require(internal==GL_SRGB8,"Base color was not uploaded as sRGB");
        glGetTextureLevelParameteriv(graphics.getTexture(gray)->getId(),0,GL_TEXTURE_INTERNAL_FORMAT,&internal);
        require(internal==GL_RGB8,"A data-map upload was incorrectly converted to sRGB");
        auto image=[&](GLint format) {auto value=std::make_shared<CTexture>();require(value->init(window.getWidth(),window.getHeight(),format),"Target allocation failed");return value;};
        auto color=image(GL_RGBA16F),normal=image(GL_RGBA16F),depth=image(GL_DEPTH_COMPONENT24);
        CFrameBuffer target;target.attach(color,GL_COLOR_ATTACHMENT0);target.attach(normal,GL_COLOR_ATTACHMENT1);target.attach(depth,GL_DEPTH_ATTACHMENT);
        auto read=[&](GLenum attachment=GL_COLOR_ATTACHMENT0) {std::array<float,4> value{};glReadBuffer(attachment);glReadPixels(window.getWidth()/2,window.getHeight()/2,1,1,GL_RGBA,GL_FLOAT,value.data());for(auto v:value) require(std::isfinite(v),"Non-finite output");return value;};
        auto bind=[&] {target.setActive(GL_FRAMEBUFFER);glViewport(0,0,window.getWidth(),window.getHeight());glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_FRAMEBUFFER_SRGB);glDepthMask(GL_TRUE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);};
        auto* mesh=graphics.getMesh(meshId);
        const std::string vertex="#version 460 core\nlayout(location=0) in vec3 position;void main(){gl_Position=vec4(position,1);}";
        const std::string sample="#version 460 core\nuniform sampler2D scene_texture;layout(location=0) out vec4 color;void main(){color=texture(scene_texture,vec2(0.5));}";
        auto vertexId=resources->createString(vertex),fragmentId=resources->createString(sample);
        auto programId=resources->createShader(vertexId,invalidResource,invalidResource,invalidResource,fragmentId);
        auto* program=graphics.getShaderProgram(programId);
        bind();graphics.getMaterial(materialId)->getDiffuse()->setActive(0);program->setUniform("scene_texture",0);::draw(*mesh);
        const float decoded=read()[0];require(std::abs(decoded-0.21586f)<0.002f,"sRGB input was not decoded once");
        auto displayId=resources->loadShader("data/shader/display_pass.ini");auto* display=graphics.getShaderProgram(displayId);
        // Use a separate output so the transfer test cannot feed back into its input.
        auto encoded=image(GL_RGBA16F);CFrameBuffer encoding;encoding.attach(encoded,GL_COLOR_ATTACHMENT0);encoding.setActive(GL_FRAMEBUFFER);
        color->setActive(0);display->setUniform("scene_texture",0);display->setUniform("screen_width",static_cast<float>(window.getWidth()));display->setUniform("screen_height",static_cast<float>(window.getHeight()));display->setUniform("encode_output",1);::draw(*mesh);
        require(std::abs(read()[0]-128.f/255.f)<0.003f,"Final sRGB conversion did not round-trip");
        bind();color->setActive(0);program->setUniform("scene_texture",0);
        bool rejected=false;try {validateFramebufferFeedback();} catch(const std::runtime_error&){rejected=true;}
#ifdef CG2015_VALIDATE_PASSES
        require(rejected,"A real sampler/draw attachment alias escaped validation");
#endif
        graphics.getTexture(gray)->setActive(0);
        require(resources->updateString(fragmentId,"#version 460 core\nlayout(location=0) out vec4 color;void main(){color=vec4(0.25,0.5,0.75,1);}"),"Shader update failed");
        require(graphics.getShaderProgram(programId)==program,"Shader reload invalidated the program wrapper pointer");
        program->setActive();::draw(*mesh);require(std::abs(read()[2]-0.75f)<0.002f,"Changed shader did not relink");
        require(resources->reloadShader(displayId),"File-backed shader reload failed");
        program->setActive();GLint before=0,after=0;glGetIntegerv(GL_CURRENT_PROGRAM,&before);glUseProgram(0);program->setActive();glGetIntegerv(GL_CURRENT_PROGRAM,&after);require(before==after&&after!=0,"Raw GL state invalidated shader binding tracking");

        Probe renderer;require(renderer.init(*resources),"Renderer fixture failed to initialize");
        auto masked=resources->createMaterial(white,invalidResource,invalidResource,invalidResource,blackAlpha);
        SMaterialSettings mask;mask.alphaMode=AlphaMode::Mask;resources->setMaterialSettings(masked,mask);
        auto geometryId=resources->loadShader("data/shader/deferred/geometry_pass.ini");auto* geometry=graphics.getShaderProgram(geometryId);
        const auto identity=glm::mat4(1.f);
        auto maskedDraw=[&](CShaderProgram* shader) {shader->setUniform("view",identity);shader->setUniform("projection",identity);drawMaterialMesh(mesh,identity,identity,identity,graphics.getMaterial(masked),graphics,shader);};
        bind();glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);maskedDraw(geometry);require(read()[0]==0.f,"Masked geometry filled a transparent texel");
        auto shadowId=resources->loadShader("data/shader/shadow_map_pass.ini");auto* shadow=graphics.getShaderProgram(shadowId);
        glEnable(GL_DEPTH_TEST);glClear(GL_DEPTH_BUFFER_BIT);maskedDraw(shadow);float z=0;glReadPixels(window.getWidth()/2,window.getHeight()/2,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&z);require(z==1.f,"Masked texel cast a solid shadow");
        auto opaqueMask=resources->createMaterial(white,invalidResource,invalidResource,invalidResource,whiteAlpha);resources->setMaterialSettings(opaqueMask,mask);
        shadow->setUniform("view",identity);shadow->setUniform("projection",identity);drawMaterialMesh(mesh,identity,identity,identity,graphics.getMaterial(opaqueMask),graphics,shadow);
        glReadPixels(window.getWidth()/2,window.getHeight()/2,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&z);require(std::abs(z-0.5f)<0.001f,"Opaque cutout texel failed to cast a shadow");

        bind();auto normalInput=image(GL_RGBA16F),depthInput=image(GL_DEPTH_COMPONENT24);float surface[]={0,0,1,0},d=0.5f;
        glClearTexImage(normalInput->getId(),0,GL_RGBA,GL_FLOAT,surface);glClearTexImage(depthInput->getId(),0,GL_DEPTH_COMPONENT,GL_FLOAT,&d);
        auto lightId=resources->loadShader("data/shader/deferred/directional_light_pass.ini");auto* light=graphics.getShaderProgram(lightId);
        depthInput->setActive(0);normalInput->setActive(1);light->setUniform("depth_texture",0);light->setUniform("normal_specular_texture",1);light->setUniform("shadow_map",2);depthInput->setActive(2);depthInput->setParameter(GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
        light->setUniform("shadows_enabled",0);light->setUniform("inverse_view_projection",identity);light->setUniform("shadow_view_projection_bias",identity);light->setUniform("light_direction",glm::vec3(0,0,-1));light->setUniform("camera_position",glm::vec3(0,0,2));light->setUniform("light_color",glm::vec3(1,0.5f,0));light->setUniform("light_intensity",1.f);light->setUniform("screen_width",static_cast<float>(window.getWidth()));light->setUniform("screen_height",static_cast<float>(window.getHeight()));
        // The depth sampler must not share a compare-mode texture with sampler2DShadow.
        auto compare=image(GL_DEPTH_COMPONENT24);compare->setParameter(GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);compare->setActive(2);depthInput->setParameter(GL_TEXTURE_COMPARE_MODE,GL_NONE);
        ::draw(*mesh);require(read(GL_COLOR_ATTACHMENT1)[0]==0.f,"A black specular map produced a highlight");
        surface[3]=1;glClearTexImage(normalInput->getId(),0,GL_RGBA,GL_FLOAT,surface);::draw(*mesh);auto specular=read(GL_COLOR_ATTACHMENT1);require(specular[0]>0.02f&&std::abs(specular[0]-2.f*specular[1])<0.002f&&specular[2]==0.f,"Specular lighting is absent or loses the light color");

        CCamera camera;camera.setView(glm::lookAt(glm::vec3(0,0,2),glm::vec3(0),glm::vec3(0,1,0)));camera.setProjection(glm::ortho(-1.f,1.f,-1.f,1.f,0.1f,10.f));camera.setPosition(glm::vec3(0,0,2));
        camera.getFeatureInfoForWrite().godRayActive=false;camera.getFeatureInfoForWrite().dofActive=false;camera.getFeatureInfoForWrite().lenseFlareActive=false;
        auto red=resources->createImage({255,0,0},1,1,EColorFormat::RGB24),blue=resources->createImage({0,0,255},1,1,EColorFormat::RGB24);
        auto redMaterial=resources->createMaterial(red,invalidResource,invalidResource,invalidResource,invalidResource),blueMaterial=resources->createMaterial(blue,invalidResource,invalidResource,invalidResource,invalidResource);
        SMaterialSettings blend;blend.alphaMode=AlphaMode::Blend;blend.opacity=0.5f;resources->setMaterialSettings(redMaterial,blend);resources->setMaterialSettings(blueMaterial,blend);
        CScene layers(&graphics);layers.setAmbientLight(glm::vec3(1),1.f);
        layers.createObject(meshId,redMaterial,glm::vec3(0,0,0.5f),glm::quat(1,0,0,0),glm::vec3(1));
        layers.createObject(meshId,blueMaterial,glm::vec3(0),glm::quat(1,0,0,0),glm::vec3(1));
        auto prime=[&](CScene& scene) {CSceneQuery query;scene.getVisibleObjects(camera,query);renderer.geometryPass(scene,camera,window,graphics,query);renderer.lightPass(scene,camera,window,graphics,query);renderer.illuminationPass(scene,camera,window,graphics,query);};
        prime(layers);renderer.transparencyPass(layers,camera,window,graphics);auto blended=read();std::printf("Blend sample: %.6f %.6f %.6f\n",blended[0],blended[1],blended[2]);require(std::abs(blended[0]-0.5f)<0.003f&&std::abs(blended[2]-0.25f)<0.003f,"Translucent objects are not blended back-to-front in linear light");

        camera.setView(identity);camera.setPosition(glm::vec3(0));camera.setProjection(glm::perspective(glm::radians(60.f),static_cast<float>(window.getWidth())/window.getHeight(),0.1f,100.f));
        camera.getFeatureInfoForWrite().shadowDistance=10.f;
        camera.getFeatureInfoForWrite().shadowMapResolution=512;
        camera.getFeatureInfoForWrite().pointShadowResolution=256;
        auto glass=resources->createMaterial(white,invalidResource,invalidResource,invalidResource,invalidResource);
        resources->setMaterialSettings(glass,blend);
        CScene shadowLayers(&graphics);
        shadowLayers.createObject(meshId,glass,glm::vec3(0,0,-3),glm::quat(1,0,0,0),glm::vec3(1));
        auto caster=shadowLayers.createObject(meshId,opaqueMask,glm::vec3(0.75f,0,-2.25f),glm::quat(1,0,0,0),glm::vec3(0.2f));
        auto directional=shadowLayers.createDirectionalLight(glm::vec3(-1,0,-1),glm::vec3(1),1.f,true);
        prime(shadowLayers);renderer.transparencyPass(shadowLayers,camera,window,graphics);const float directionalShadowed=read()[0];
        camera.getFeatureInfoForWrite().shadowsActive=false;
        prime(shadowLayers);renderer.transparencyPass(shadowLayers,camera,window,graphics);const float directionalLit=read()[0];
        require(directionalLit>0.1f&&directionalShadowed<directionalLit*0.5f,"Translucent material did not receive an opaque directional shadow");
        shadowLayers.setDirectionalLight(directional,glm::vec3(-1,0,-1),glm::vec3(1),0.f,false);
        shadowLayers.setObject(caster,meshId,opaqueMask,glm::vec3(0.5f,0,-2.25f),glm::quat(1,0,0,0),glm::vec3(0.2f),true);
        shadowLayers.createPointLight(glm::vec3(2,0,0),8.f,glm::vec3(1),1.f,true);
        camera.getFeatureInfoForWrite().shadowsActive=true;
        prime(shadowLayers);renderer.transparencyPass(shadowLayers,camera,window,graphics);const float pointShadowed=read()[0];
        camera.getFeatureInfoForWrite().shadowsActive=false;
        prime(shadowLayers);renderer.transparencyPass(shadowLayers,camera,window,graphics);const float pointLit=read()[0];
        require(pointLit>0.1f&&pointShadowed<pointLit*0.5f,"Translucent material did not receive an opaque point shadow");
        camera.getFeatureInfoForWrite().shadowsActive=true;        CScene sun(&graphics);sun.createDirectionalLight(glm::vec3(0,0,1),glm::vec3(1),1.f,false);
        prime(sun);renderer.backgroundPass(sun,camera,window,graphics);require(read()[0]>10.f,"Visible sun was not rendered into the background");
        bind();renderer.godRayPass1(window,graphics,color);require(read()[0]>0.01f,"Visible sun produced no rays");
        auto blocker=sun.createObject(meshId,opaqueMask,glm::vec3(0,0,-2),glm::quat(1,0,0,0),glm::vec3(1));
        prime(sun);renderer.backgroundPass(sun,camera,window,graphics);bind();renderer.godRayPass1(window,graphics,color);require(read()[0]<0.001f,"An occluded sun still produced rays");
        sun.setObject(blocker,meshId,opaqueMask,glm::vec3(0,0,-2),glm::quat(1,0,0,0),glm::vec3(1),false);
        camera.setView(glm::lookAt(glm::vec3(0),glm::vec3(0.4f,0,-1),glm::vec3(0,1,0)));prime(sun);renderer.backgroundPass(sun,camera,window,graphics);require(read()[0]<1.f,"Sun remained fixed at the screen center");

        bind();program->setActive();glDepthMask(GL_FALSE);glDepthFunc(GL_GREATER);glEnable(GL_SCISSOR_TEST);glEnable(GL_FRAMEBUFFER_SRGB);glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);
        for(int frame=0;frame<3;++frame){renderer.draw(sun,camera,window,graphics);glFinish();}
        GLint restored=0;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&restored);require(static_cast<GLuint>(restored)==target.getId(),"Renderer did not restore the caller's framebuffer");
        glGetIntegerv(GL_CURRENT_PROGRAM,&restored);require(restored==before,"Renderer did not restore the caller's shader");
        GLboolean depthWrite=GL_TRUE;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);require(!depthWrite&&glIsEnabled(GL_SCISSOR_TEST)&&glIsEnabled(GL_FRAMEBUFFER_SRGB),"Renderer leaked depth/scissor/color-space state");
        require(renderer.profiler().cpuMilliseconds().contains("frame")&&renderer.profiler().gpuMilliseconds().contains("geometry"),"Non-blocking CPU/GPU timings are missing");
        glDisable(GL_SCISSOR_TEST);glDisable(GL_FRAMEBUFFER_SRGB);glDepthMask(GL_TRUE);glDepthFunc(GL_LESS);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);CFrameBuffer::setDefaultActive();
        std::filesystem::create_directories("build/reload-tests");
        const std::string png="build/reload-tests/color.png",obj="build/reload-tests/mesh.obj",mat="build/reload-tests/material.json",model="build/reload-tests/model.json";
        require(lodepng::encode(png,{128,128,128},1,1,LCT_RGB)==0,"PNG fixture creation failed");
        {std::ofstream f(obj);f<<"v -1 -1 0\nv 1 -1 0\nv 0 1 0\nf 1 2 3\n";}
        {std::ofstream f(mat);f<<"{\"base\":\""<<png<<"\"}";}
        {std::ofstream f(model);f<<"{\"mesh\":\""<<obj<<"\",\"material\":\""<<mat<<"\"}";}
        auto fileModel=resources->loadModel(model);auto* modelWrapper=graphics.getModel(fileModel);
        require(modelWrapper&&modelWrapper->hasMesh(),"File-backed model load failed");
        const auto fileMesh=modelWrapper->getMeshId(),fileMaterial=modelWrapper->getMaterialId();
        auto* meshWrapper=graphics.getMesh(fileMesh);auto* materialWrapper=graphics.getMaterial(fileMaterial);
        auto* colorWrapper=materialWrapper->getDiffuse();
        const auto fileImage=resources->loadImage(png,EColorFormat::RGB24);
        require(lodepng::encode(png,{64,64,64},1,1,LCT_RGB)==0&&resources->reloadResource(EResourceType::Image,fileImage),"Image reload failed");
        require(materialWrapper->getDiffuse()==colorWrapper,"Image reload invalidated material pointers");
        require(resources->updateString(fragmentId,sample),"Sample shader restore failed");bind();colorWrapper->setActive(0);program->setUniform("scene_texture",0);::draw(*mesh);
        require(std::abs(read()[0]-0.05127f)<0.002f,"The sRGB image variant was not refreshed");
        {std::ofstream f(obj);f<<"v -3 -3 0\nv 3 -3 0\nv 0 3 0\nf 1 2 3\n";}
        require(resources->reloadResource(EResourceType::Mesh,fileMesh)&&graphics.getMesh(fileMesh)==meshWrapper&&meshWrapper->getBoundingSphere().getRadius()>4.f,"Mesh reload lost object identity or bounds");
        {std::ofstream f(mat);f<<"{\"base\":\""<<png<<"\",\"alpha_mode\":\"blend\",\"opacity\":0.25,\"color_space\":\"linear\"}";}
        require(resources->reloadResource(EResourceType::Material,fileMaterial)&&graphics.getMaterial(fileMaterial)==materialWrapper&&materialWrapper->settings.alphaMode==AlphaMode::Blend&&materialWrapper->settings.opacity==0.25f,"Material reload did not apply settings");
        {
            CScene linked(&graphics);auto object=linked.createObject(fileModel,glm::vec3(0),glm::quat(1,0,0,0),glm::vec3(1));
            {std::ofstream f(model);f<<"{\"mesh\":\"data/mesh/screen_quad.obj\",\"material\":\""<<mat<<"\"}";}
            require(resources->reloadResource(EResourceType::Model,fileModel)&&graphics.getModel(fileModel)==modelWrapper,"Model reload invalidated its wrapper");
            ResourceId resolvedMesh,resolvedMaterial;glm::vec3 position,scale;glm::quat rotation;bool visible;
            require(linked.getObject(object,resolvedMesh,resolvedMaterial,position,rotation,scale,visible)&&resolvedMesh!=fileMesh,"Live scene model retained an obsolete mesh");
            require(linked.removeObject(object),"Scene handle release failed");
        }
        require(resources->removeResource(EResourceType::Model,fileModel)&&resources->removeResource(EResourceType::Mesh,fileMesh)&&resources->removeResource(EResourceType::Material,fileMaterial)&&resources->removeResource(EResourceType::Image,fileImage),"Reloaded resource cleanup failed");        require(!resources->removeResource(EResourceType::Image,gray),"Deleted an image still referenced by a material");
        require(!resources->removeResource(EResourceType::Material,materialId),"Deleted a material still referenced by a model");
        require(resources->removeResource(EResourceType::Model,modelId)&&!graphics.getModel(modelId),"GPU model deletion failed");
        require(resources->removeResource(EResourceType::Material,materialId)&&resources->removeResource(EResourceType::Image,gray)&&!graphics.getTexture(gray),"Dependency-safe GPU resource deletion failed");
        std::puts("Remaining rendering checks passed: model replay/reload/delete, sRGB, cutouts/shadows, colored specular, sorted transparency, projected/occluded sun, state isolation, alias validation and GPU/CPU timings.");
        return true;
    } catch(const std::exception& error) {std::fprintf(stderr,"Remaining rendering regression: %s\n",error.what());return false;}
}
