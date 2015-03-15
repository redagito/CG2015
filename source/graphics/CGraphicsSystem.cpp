#include "CGraphicsSystem.h"


bool CGraphicsSystem::init()
{
	return true;
}

void CGraphicsSystem::start()
{
	return;
}

void CGraphicsSystem::stop()
{
	return;
}

GraphicsResourceId CGraphicsSystem::createMesh()
{
	GraphicsResourceId meshId = m_nextMeshId;
	++m_nextMeshId;
	return meshId;
}

void CGraphicsSystem::setMesh(GraphicsResourceId id, const std::vector<float>& vertices,
	const std::vector<float>& normals, const std::vector<float>& uvs, const std::vector<uint32_t>& indices)
{

}

GraphicsResourceId CGraphicsSystem::createTexture()
{
	GraphicsResourceId textureId = m_nextTextureId;
	++m_nextTextureId;
	return textureId;
}

void CGraphicsSystem::setTexture(GraphicsResourceId id, const std::vector<uint8_t>& data, uint32_t width, uint32_t height)
{

}

GraphicsResourceId CGraphicsSystem::createMaterial()
{
	GraphicsResourceId materialId = m_nextMaterialId;
	++m_nextMaterialId;
	return materialId;
}

void CGraphicsSystem::setMaterial(GraphicsResourceId id, GraphicsResourceId baseTexture)
{

}

GraphicsResourceId CGraphicsSystem::createModel()
{
	GraphicsResourceId modelId = m_nextModelId;
	++m_nextModelId;
	return modelId;
}

void CGraphicsSystem::setModel(GraphicsResourceId id, GraphicsResourceId meshId, GraphicsResourceId materialId)
{

}

GraphicsResourceId CGraphicsSystem::createWorld()
{
	GraphicsResourceId worldId = m_nextWorldId;
	++m_nextWorldId;
	return worldId;
}

GraphicsResourceId CGraphicsSystem::worldCreateStaticObject(GraphicsResourceId worldId)
{
	CGraphicsWorld* world = m_worlds.at(worldId).get();
	GraphicsResourceId objectId = world->createStaticObject();
	return objectId;
}

void CGraphicsSystem::worldSetStaticObject(GraphicsResourceId worldId, GraphicsResourceId objectId, GraphicsResourceId modelId,
	const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{

}

void CGraphicsSystem::worldSetAmbientLight(GraphicsResourceId worldId, const glm::vec3& color, float intensity)
{

}

void CGraphicsSystem::worldSetActve(GraphicsResourceId world)
{

}