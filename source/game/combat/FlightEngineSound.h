#pragma once
#include "EngineSoundLevels.h"
#include "EngineSoundTuning.h"
struct FlightState;
class FlightEngineSound
{
  public:
    static EngineSoundLevels levels(const FlightState &flight, const EngineSoundTuning &tuning);
};
