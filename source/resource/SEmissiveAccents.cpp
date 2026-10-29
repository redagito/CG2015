#include "SEmissiveAccents.h"
#include <cmath>
#include <json/json.h>

bool SEmissiveAccents::load(const Json::Value &root)
{
    if (!root.isObject())
        return false;
    SEmissiveAccents result;
    const auto number = [&](const char *key, float &value)
    {
        if (!root[key].isNumeric())
            return false;
        value = root[key].asFloat();
        return std::isfinite(value);
    };
    const auto vector = [&](const char *key, float *values, unsigned count)
    {
        const auto &array = root[key];
        if (!array.isArray() || array.size() != count)
            return false;
        for (unsigned i = 0; i < count; ++i)
        {
            if (!array[i].isNumeric())
                return false;
            values[i] = array[i].asFloat();
        }
        return true;
    };
    if (!vector("color", &result.color.x, 3) || !vector("hull_z_range", &result.hullZRange.x, 2) ||
        !number("strength", result.strength) ||
        !number("hull_strip_offset", result.hullStripOffset) ||
        !number("hull_strip_width", result.hullStripWidth) ||
        !number("engine_z_max", result.engineZMax) ||
        !number("engine_half_width", result.engineHalfWidth) ||
        !number("engine_center_y", result.engineCenterY) ||
        !number("engine_half_height", result.engineHalfHeight) || !result.valid())
        return false;
    *this = result;
    return true;
}
bool SEmissiveAccents::valid() const
{
    for (float value :
         {color.x, color.y, color.z, strength, hullStripOffset, hullStripWidth, hullZRange.x,
          hullZRange.y, engineZMax, engineHalfWidth, engineCenterY, engineHalfHeight})
        if (!std::isfinite(value))
            return false;
    return glm::all(glm::greaterThanEqual(color, glm::vec3(0))) &&
           glm::all(glm::lessThanEqual(color, glm::vec3(1))) && strength >= 0 && strength <= 8 &&
           hullStripOffset >= 0 && hullStripWidth > 0 && hullZRange.y > hullZRange.x &&
           engineHalfWidth > 0 && engineHalfHeight > 0;
}
