#include <string>

#include "log/Log.h"
#include "log/CLogger.h"
#include "util/Util.h"

#include "engine/CEngine.h"

int main(int argc, const char** argv)
{
	// Initialize logger
	std::string logFile = "log/" + createTimeStamp() + ".txt";
	if (!CLogger::initLogFile(logFile))
	{
		LOG_WARNING("Failed to create log file %s.", logFile.c_str());
	}

	// Create and initialize game engine
	CEngine engine;
	if (!engine.init(argc, argv))
	{
		LOG_ERROR("Failed to initialize engine.");
		return 0;
	}
	
	// Start game
	int result = engine.run();

	return result;
}