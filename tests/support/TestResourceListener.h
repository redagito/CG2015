#pragma once
#include "resource/IResourceListener.h"
#include <thread>

class TestResourceListener : public IResourceListener
{
  public:
    std::thread::id owner = std::this_thread::get_id();
    unsigned events = 0;
    void onAttach(IResourceManager *) override;
    void onDetach(IResourceManager *) override;
    void notify(EResourceType, ResourceId, EListenerEvent, IResourceManager *) override;
};
