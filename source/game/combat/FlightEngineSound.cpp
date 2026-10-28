#include "FlightEngineSound.h"
#include "game/flight/FlightState.h"
#include <algorithm>
#include <glm/common.hpp>
EngineSoundLevels FlightEngineSound::levels(const FlightState &flight,
                                            const EngineSoundTuning &tuning)
{
    const float load = flight.braking ? 0 : std::clamp(flight.throttle, 0.f, 1.f);
    const float boost = flight.braking ? 0 : std::clamp(flight.boost, 0.f, 1.f);
    return {glm::mix(glm::mix(tuning.idleGain, tuning.fullGain, load), tuning.boostGain, boost),
            glm::mix(glm::mix(tuning.idlePitch, tuning.fullPitch, load), tuning.boostPitch, boost)};
}
