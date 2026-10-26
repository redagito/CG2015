#include "RejectUploadListener.h"
#include <stdexcept>
void RejectUploadListener::onAttach(IResourceManager *)
{
}
void RejectUploadListener::onDetach(IResourceManager *)
{
}
void RejectUploadListener::notify(EResourceType type, ResourceId, EListenerEvent event,
                                  IResourceManager *)
{
    if (armed && type == EResourceType::Mesh && event == EListenerEvent::Create)
    {
        armed = false;
        throw std::runtime_error("Simulated upload failure");
    }
}
