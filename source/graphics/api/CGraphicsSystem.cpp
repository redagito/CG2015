#include <string>

#include "CGraphicsSystem.h"

#include "graphics/core/CoreConfig.h"
#include <GLFW/glfw3.h>

#include "log/Log.h"

CGraphicsSystem::~CGraphicsSystem()
{
	stop();
}

bool CGraphicsSystem::init(unsigned int width, unsigned int height, const std::string& name)
{
	if (m_init)
	{
		return true;
	}

	LOG_INFO("Initializing graphics system.");
	LOG_INFO("Initializing GLFW.");
	if (!glfwInit())
	{
		LOG_ERROR("Failed to initialize GLFW.");
		return false;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, FLEXT_MAJOR_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, FLEXT_MINOR_VERSION);
	// Remove deprecated functionality
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// Request core profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window from parameters
	m_window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
	if (m_window == nullptr)
	{
		LOG_ERROR("Failed to create GLFW window.");
		glfwTerminate();
		return false;
	}

	int framebufferWidth;
	int framebufferHeight;

	glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

	m_width = framebufferWidth;
	m_height = framebufferHeight;

	glfwMakeContextCurrent(m_window);
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);

	// Load extensions
	if (flextInit() != GL_TRUE)
	{
		glfwTerminate();
		LOG_ERROR("Failed to initialize flextGL.");
		return false;
	}
	LOG_INFO("OpenGL version %u.%u core.", FLEXT_MAJOR_VERSION, FLEXT_MINOR_VERSION);
	glfwShowWindow(m_window);
	glfwMakeContextCurrent(nullptr);
	m_init = true;
	return true;
}

void CGraphicsSystem::start()
{
	if (!m_init)
	{
		LOG_CRITICAL("Cannot start rendering without initailization.");
		return;
	}
	m_running = true;
	m_renderThread = std::thread(&CGraphicsSystem::render, this);
	return;
}

bool CGraphicsSystem::isRunning() const
{
	return m_running;
}

void CGraphicsSystem::stop()
{
	m_running = false;
	m_renderThread.join();
	return;
}

void CGraphicsSystem::pollEvents()
{
	glfwPollEvents();
}

void CGraphicsSystem::render()
{
	glfwMakeContextCurrent(m_window);
	glClearColor(0.2f, 0.3f, 0.2f, 1.f);
	while (m_running && glfwWindowShouldClose(m_window) == 0)
	{
		// Draw logic here
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(m_window);
	}
	m_running = false;
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