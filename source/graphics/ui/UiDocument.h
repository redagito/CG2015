#pragma once
#include "UiRect.h"
#include "UiText.h"
#include <vector>

struct UiDocument
{
    std::vector<UiRect> rectangles;
    std::vector<UiText> texts;
    bool empty() const;
};
