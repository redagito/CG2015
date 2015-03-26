#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <mutex>

/**
 * \brief Simple profiler class.
 *
 * TODO Make thread safe?
 */
class CProfiler
{
public:
	CProfiler(const std::string& name);
	~CProfiler();

	/**
	 * \brief Formats profiler data and returns as string.
	 */
	static std::string toString();

private:
	/**
	* \brief Profile data.
	*/
	struct SData
	{
		uint64_t callCount = 0; /**< Total number of calls. */
		double totalCallTime = 0.f; /**< Total time of calls. */
		double minCallTime = 9999999.f; /**< Min call time. */
		double maxCallTime = 0.f; /**< Max call time. */
	};

	double m_time = 0.0; /**< Start time. */
	std::string m_name; /**< Profile name. */
	static std::unordered_map<std::string, SData> s_profileData; /**< Profile data. */
	static std::mutex s_mutex;
};
