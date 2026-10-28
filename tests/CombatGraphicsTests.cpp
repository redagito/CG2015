#include "game/combat/CombatTuning.h"
#include "game/combat/ProjectileBallistics.h"
#include "game/control/CWeaponController.h"
#include "game/encounter/CEnemySpawner.h"
#include "game/encounter/EncounterProgress.h"
#include "graphics/resource/CGraphicsResourceManager.h"
#include "graphics/scene/CScene.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include "support/TestInput.h"
#include <chrono>
#include <cstdio>
#include <glm/ext.hpp>
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
} // namespace
bool runCombatGraphicsTests(const CGlfwWindow &)
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        CGraphicsResourceManager graphics;
        resources->addResourceListener(&graphics);
        const auto mesh = resources->loadMesh("data/mesh/bullet.obj"),
                   material = resources->loadMaterial("data/material/white.json");
        const auto tuning = CombatTuning::load(*resources);
        CScene scene(&graphics);
        CGameWorld world;
        TestInput input;
        CCollisionSystem collisions;
        const auto group = collisions.getNewGroupId();
        collisions.getNewGroupId();
        CGameObject ship;
        ship.setPosition({10, 20, 30});
        ship.setScale({.5f, .5f, .5f});
        ship.setOrientation(glm::quat(glm::vec3(.4f, .8f, .6f)));
        ship.setVelocity({20, 5, -4});
        CWeaponController weapon(&input, &world, &scene, mesh, material, resources.get(),
                                 &collisions, group, nullptr, tuning.weapon);
        weapon.attach(&ship);
        const auto aim = weapon.aim();
        input.space = true;
        weapon.update(.016f);
        require(!world.getObject(1), "Brake fired a projectile");
        input.fire = true;
        weapon.update(.016f);
        const auto *first = world.getObject(1);
        require(first, "Primary fire produced no projectile");
        require(glm::distance(first->getPosition(), aim.muzzle) < .001f,
                "Projectile did not spawn at the transformed muzzle");
        require(glm::distance(first->getVelocity(),
                              aim.inheritedVelocity + aim.forward * aim.speed) < .001f,
                "Weapon did not use inherited-velocity ballistics");
        ResourceId meshId, materialId;
        glm::vec3 position, scale;
        glm::quat orientation;
        bool visible;
        require(scene.getObject(0, meshId, materialId, position, orientation, scale, visible) &&
                    glm::distance(position, aim.muzzle) < .001f,
                "Projectile first appeared at a stale scene transform");
        const auto start = first->getPosition(), velocity = first->getVelocity();
        world.update(.1f);
        require(glm::distance(world.getObject(1)->getPosition(), start + velocity * .1f) < .001f,
                "Projectile movement disagrees with lead math");
        for (int i = 0; i < 20; ++i)
            weapon.update(.016f);
        require(world.getObject(2) && glm::distance(world.getObject(2)->getPosition(),
                                                    ProjectileBallistics::muzzlePosition(
                                                        ship, tuning.weapon.muzzles[1])) < .001f,
                "Guns did not alternate between the two muzzle sockets");
        for (int frame = 0; frame < 45; ++frame)
            world.update(.1f);
        require(!world.getObject(1) && !world.getObject(2),
                "Expired projectiles remained in the world");
        collisions.update();
        require(!scene.getObject(0, meshId, materialId, position, orientation, scale, visible),
                "Expired projectile leaked its scene object");
        const auto reused =
            scene.createObject(mesh, material, glm::vec3(0), glm::quat(1, 0, 0, 0), glm::vec3(1));
        require(reused < 2, "Scene storage grew instead of reusing an expired projectile slot");
        CGameObject hostile;
        hostile.setScale(glm::vec3(.5f));
        auto enemyTuning = tuning.weapon;
        enemyTuning.damage = 12;
        enemyTuning.interval = 1;
        CWeaponController enemyWeapon(nullptr, &world, &scene, mesh, material, resources.get(),
                                      &collisions, 1, nullptr, enemyTuning);
        enemyWeapon.attach(&hostile);
        CAABBox playerBox;
        playerBox.setMid(enemyWeapon.aim().muzzle + glm::vec3(0, 0, 10));
        playerBox.setHalfWidths(glm::vec3(1));
        auto *playerCollider = collisions.add(playerBox, group);
        enemyWeapon.setTrigger(true);
        enemyWeapon.update(.016f);
        require(world.getObject(3), "AI trigger did not fire through the shared weapon path");
        world.update(.25f);
        require(playerCollider->getDamageReceived() == 12 && !world.getObject(3),
                "Enemy projectile did not hit the player group exactly once");
        for (int fps : {30, 60, 144})
        {
            CGameWorld cadenceWorld;
            CCollisionSystem cadenceCollisions;
            cadenceCollisions.getNewGroupId();
            CWeaponController cadence(&input, &cadenceWorld, &scene, mesh, material,
                                      resources.get(), &cadenceCollisions, 0, nullptr,
                                      tuning.weapon);
            cadence.attach(&ship);
            for (int frame = 0; frame < fps * 2; ++frame)
                cadence.update(1.f / fps);
            require(cadenceWorld.addObject(new CGameObject()) == 8,
                    "Weapon cadence changes with frame rate");
        }
        const auto load = resources->loadAsync(
            {{AssetKind::EnemyRoster, "data/game/defenders_of_cthedra/encounters.json"}});
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        for (;;)
        {
            resources->pumpAsyncLoads(2);
            const auto status = resources->getLoadStatus(load);
            require(status.phase != LoadPhase::Failed, status.error.c_str());
            if (status.phase == LoadPhase::Ready)
                break;
            require(std::chrono::steady_clock::now() < deadline,
                    "Enemy roster GPU preparation timed out");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        resources->setCacheOnly(true);
        Json::Value encounter;
        require(resources->loadJsonDocument("data/game/defenders_of_cthedra/encounters.json",
                                            encounter),
                "Prepared encounter missing");
        encounter["max_active"] = 4;
        encounter["waves"][1]["count"] = 36;
        for (auto &wave : encounter["waves"])
        {
            wave["delay"] = 0;
            wave["interval"] = 0;
        }
        const auto roster = EnemyRoster::parse(encounter, ArenaTuning{}, tuning.weapon);
        EncounterProgress progress{roster.spawns.size(), 0};
        CCollisionSystem spawnCollisions;
        const auto playerGroup = spawnCollisions.getNewGroupId(),
                   enemyGroup = spawnCollisions.getNewGroupId();
        CGameWorld spawnWorld;
        auto *observer = new CGameObject();
        observer->setPosition({0, 70, -130});
        const auto observerId = spawnWorld.addObject(observer);
        CEnemySpawner spawner(roster, ArenaTuning{}, spawnWorld, scene, *resources, spawnCollisions,
                              observerId, enemyGroup, mesh, material, nullptr,
                              [&] { progress.recordDefeat(); });
        spawner.update(0);
        require(spawnWorld.targets().size() == 4 && spawner.pending() == 33,
                "Active spawn limit or arbitrary wave count was ignored");
        for (int batch = 0; batch < 20 && !progress.complete(); ++batch)
        {
            const auto targets = spawnWorld.targets();
            require(targets.size() <= 4, "Spawner exceeded its active budget");
            for (const auto &target : targets)
                spawnWorld.getObject(target.id)->getCollidable()->receiveDamage(100000);
            spawnWorld.update(.016f);
            spawnCollisions.update();
            spawner.update(0);
            require(progress.defeated + spawner.pending() + spawnWorld.targets().size() ==
                        progress.total,
                    "Pending enemies disappeared from encounter progress");
        }
        require(progress.total == 37 && progress.complete() && spawner.pending() == 0,
                "Generic spawner failed to complete a larger encounter");
        std::puts("Combat GPU integration: transformed alternating muzzles, initial render pose, "
                  "brake/fire separation, inherited motion and expiry passed.");
        return true;
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "Combat GPU regression: %s\n", e.what());
        return false;
    }
}
