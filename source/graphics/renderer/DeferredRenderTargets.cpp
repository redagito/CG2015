#include "DeferredRenderTargets.h"
#include <algorithm>

void ShadowTargets::prune()
{
    std::erase_if(directional,[this](const auto& entry){return !renderedDirectional.contains(entry.first);});
    std::erase_if(point,[this](const auto& entry){return !renderedPoint.contains(entry.first);});
}
