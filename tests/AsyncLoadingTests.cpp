#include "debug/Logging.h"
#include "resource/IResourceListener.h"
#include "resource/IResourceManager.h"
#include "resource/Resource.h"
#include "support/RejectUploadListener.h"
#include "support/TestResourceListener.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <lodepng.h>
#include <thread>

namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}

AsyncLoadStatus finish(IResourceManager &resources, AsyncLoadId id, bool publish = true)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    for (;;)
    {
        if (publish)
            resources.pumpAsyncLoads(0.1);
        auto status = resources.getLoadStatus(id);
        if (status.terminal() || (!publish && status.phase == LoadPhase::AwaitingUpload))
            return status;
        require(std::chrono::steady_clock::now() < deadline, "Async request did not complete");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
} // namespace
int main()
{
    try
    {
        logging::initialize();
        std::filesystem::create_directories("build/async-tests");
        const std::string dir = "build/async-tests/";
        require(lodepng::encode(dir + "image.png", std::vector<unsigned char>(18, 128), 3, 2,
                                LCT_RGB) == 0,
                "Image fixture failed");
        {
            std::ofstream f(dir + "mesh.obj");
            f << "v -1 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
        }
        {
            std::ofstream f(dir + "material.json");
            f << "{\"base\":\"" << dir << "image.png\",\"alpha_mode\":\"blend\",\"opacity\":0.4}";
        }
        {
            std::ofstream f(dir + "model.json");
            f << "{\"mesh\":\"" << dir << "mesh.obj\",\"material\":\"" << dir << "material.json\"}";
        }
        {
            std::ofstream f(dir + "scene.json");
            f << "{\"scene_objects\":[{\"mesh\":\"" << dir << "mesh.obj\",\"material\":\"" << dir
              << "material.json\"}]}";
        }
        TestResourceListener listener;
        RejectUploadListener reject;
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        resources->addResourceListener(&listener);
        // Force local worker IDs to collide with unrelated live IDs.
        resources->createImage({0, 0, 0}, 1, 1, EColorFormat::RGB24);
        resources->createMesh({0, 0, 0}, {}, {}, {}, EPrimitiveType::Point);
        const auto before = listener.events;
        auto id =
            resources->loadAsync({{AssetKind::Model, dir + "model.json"},
                                  {AssetKind::Scene, dir + "scene.json"},
                                  {AssetKind::Shader, "data/shader/deferred/geometry_pass.ini"},
                                  {AssetKind::Model, dir + "model.json"}});
        require(finish(*resources, id, false).phase == LoadPhase::AwaitingUpload,
                "CPU preparation failed");
        require(listener.events == before, "Worker published resources before the owner pumped");
        resources->pumpAsyncLoads(0.000000001);
        require(resources->getLoadStatus(id).published == 1,
                "Publication ignored the per-frame budget");
        // Publication must consume prepared data, not reopen/reparse source files.
        std::filesystem::remove(dir + "image.png");
        std::filesystem::remove(dir + "mesh.obj");
        std::filesystem::remove(dir + "material.json");
        std::filesystem::remove(dir + "model.json");
        std::filesystem::remove(dir + "scene.json");
        resources->setCacheOnly(true);
        require(finish(*resources, id).phase == LoadPhase::Ready, "Owner publication failed");
        const auto model = resources->loadModel(dir + "model.json");
        ResourceId mesh, material;
        require(resources->getModel(model, mesh, material) && mesh != 0,
                "Model mesh IDs were not remapped");
        const auto settings = resources->getMaterialSettings(material);
        require(settings.alphaMode == AlphaMode::Blend && settings.opacity == 0.4f,
                "Async material settings were lost");
        Json::Value scene;
        require(resources->loadJsonDocument(dir + "scene.json", scene) &&
                    scene["scene_objects"].size() == 1,
                "Scene JSON was not prepared");
        bool rejected = false;
        try
        {
            resources->loadMesh("build/async-tests/not-prepared.obj");
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "Render-thread cache miss silently performed synchronous I/O");
        rejected = false;
        try
        {
            resources->reloadShader(
                resources->loadShader("data/shader/deferred/geometry_pass.ini"));
        }
        catch (const std::logic_error &)
        {
            rejected = true;
        }
        require(rejected, "Synchronous shader reload bypassed cache-only mode");
        std::atomic<bool> wrongThreadRejected = false;
        std::thread wrong(
            [&]
            {
                try
                {
                    resources->loadMesh("anything");
                }
                catch (const std::logic_error &)
                {
                    wrongThreadRejected = true;
                }
            });
        wrong.join();
        require(wrongThreadRejected, "Live resource cache accepted cross-thread access");
        const auto eventCount = listener.events;
        const auto duplicate =
            resources->loadAsync({{AssetKind::Shader, "data/shader/deferred/geometry_pass.ini"}});
        require(finish(*resources, duplicate).phase == LoadPhase::Ready &&
                    listener.events == eventCount,
                "Duplicate async requests republished cached resources");
        const auto failure =
            resources->loadAsync({{AssetKind::Mesh, "build/async-tests/missing.obj"}});
        const auto failed = finish(*resources, failure);
        require(failed.phase == LoadPhase::Failed && !failed.error.empty(),
                "Missing asset did not report a failure");
        const auto cancelled =
            resources->loadAsync({{AssetKind::Scene, "data/world/intro_1.json"}});
        resources->cancelLoad(cancelled);
        require(finish(*resources, cancelled).phase == LoadPhase::Cancelled,
                "Cancelled request was published");
        resources->addResourceListener(&reject);
        reject.armed = true;
        const auto uploadFailure =
            resources->loadAsync({{AssetKind::Mesh, "data/mesh/screen_quad.obj"}});
        require(finish(*resources, uploadFailure).phase == LoadPhase::Failed,
                "Upload error did not reach the async status");
        const auto retry = resources->loadAsync({{AssetKind::Mesh, "data/mesh/screen_quad.obj"}});
        require(finish(*resources, retry).phase == LoadPhase::Ready,
                "Failed publication left a poisoned cache entry");
        resources->removeResourceListener(&reject);
        resources->removeResourceListener(&listener);
        // Destruction joins outstanding workers and cancels pending work safely.
        {
            std::unique_ptr<IResourceManager> pending(createResourceManager());
            pending->loadManifestAsync("data/game/defenders_of_cthedra/assets.json");
        }
        std::cout
            << "Async loading: CPU preparation, owner publication, dependency remapping, "
               "cache-only access, deduplication, cancellation, errors and shutdown passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
