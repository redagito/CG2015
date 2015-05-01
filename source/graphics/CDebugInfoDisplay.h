#pragma once

#include <memory>
#include <string>
#include <vector>

class CDebugInfo;
class CShaderProgram;
class CTexture;
class CVertexArrayObject;
class CVertexBuffer;
class IResourceManager;

class CDebugInfoDisplay
{
   public:
    CDebugInfoDisplay(IResourceManager& resourceManager);

	bool loadFont(const std::string& path, IResourceManager& resourceManager);
    void draw(const CDebugInfo& info);

   private:
    std::unique_ptr<CShaderProgram> m_textShader = nullptr;
    std::unique_ptr<CShaderProgram> m_overlayShader = nullptr;
    std::unique_ptr<CTexture> m_texture = nullptr;

    std::unique_ptr<CVertexArrayObject> m_VAO = nullptr;
    
    std::unique_ptr<CVertexBuffer> m_verticesBuffer = nullptr;
    std::unique_ptr<CVertexBuffer> m_uvsBuffer = nullptr;
    std::unique_ptr<CVertexBuffer> m_overlaysBuffer = nullptr;

    int m_fontSize = 12;
};
