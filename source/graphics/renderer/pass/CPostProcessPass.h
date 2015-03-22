#pragma once

#include <string>

#include "IRenderPass.h"

class CPostProcessPass
{
public:
	CPostProcessPass(const std::string& shaderFile);

	bool init(IResourceManager* manager);

	void run();
};