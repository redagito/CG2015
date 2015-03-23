#include "CProfiler.h"

#include <GLFW/glfw3.h>

#include <cassert>
#include <sstream>
#include <iostream>

std::mutex CProfiler::d_mutex;

std::unordered_map<std::string, CProfiler::SData> CProfiler::s_profileData;

CProfiler::CProfiler(const std::string& name)
	:
	d_time(glfwGetTime()),
	d_name(name)
{
	return;
}

CProfiler::~CProfiler()
{
	d_time = (glfwGetTime() - d_time) * 1000.0;

	// Lock map
	// TODO Per entry locking
	//std::lock_guard<std::mutex> lock(d_mutex);

	// Get entry
	CProfiler::SData& data = s_profileData[d_name];

	// Update call count and call time
	data.callCount++;
	data.totalCallTime += d_time;
	
	// Update max and min times
	if (data.maxCallTime < d_time)
	{
		data.maxCallTime = d_time;
	}
	if (data.minCallTime > d_time)
	{
		data.minCallTime = d_time;
	}
}

std::string CProfiler::toString()
{
	std::stringstream ss;
	ss << "[Profile data]" << std::endl;

	// Lock map
	//std::lock_guard<std::mutex> lock(d_mutex);

	// Write profiling statistics
	for (const auto& data : s_profileData)
	{
		ss << "Name: " << data.first << std::endl;
		ss << "\tCall count: " << data.second.callCount << std::endl;
		ss << "\tTotal call time: " << data.second.totalCallTime << " [ms]" << std::endl;
		ss << "\tMin call time: " << data.second.minCallTime << " [ms]" << std::endl;
		ss << "\tMax call time: " << data.second.maxCallTime << " [ms]" << std::endl;
		ss << "\tAverage call time: " << data.second.totalCallTime / data.second.callCount << " [ms]" << std::endl;
	}

	return ss.str();
}