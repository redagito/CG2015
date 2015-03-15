#include "Draw.h"

#include "graphics/core/PrimitiveUtility.h"

void draw(CMesh& mesh)
{
	mesh.getVertexArray()->setActive();

	// Set primitive draw mode
	GLenum mode = toGLPrimitive(mesh.getPrimitiveType());
	unsigned int primitiveSize = getSize(mesh.getPrimitiveType());

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
		glDrawArrays(mode, 0, mesh.getVertexBuffer()->getSize() / primitiveSize);
	}
	mesh.getVertexArray()->setInactive();
}