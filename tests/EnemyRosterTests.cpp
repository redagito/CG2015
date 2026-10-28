#include "game/arena/CEnemyNeighborhood.h"
#include "game/encounter/EncounterProgress.h"
#include "game/encounter/EnemyRoster.h"
#include "resource/Resource.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json/json.h>
#include <stdexcept>
#include <thread>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
void reject(const Json::Value &root)
{
    bool failed = false;
    try
    {
        EnemyRoster::parse(root, ArenaTuning{}, WeaponTuning{});
    }
    catch (const std::runtime_error &)
    {
        failed = true;
    }
    require(failed, "Invalid enemy roster was accepted");
}
} // namespace
int main()
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        Json::Value root;
        require(resources->loadJsonDocument("data/game/defenders_of_cthedra/encounters.json", root),
                "Enemy roster fixture missing");
        auto interceptor = root["enemy_types"][1];
        interceptor["id"] = "interceptor";
        interceptor["name"] = "INTERCEPTOR";
        interceptor["mesh"] = "data/mesh/ship_2.obj";
        interceptor["material"] = "data/material/white.json";
        interceptor["health"] = 175;
        interceptor["behavior"]["speed"] = 34;
        interceptor["behavior"]["turnRate"] = 95;
        interceptor["weapon"]["interval"] = .6;
        interceptor["weapon"]["damage"] = 7;
        root["enemy_types"].append(interceptor);
        root["max_active"] = 12;
        root["waves"][1]["type"] = "interceptor";
        root["waves"][1]["count"] = 136;
        root["waves"][1]["interval"] = .2;
        const auto roster = EnemyRoster::parse(root, ArenaTuning{}, WeaponTuning{});
        require(roster.types.size() == 3 && roster.spawns.size() == 137 && roster.maxActive == 12,
                "Encounter still has a fixed enemy roster or count");
        require(roster.types[2].health == 175 && roster.types[2].behavior.speed == 34 &&
                    roster.types[2].weapon.damage == 7,
                "Per-type settings were ignored");
        require(glm::distance(roster.spawns[8].offset, roster.spawns[14].offset) > .1f,
                "Procedural spawns still repeat six formation slots");
        EncounterProgress progress{roster.spawns.size(), 0};
        for (std::size_t i = 0; i < 136; ++i)
            progress.recordDefeat();
        require(!progress.complete(), "Victory ignored pending configured enemies");
        progress.recordDefeat();
        require(progress.complete(), "Dynamic encounter completion failed");
        auto invalid = root;
        invalid["waves"][1]["type"] = "missing";
        reject(invalid);
        invalid = root;
        invalid["enemy_types"].append(interceptor);
        reject(invalid);
        invalid = root;
        invalid["waves"][1]["interval"] = -1;
        reject(invalid);
        invalid = root;
        invalid["max_active"] = 0;
        reject(invalid);
        auto empty = root;
        for (auto &wave : empty["waves"])
            wave["count"] = 0;
        require(EnemyRoster::parse(empty, ArenaTuning{}, WeaponTuning{}).spawns.empty(),
                "Disabling a wave with count zero failed");
        require(!EncounterProgress{}.complete(), "An empty encounter won immediately");

        std::filesystem::create_directories("build/enemy-roster-tests");
        const std::string file = "build/enemy-roster-tests/encounters.json";
        {
            std::ofstream out(file);
            out << root;
        }
        // Start with an empty manager so the normal gameplay manifest cannot hide a missing
        // dependency.
        std::unique_ptr<IResourceManager> prepared(createResourceManager());
        const auto request = prepared->loadAsync({{AssetKind::EnemyRoster, file}});
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        for (;;)
        {
            prepared->pumpAsyncLoads(2);
            const auto status = prepared->getLoadStatus(request);
            require(status.phase != LoadPhase::Failed, status.error.c_str());
            if (status.phase == LoadPhase::Ready)
                break;
            require(std::chrono::steady_clock::now() < deadline,
                    "Enemy asset preparation timed out");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        prepared->setCacheOnly(true);
        for (const auto &type : roster.types)
        {
            require(prepared->loadMesh(type.mesh) != invalidResource &&
                        prepared->loadMaterial(type.material) != invalidResource,
                    "New enemy type was not prepared asynchronously");
            if (!type.decoration.mesh.empty())
                require(prepared->loadMesh(type.decoration.mesh) != invalidResource &&
                            prepared->loadMaterial(type.decoration.material) != invalidResource,
                        "Enemy decoration was not prepared asynchronously");
        }
        CEnemyNeighborhood neighborhood;
        std::vector<TargetSnapshot> fleet;
        for (int i = 0; i < 1000; ++i)
            fleet.push_back(
                {GameObjectId(i + 1), "SHIP", {float(i * 20 - 10000), 0, 0}, {0, 0, 0}});
        neighborhood.rebuild(fleet);
        const auto near = neighborhood.nearby({0, 0, 0}, 45);
        require(near.size() == 5,
                "Spatial neighbor queries returned the whole fleet or missed nearby enemies");
        neighborhood.rebuild({{1, "A", {-64.1f, 0, 0}, {}}, {2, "B", {-63.9f, 0, 0}, {}}});
        require(neighborhood.nearby({-64, 0, 0}, .3f).size() == 2,
                "Spatial queries fail across negative cell boundaries");
        std::cout << "Enemy roster: arbitrary types/counts, wave validation, async dependencies, "
                     "dynamic victory and 1000-ship spatial queries passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Enemy roster regression: " << e.what() << '\n';
        return 1;
    }
}
