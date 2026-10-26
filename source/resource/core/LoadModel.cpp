#include "LoadModel.h"

#include "io/JsonUtil.h"
#include "io/JsonDeserialize.h"
#include "util/StringUtil.h"
#include <spdlog/spdlog.h>

bool load(const std::string& file, CResourceManager& manager, SModel& model)
{
	std::string extension = getFileExtension(file);
	std::string mesh;
	std::string material;
	if (extension == "json")
	{
		if (!loadModelFromJson(file, mesh, material))
		{
			return false;
		}
	}
	else
	{
		SPDLOG_ERROR("Invalid or unknown model file format.");
		return false;
	}

	// Load mesh
	ResourceId meshId = manager.loadMesh(mesh);
	if (meshId == invalidResource)
	{
		SPDLOG_ERROR("Failed to load mesh.");
		return false;
	}

	// Load material
	ResourceId materialId = manager.loadMaterial(material);
	if (materialId == invalidResource)
	{
		SPDLOG_ERROR("Failed to load material.");
		return false;
	}

	// Set ids
	model.m_mesh = meshId;
	model.m_material = materialId;
	return true;
}

bool loadModelFromJson(const std::string& file, std::string& meshFile, std::string& materialFile)
{
	Json::Value root;
	if (!load(file, root))
	{
		return false;
	}

	if (!deserialize(root["mesh"], meshFile))
	{
		SPDLOG_ERROR("Failed to load mesh node.");
		return false;
	}
	if (!deserialize(root["material"], materialFile))
	{
		SPDLOG_ERROR("Failed to load material node.");
		return false;
	}
	return true;
}
