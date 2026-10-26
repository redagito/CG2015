#include "CUiRenderer.h"
#include "resource/IResourceManager.h"
#include "graphics/IGraphicsResourceManager.h"
#include "graphics/resource/CShaderProgram.h"
#include "graphics/resource/CTexture.h"
#include "graphics/renderer/core/CScopedRenderState.h"
#include "graphics/renderer/debug/RendererDebug.h"
#include <json/json.h>
#include <algorithm>
#include <stdexcept>

bool CUiRenderer::init(IResourceManager& resources)
{
    CScopedRenderState state;
    m_shader=resources.loadShader("data/shader/ui.ini");
    m_font=resources.loadImage("data/ui/font.png",EColorFormat::GreyScale8);
    if(m_shader==invalidResource || m_font==invalidResource) return false;
    Json::Value root;
    if(!resources.loadJsonDocument("data/ui/font.json",root)) return false;
    m_fontSize=root["size"].asFloat();m_ascent=root["ascent"].asFloat();m_descent=root["descent"].asFloat();
    m_atlasWidth=root["width"].asFloat();m_atlasHeight=root["height"].asFloat();
    if(m_fontSize<=0 || m_atlasWidth<=0 || m_atlasHeight<=0) return false;
    for(unsigned code=32;code<127;++code) {
        const auto& g=root["glyphs"][std::to_string(code)];
        m_glyphs[code]={g["x"].asFloat(),g["y"].asFloat(),g["width"].asFloat(),g["height"].asFloat(),
            g["offsetX"].asFloat(),g["offsetY"].asFloat(),g["advance"].asFloat()};
    }
    const GLuint vao=m_vao.getId();
    // These legacy wrappers reserve names with glGen*. Instantiate the objects
    // before configuring them through direct-state-access calls.
    GLint previousArray=0;glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&previousArray);
    m_vao.setActive();glBindBuffer(GL_ARRAY_BUFFER,m_vertices.getId());
    glVertexArrayVertexBuffer(vao,0,m_vertices.getId(),0,9*sizeof(float));
    const GLint sizes[]={2,2,4,1};const GLuint offsets[]={0,2,4,8};
    for(GLuint attribute=0;attribute<4;++attribute) {
        glEnableVertexArrayAttrib(vao,attribute);
        glVertexArrayAttribFormat(vao,attribute,sizes[attribute],GL_FLOAT,GL_FALSE,offsets[attribute]*sizeof(float));
        glVertexArrayAttribBinding(vao,attribute,0);
    }
    glBindBuffer(GL_ARRAY_BUFFER,previousArray);
    return true;
}

void CUiRenderer::draw(const UiDocument& document,unsigned width,unsigned height,
                       const IGraphicsResourceManager& resources)
{
    if(document.empty() || !width || !height) return;
    CScopedRenderState state;
    // Buffer uploads in the old wrapper bind GL_ARRAY_BUFFER; restore that too.
    GLint previousArray=0;glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&previousArray);
    const float scale=std::min(width/1280.f,height/720.f);
    std::vector<float> vertices;
    auto quad=[&](float x,float y,float w,float h,glm::vec4 uv,glm::vec4 color,float glyph) {
        const float points[][4]={{x,y,uv.x,uv.y},{x+w,y,uv.z,uv.y},{x,y+h,uv.x,uv.w},
            {x+w,y,uv.z,uv.y},{x+w,y+h,uv.z,uv.w},{x,y+h,uv.x,uv.w}};
        for(const auto& p:points) vertices.insert(vertices.end(),{p[0],p[1],p[2],p[3],color.r,color.g,color.b,color.a,glyph});
    };
    for(const auto& rect:document.rectangles) {
        const auto size=rect.size*scale,center=rect.anchor*glm::vec2(width,height)+rect.offset*scale;
        quad(center.x-size.x/2,center.y-size.y/2,size.x,size.y,glm::vec4(0),rect.color,0);
    }
    for(const auto& text:document.texts) {
        if(text.text.empty()) continue;
        float advance=0;
        for(unsigned char c:text.text) advance+=m_glyphs[c<128?c:'?'].advance;
        float spacing=text.tracking*scale;
        float glyphScale=text.size*scale/m_fontSize;
        float lineWidth=advance*glyphScale+spacing*(text.text.size()-1);
        const float limit=width*std::clamp(text.maxWidth,0.01f,1.f);
        if(lineWidth>limit) {const float fit=limit/lineWidth;glyphScale*=fit;spacing*=fit;lineWidth=limit;}
        float x=text.anchor.x*width-lineWidth/2;
        const float baseline=text.anchor.y*height+(m_ascent-m_descent)*glyphScale/2;
        for(unsigned char c:text.text) {
            const auto& g=m_glyphs[c<128?c:'?'];
            if(g.width>0 && g.height>0) quad(x+g.offsetX*glyphScale,baseline+g.offsetY*glyphScale,
                g.width*glyphScale,g.height*glyphScale,
                {g.x/m_atlasWidth,g.y/m_atlasHeight,(g.x+g.width)/m_atlasWidth,(g.y+g.height)/m_atlasHeight},text.color,1);
            x+=g.advance*glyphScale+spacing;
        }
    }
    if(vertices.empty()) return;
    glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_RASTERIZER_DISCARD);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    auto* shader=resources.getShaderProgram(m_shader);auto* font=resources.getTexture(m_font);
    if(!shader || !font) throw std::runtime_error("UI resources are unavailable");
    font->setActive(0);glBindSampler(0,0);
    shader->setUniform("font_texture",0);shader->setUniform("viewport",glm::vec2(width,height));shader->setActive();
    m_vertices.setData(vertices);glBindBuffer(GL_ARRAY_BUFFER,previousArray);
    m_vao.setActive();validateFramebufferFeedback();glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()/9));
}
