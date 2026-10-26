#include <fstream>

#include "JsonUtil.h"
#include "JsonDeserialize.h"
#include <spdlog/spdlog.h>

bool load(const std::string& file, Json::Value& value)
{
	std::ifstream ifs(file);
	if (!ifs.is_open())
	{
		SPDLOG_ERROR("failed to open json file {}.", file.c_str());
		return false;
	}

	Json::Reader reader;
	if (!reader.parse(ifs, value))
	{
		SPDLOG_ERROR("Failed to parse json file {}: {}", file.c_str(), reader.getFormattedErrorMessages().c_str());
		return false;
	}
	return true;
}

bool load(const Json::Value& node, const std::string& name, int& i)
{
	if (!deserialize(node[name], i))
	{
		SPDLOG_ERROR("Failed to load '{}' parameter.", name.c_str());
		return false;
	}
	return true;
}

bool load(const Json::Value& node, const std::string& name, unsigned int& u)
{
	if (!deserialize(node[name], u))
	{
		SPDLOG_ERROR("Failed to load '{}' parameter.", name.c_str());
		return false;
	}
	return true;
}

bool load(const Json::Value& node, const std::string& name, float& f)
{
	if (!deserialize(node[name], f))
	{
		SPDLOG_ERROR("Failed to load '{}' parameter.", name.c_str());
		return false;
	}
	return true;
}

bool load(const Json::Value& node, const std::string& name, bool& b)
{
	if (!deserialize(node[name], b))
	{
		SPDLOG_ERROR("Failed to load '{}' parameter.", name.c_str());
		return false;
	}
	return true;
}

bool load(const Json::Value& node, const std::string& name, glm::vec3& vec)
{
	if (!deserialize(node[name], vec))
	{
		SPDLOG_ERROR("Failed to load '{}' parameter.", name.c_str());
		return false;
	}
	return true;
}

bool load(const Json::Value& node, const std::string& name, std::string& str)
{
	if (!deserialize(node[name], str))
	{
		SPDLOG_ERROR("Failed to load '{}' parameter.", name.c_str());
		return false;
	}
	return true;
}