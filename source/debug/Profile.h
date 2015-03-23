#pragma once

#if defined(_DEBUG) || defined(DEBUG)

#include "CProfiler.h"
#define START_PROFILE(x) CProfiler x_profiler(#x)

#else

#define START_PROFILE(x)

#endif