#pragma once
#include "UiDocument.h"
#include "graphics/renderer/core/CVertexArrayObject.h"
#include "graphics/renderer/core/CVertexBuffer.h"
#include "resource/ResourceConfig.h"
#include <array>

class IResourceManager;
class IGraphicsResourceManager;

class CUiRenderer {
public:
    bool init(IResourceManager& resources);
    // Draw into the caller's destination after scene post-processing.
    void draw(const UiDocument&, unsigned width, unsigned height,
              const IGraphicsResourceManager& resources);
private:
    struct Glyph {
        float x=0,y=0,width=0,height=0,offsetX=0,offsetY=0,advance=0;
    };
    std::array<Glyph,128> m_glyphs{};
    float m_fontSize=128,m_ascent=0,m_descent=0,m_atlasWidth=1,m_atlasHeight=1;
    ResourceId m_shader=invalidResource,m_font=invalidResource;
    CVertexArrayObject m_vao;
    CVertexBuffer m_vertices{GL_STREAM_DRAW};
};
