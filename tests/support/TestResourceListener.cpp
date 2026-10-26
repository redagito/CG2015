#include "TestResourceListener.h"
#include "resource/IResourceManager.h"
#include <stdexcept>
void TestResourceListener::onAttach(IResourceManager *)
{
}
void TestResourceListener::onDetach(IResourceManager *)
{
}
void TestResourceListener::notify(EResourceType type, ResourceId id, EListenerEvent,
                                  IResourceManager *resources)
{
    if (std::this_thread::get_id() != owner)
        throw std::runtime_error("Resource callback ran on a worker");
    ++events;
    if (type == EResourceType::Material)
    {
        ResourceId base, normal, spec, glow, alpha;
        if (!resources->getMaterial(id, base, normal, spec, glow, alpha))
            throw std::runtime_error("Material unavailable during publication");
        std::vector<unsigned char> bytes;
        unsigned width, height;
        EColorFormat format;
        if (!resources->getImage(base, bytes, width, height, format) || width != 3 || height != 2)
            throw std::runtime_error("Material published before its image dependency");
    }
}
