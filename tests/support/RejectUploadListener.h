#pragma once
#include "resource/IResourceListener.h"

class RejectUploadListener : public IResourceListener
{
  public:
    bool armed = false;
    void onAttach(IResourceManager *) override;
    void onDetach(IResourceManager *) override;
    void notify(EResourceType, ResourceId, EListenerEvent, IResourceManager *) override;
};
