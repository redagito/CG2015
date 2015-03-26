#pragma once

#if defined(_DEBUG) || defined(DEBUG)

#include "CProfiler.h"
#define PROFILE_START(x) CProfiler x_profiler(#x)

#else

#define PROFILE_START(x)

#endif