#include "CInstancedDraw.h"
#include "MaterialDraw.h"
#include "graphics/IGraphicsResourceManager.h"
#include "graphics/renderer/debug/RendererDebug.h"
#include "graphics/resource/CMesh.h"
#include "graphics/resource/CShaderProgram.h"
CInstancedDraw::CInstancedDraw() = default;
CInstancedDraw::~CInstancedDraw()
{
    if (m_buffer)
        glDeleteBuffers(1, &m_buffer);
}
void CInstancedDraw::begin()
{
    for (auto &[key, models] : m_batches)
        models.clear();
}
void CInstancedDraw::add(ResourceId mesh, ResourceId material, const glm::mat4 &model,
                         bool mirrored)
{
    m_batches[{mesh, material, mirrored}].push_back(model);
}
void CInstancedDraw::draw(const IGraphicsResourceManager &manager, CShaderProgram &shader)
{
    if (!m_buffer)
        glCreateBuffers(1, &m_buffer);
    GLint previousStorage = 0, previousGeneric = 0, previousVao = 0, previousFront = 0;
    glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 7, &previousStorage);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &previousGeneric);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_FRONT_FACE, &previousFront);
    for (const auto &[key, models] : m_batches)
    {
        if (models.empty())
            continue;
        const auto [meshId, materialId, mirrored] = key;
        auto *mesh = manager.getMesh(meshId);
        if (!mesh)
            continue;
        bindMaterial(manager.getMaterial(materialId), manager, &shader);
        shader.setUniform("instanced", 1);
        glNamedBufferData(m_buffer, models.size() * sizeof(glm::mat4), models.data(),
                          GL_STREAM_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, m_buffer);
        glFrontFace(mirrored ? GL_CW : GL_CCW);
        // Keep framebuffer feedback validation, once per material batch rather than per ship.
        validateFramebufferFeedback();
        mesh->getVertexArray()->setActive();
        const auto primitive = CMesh::toGLPrimitive(mesh->getPrimitiveType());
        if (mesh->hasIndexBuffer())
        {
            mesh->getIndexBuffer()->setActive();
            glDrawElementsInstanced(primitive, mesh->getIndexBuffer()->getSize(), GL_UNSIGNED_INT,
                                    nullptr, GLsizei(models.size()));
        }
        else
            glDrawArraysInstanced(primitive, 0, mesh->getVertexBuffer()->getSize() / 3,
                                  GLsizei(models.size()));
    }
    shader.setUniform("instanced", 0);
    glBindVertexArray(previousVao);
    glFrontFace(previousFront);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, previousStorage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, previousGeneric);
}
