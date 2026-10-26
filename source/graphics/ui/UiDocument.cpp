#include "UiDocument.h"

bool UiDocument::empty() const
{
    return rectangles.empty() && texts.empty();
}
