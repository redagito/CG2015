#include "AsyncLoadStatus.h"
bool AsyncLoadStatus::terminal() const
{
    return phase == LoadPhase::Ready || phase == LoadPhase::Failed || phase == LoadPhase::Cancelled;
}
