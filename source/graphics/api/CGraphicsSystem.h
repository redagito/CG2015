#pragma once

#include <vector>
#include <atomic>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include "GraphicsConfig.h"
#include "CGraphicsWorld.h"

class CGraphicsSystem
{
public:
	bool init();
	void start();
	void stop();
	
	GraphicsResourceId createMesh();
	void setMesh(GraphicsResourceId id, const std::vector<float>& vertices, 
		const std::vector<float>& normals, const std::vector<float>& uvs, const std::vector<uint32_t>& indices);

	GraphicsResourceId createTexture();
	void setTexture(GraphicsResourceId id, const std::vector<uint8_t>& data, uint32_t width, uint32_t height);

	GraphicsResourceId createMaterial();
	void setMaterial(GraphicsResourceId id, GraphicsResourceId baseTexture);

	GraphicsResourceId createModel();
	void setModel(GraphicsResourceId id, GraphicsResourceId meshId, GraphicsResourceId materialId);

	GraphicsResourceId createWorld();
	GraphicsResourceId worldCreateStaticObject(GraphicsResourceId worldId);

	void worldSetStaticObject(GraphicsResourceId worldId, GraphicsResourceId objectId, GraphicsResourceId modelId, 
		const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);

	void worldSetAmbientLight(GraphicsResourceId worldId, const glm::vec3& color, float intensity);

	void worldSetActve(GraphicsResourceId world);

private:
	GraphicsResourceId m_nextMeshId = 1;
	GraphicsResourceId m_nextTextureId = 1;
	GraphicsResourceId m_nextMaterialId = 1;
	GraphicsResourceId m_nextModelId = 1;

	GraphicsResourceId m_nextWorldId = 1;

	std::unordered_map<GraphicsResourceId, std::unique_ptr<CGraphicsWorld>> m_worlds;
};