#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
#include "game/CGameWorld.h"
#include "game/combat/CPlayerCombatUpgrades.h"
#include "game/control/CHealthController.h"
#include "game/control/CProjectileController.h"
#include "game/control/CRemoveOnDeathController.h"
#include "game/progression/CPlayerProgression.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main()
{
    try
    {
        CCollisionSystem collisions;
        const auto friendly = collisions.getNewGroupId(), hostile = collisions.getNewGroupId();
        CGameWorld world;
        auto *player = new CGameObject();
        player->setCollidable(collisions.add(CAABBox::create({-1, -1, -1, 1, 1, 1}), friendly));
        auto health = std::make_shared<CHealthController>(200, nullptr, nullptr,
                                                          DefenseTuning{1, .2f, 60, 10, 3});
        player->addController(health);
        world.addObject(player);
        player->getCollidable()->receiveDamage(100);
        world.update(.016f);
        require(health->health() == 180 && health->shield() == 0,
                "Armor/shield did not absorb damage correctly");
        for (int i = 0; i < 8; ++i)
            world.update(.25f);
        require(health->health() == 180 && health->shield() == 0,
                "Repair started before its delay");
        for (int i = 0; i < 8; ++i)
            world.update(.25f);
        require(health->health() == 180 && health->shield() == 0,
                "Recovery started before the new independent delays");
        for (int i = 0; i < 4; ++i)
            world.update(.25f);
        require(health->health() == 180 && health->shield() > 0,
                "Shield recharge did not resume independently of hull repair");
        for (int i = 0; i < 12; ++i)
            world.update(.25f);
        require(health->health() > 180, "Slow hull repair did not resume after seven seconds");
        CPlayerCombatUpgrades upgrades(world, *health, nullptr, {10, .1f, 100, 10});
        health->heal(1000);
        require(health->health() == 200, "Healing exceeded hull capacity");
        player->getCollidable()->receiveDamage(100);
        world.update(.016f);
        const float before = health->health();
        int killed = 0;
        const auto enemy = [&](glm::vec3 position)
        {
            auto *object = new CGameObject();
            object->setPosition(position);
            object->setTargetName("ENEMY");
            object->setCollidable(collisions.add(CAABBox::create({-1, -1, -1, 1, 1, 1}), hostile));
            auto hp = std::make_shared<CHealthController>(100);
            object->addController(hp);
            object->addController(std::make_shared<CRemoveOnDeathController>(
                [&, object]
                {
                    ++killed;
                    upgrades.onKill(object->getPosition(),
                                    object->getCollidable()->fatalDamageKind());
                }));
            const auto id = world.addObject(object);
            return id;
        };
        const auto primary = enemy({0, 0, 10}), near = enemy({2, 0, 10}),
                   outside = enemy({100, 0, 10}), chainEdge = enemy({11, 0, 10});
        world.update(0);
        auto *primaryCollider = world.getObject(primary)->getCollidable();
        const float dealt = primaryCollider->receiveDamage(40);
        upgrades.onHit({0, 0, 10}, dealt, 8, 40, primaryCollider->id());
        upgrades.update();
        world.update(.016f);
        require(health->health() > before, "Lifesteal did not repair the player");
        require(world.getObject(primary) && world.getObject(near) && world.getObject(outside),
                "Nonlethal splash killed an enemy unexpectedly");
        upgrades.onKill({0, 0, 10});
        upgrades.update();
        world.update(.016f);
        require(!world.getObject(primary) && !world.getObject(near) && world.getObject(outside) &&
                    killed == 2,
                "Chain blast missed nearby targets, hit distant targets, or repeated a kill");
        upgrades.update();
        world.update(.016f);
        require(killed == 2 &&
                    world.getObject(chainEdge)->getCollidable()->remainingHealthBudget() == 100,
                "An area kill recursively triggered another chain blast");
        // Credit only hull actually removed after armor/shields and all queued hits.
        CCollidable credited(0, CAABBox());
        credited.setHealthBudget(100, 20, .2f);
        require(credited.receiveDamage(25) == 0 && credited.receiveDamage(50) == 40,
                "Shield or armor damage incorrectly grants lifesteal");
        require(credited.receiveDamage(1000, {}, DamageKind::Chain) == 60 &&
                    credited.receiveDamage(1000) == 0 &&
                    credited.fatalDamageKind() == DamageKind::Chain,
                "Overkill was credited or replaced the real lethal hit's origin");
        {
            CCollisionSystem hitCollisions;
            const auto ownGroup = hitCollisions.getNewGroupId(),
                       enemyGroup = hitCollisions.getNewGroupId();
            CGameWorld hitWorld;
            auto *pilot = new CGameObject();
            pilot->setCollidable(hitCollisions.add(CAABBox(), ownGroup));
            auto pilotHealth = std::make_shared<CHealthController>(1000);
            pilot->addController(pilotHealth);
            hitWorld.addObject(pilot);
            pilot->getCollidable()->receiveDamage(500);
            hitWorld.update(0);
            auto *victim = new CGameObject();
            victim->setPosition({0, 0, 10});
            victim->setCollidable(
                hitCollisions.add(CAABBox::create({-1, -1, -1, 1, 1, 1}), enemyGroup));
            victim->addController(std::make_shared<CHealthController>(10));
            hitWorld.addObject(victim);
            CPlayerCombatUpgrades siphon(hitWorld, *pilotHealth, nullptr, {0, .1f, 0, 0, 0, 5});
            for (int shot = 0; shot < 2; ++shot)
            {
                CGameObject bullet;
                bullet.setCollidable(hitCollisions.add(CAABBox(), ownGroup));
                bullet.getCollidable()->setCollidable(false);
                bullet.getCollidable()->setDamage(1000);
                CProjectileController projectile({0, 0, 100}, 2, hitCollisions, ownGroup, nullptr,
                                                 true, {1, 1, 1}, {}, &siphon);
                projectile.attach(&bullet);
                projectile.update(.2f);
                require(std::abs(pilotHealth->health() - 501) < .001f,
                        "Real projectiles grant lifesteal for overkill or repeated pending kills");
            }
        }
        for (int fps : {30, 60, 144})
        {
            CGameWorld recoveryWorld;
            CCollidable receiver(0, CAABBox());
            auto *pilot = new CGameObject();
            pilot->setCollidable(&receiver);
            auto recovery = std::make_shared<CHealthController>(1000);
            pilot->addController(recovery);
            recoveryWorld.addObject(pilot);
            receiver.receiveDamage(500);
            recoveryWorld.update(0);
            CPlayerCombatUpgrades limited(recoveryWorld, *recovery, nullptr, {10, .5f, 0, 0, 6, 4});
            for (int i = 0; i < 100; ++i)
            {
                limited.onHit({}, 1000, 0, 0, 0);
                limited.onKill({});
            }
            require(recovery->health() == 510,
                    "Pellets/kills bypassed the initial healing allowances");
            limited.onKill({}, DamageKind::Splash);
            limited.onKill({}, DamageKind::Chain);
            limited.onHit({}, 0, 0, 0, 0);
            require(recovery->health() == 510, "Area kills or zero hull damage grant healing");
            for (int frame = 0; frame < fps; ++frame)
            {
                limited.update(1.f / fps);
                limited.onHit({}, 1000, 0, 0, 0);
                limited.onKill({});
            }
            require(std::abs(recovery->health() - 520) < .02f,
                    "Healing rates depend on frame rate or exceed their per-second limits");
        }
        {
            CPlayerProgression maximum;
            for (int i = 0; i < 500; ++i)
                maximum.awardKill();
            for (const auto &skill : maximum.skills())
                for (int rank = 0; rank < 5; ++rank)
                    maximum.buy(skill.id);
            CGameWorld dangerWorld;
            CCollidable receiver(0, CAABBox());
            auto *pilot = new CGameObject();
            pilot->setCollidable(&receiver);
            const auto defense = maximum.defenses();
            auto vulnerable = std::make_shared<CHealthController>(300 * defense.hullMultiplier,
                                                                  nullptr, nullptr, defense);
            pilot->addController(vulnerable);
            dangerWorld.addObject(pilot);
            CPlayerCombatUpgrades bestRecovery(dangerWorld, *vulnerable, nullptr,
                                               maximum.combatUpgrades());
            for (int frame = 0; frame < 100 && !pilot->isDead(); ++frame)
            {
                receiver.receiveDamage(12);
                dangerWorld.update(.1f);
                bestRecovery.update(.1f);
                bestRecovery.onHit({}, 10000, 0, 0, 0);
                bestRecovery.onKill({});
            }
            require(pilot->isDead(), "Maximum defenses plus unlimited kill/hit opportunities make "
                                     "sustained fire harmless");
        }
        // Sweeps cross multiple targets in one step, but never hit one receiver twice.
        CCollisionSystem piercing;
        const auto shots = piercing.getNewGroupId(), targets = piercing.getNewGroupId();
        auto *a = piercing.add(CAABBox::create({-1, -1, 9, 1, 1, 11}), targets);
        auto *b = piercing.add(CAABBox::create({-1, -1, 19, 1, 1, 21}), targets);
        auto *c = piercing.add(CAABBox::create({-1, -1, 29, 1, 1, 31}), targets);
        CGameObject bullet;
        bullet.setCollidable(piercing.add(CAABBox(), shots));
        bullet.getCollidable()->setCollidable(false);
        bullet.getCollidable()->setDamage(30);
        CProjectileController projectile({0, 0, 200}, 2, piercing, shots, nullptr, true, {1, 1, 1},
                                         ProjectileModifiers{2, 0, 0});
        projectile.attach(&bullet);
        projectile.update(.2f);
        require(a->getDamageReceived() == 30 && b->getDamageReceived() == 30 &&
                    c->getDamageReceived() == 0 && bullet.isDeleteRequested(),
                "Piercing did not stop after two distinct targets");
        const auto removedId = a->id();
        piercing.remove(a);
        auto *replacement = piercing.add(CAABBox::create({-1, -1, 4, 1, 1, 6}), targets);
        const std::uint64_t excluded[] = {removedId};
        require(piercing.trace({0, 0, 0}, {0, 0, 10}, {0, 0, 0}, shots, 1, excluded).entity ==
                    replacement,
                "Broadphase retained a freed collider or reused an excluded ID");
        piercing.beginStep();
        replacement->setTranslation({100, 0, 0});
        require(!piercing.trace({0, 0, 0}, {0, 0, 10}, {0, 0, 0}, shots).entity,
                "Moving receiver did not invalidate the collision index");
        player->getCollidable()->receiveDamage(10000);
        world.update(.016f);
        health->heal(1000);
        require(player->isDead() && health->health() == 0, "Repair resurrected a dead player");
        std::cout << "Combat upgrades: shield/armor/repair, lifesteal, splash/chains, piercing and "
                     "collision-index lifetime passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
