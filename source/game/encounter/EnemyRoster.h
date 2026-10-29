#pragma once
#include "EnemyDefinition.h"
#include "EnemySpawn.h"
#include "game/arena/ArenaTuning.h"
#include <vector>
namespace Json
{
class Value;
}
class IResourceManager;
struct EnemyRoster
{
    std::vector<EnemyDefinition> types;
    std::vector<EnemySpawn> spawns;
    std::size_t maxActive = 24;
    float spawnSeparationScale = 1.4f;
    static EnemyRoster load(IResourceManager &resources, const ArenaTuning &arena,
                            const WeaponTuning &weapon);
    static EnemyRoster parse(const Json::Value &root, const ArenaTuning &arena,
                             const WeaponTuning &weapon);
};
