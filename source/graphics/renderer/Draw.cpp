#include <cassert>

#include "Draw.h"
#include "graphics/renderer/debug/RendererDebug.h"

#include "graphics/resource/CMesh.h"

void draw(CMesh& mesh)
{
    validateFramebufferFeedback();
    mesh.getVertexArray()->setActive();

    // Set primitive draw mode
    GLenum mode = CMesh::toGLPrimitive(mesh.getPrimitiveType());

    // Decide on draw method based on the stored data
    if (mesh.hasIndexBuffer())
    {
        // Indexed draw, faster
        mesh.getIndexBuffer()->setActive();
        glDrawElements(mode, mesh.getIndexBuffer()->getSize(), GL_UNSIGNED_INT, nullptr);
        mesh.getIndexBuffer()->setInactive();
    }
    else
    {
        // Slowest draw method
        // Positions always have three floats, regardless of primitive topology.
        glDrawArrays(mode, 0, mesh.getVertexBuffer()->getSize() / 3);
    }
    mesh.getVertexArray()->setInactive();
}
