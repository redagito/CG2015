#include "EncounterProgress.h"
void EncounterProgress::recordDefeat()
{
    if (defeated < total)
        ++defeated;
}
bool EncounterProgress::complete() const
{
    return total > 0 && defeated >= total;
}
