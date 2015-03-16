#pragma once

#include <vector>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <thread>

#include <glm/glm.hpp>

#include "core/CJobSystem.h"

#include "GraphicsConfig.h"
#include "CGraphicsWorld.h"

struct GLFWwindow;

class CGraphicsSystem
{
public:
	~CGraphicsSystem();

	bool init(unsigned int width, unsigned int height, const std::string& name);
	void start(CJobSystem& jobSystem);
	bool isRunning() const;
	void pollEvents();
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

protected:
	void render();

private:
	GLFWwindow* m_window = nullptr;
	unsigned int m_width = 0;
	unsigned int m_height = 0;

	std::atomic<bool> m_running = false;
	bool m_init = false;

	GraphicsResourceId m_nextMeshId = 1;
	GraphicsResourceId m_nextTextureId = 1;
	GraphicsResourceId m_nextMaterialId = 1;
	GraphicsResourceId m_nextModelId = 1;

	GraphicsResourceId m_nextWorldId = 1;

	std::unordered_map<GraphicsResourceId, std::unique_ptr<CGraphicsWorld>> m_worlds;
};