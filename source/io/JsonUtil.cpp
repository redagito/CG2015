#include <fstream>

#include "JsonUtil.h"
#include "debug/Log.h"

bool load(const std::string& file, Json::Value& value)
{
	std::ifstream ifs(file);
	if (!ifs.is_open())
	{
		LOG_ERROR("failed to open json file %s.", file.c_str());
		return false;
	}

	Json::Reader reader;
	if (!reader.parse(ifs, value))
	{
		LOG_ERROR("Failed to parse json file %s: %s", file.c_str(), reader.getFormattedErrorMessages().c_str());
		return false;
	}
	return true;
}