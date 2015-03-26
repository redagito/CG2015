#include <cassert>
#include <sstream>
#include <iostream>

#include "CProfiler.h"
#include "util/Time.h"

std::mutex CProfiler::s_mutex;

std::unordered_map<std::string, CProfiler::SData> CProfiler::s_profileData;

CProfiler::CProfiler(const std::string& name)
	:
	m_time(getTime()),
	m_name(name)
{
	return;
}

CProfiler::~CProfiler()
{
	m_time = (getTime() - m_time) * 1000.0;

	// Lock map
	// TODO Per entry locking
	std::lock_guard<std::mutex> lock(s_mutex);

	// Get entry
	CProfiler::SData& data = s_profileData[m_name];

	// Update call count and call time
	data.callCount++;
	data.totalCallTime += m_time;
	
	// Update max and min times
	if (data.maxCallTime < m_time)
	{
		data.maxCallTime = m_time;
	}
	if (data.minCallTime > m_time)
	{
		data.minCallTime = m_time;
	}
}

std::string CProfiler::toString()
{
	std::stringstream ss;
	ss << "[Profile data]" << std::endl;
	
	// Lock map
	std::lock_guard<std::mutex> lock(s_mutex);
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