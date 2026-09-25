#include "WarZone/Recorder.h"
#include "WarZone/Zones.h"
#include "WarZone/Config.h"

#include <TechnoClass.h>
#include <UnitClass.h>
#include <UnitTypeClass.h>
#include <HouseClass.h>
#include <Unsorted.h>
#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

#include <cstdio>

namespace
{
	int g_kills = 0;
	int g_minerDeaths = 0;

	// Harvesters are UnitTypes with Harvester=yes. Slaves/miners of other
	// shapes that aren't flagged simply won't count — better to under-report
	// than to guess at what "is a miner" means.
	bool IsHarvester(TechnoClass* const pTechno)
	{
		if (!pTechno || pTechno->WhatAmI() != AbstractType::Unit)
			return false;
		auto const pType = static_cast<UnitTypeClass*>(pTechno->GetTechnoType());
		return pType && pType->Harvester;
	}

	// A house's start position, or -1 (observer/neutral/no slot).
	int SpawnOf(TechnoClass* const pTechno)
	{
		if (!pTechno || !pTechno->Owner)
			return -1;
		return pTechno->Owner->GetSpawnPosition();
	}

	void AddLane(int const spawn, const char* const suffix, int const x, int const y)
	{
		if (spawn < 0)
			return;
		char zone[32];
		std::snprintf(zone, sizeof(zone), "Lane.%d.%s", spawn, suffix);
		Zones::Add(zone, x, y);
	}
}

void Recorder::Reset()
{
	g_kills = 0;
	g_minerDeaths = 0;
}

// TechnoClass::RegisterDestruction entry — ECX = dying object, [ESP+4] = killer.
// DoctrineExt and DossierExt already sit on this address; same-address hooks
// chain legally and we only read, so ordering doesn't matter.
DEFINE_HOOK(0x702D40, WarZoneExt_RegisterDestruction_Record, 0x5)
{
	GET(TechnoClass* const, pVictim, ECX);
	GET_STACK(TechnoClass* const, pKiller, 0x4);

	auto const& cfg = WarZoneConfig::Instance;
	if (!cfg.Parsed || !cfg.Enabled || !pVictim)
		return 0;

	auto const cell = pVictim->GetMapCoords();

	// Where things die. Recorded for EVERY house — AI-vs-AI teaches the map too,
	// which is most of why this layer learns faster than per-player habits.
	Zones::Add("Kill", cell.X, cell.Y);
	++g_kills;

	if (IsHarvester(pVictim))
	{
		Zones::Add("MinerDeath", cell.X, cell.Y);
		++g_minerDeaths;
		if (cfg.DebugTicks)
			Debug::Log("[WarZoneExt] miner died at %d,%d (bucket %s) — %d this match\n",
				cell.X, cell.Y, Zones::BucketKey(cell.X, cell.Y).c_str(), g_minerDeaths);
	}

	// Directional lanes, from the one event. Only count a lane when the two
	// sides are actually hostile, so friendly fire and civilian casualties
	// don't paint a false attack route.
	if (pKiller && pKiller->Owner && pVictim->Owner
		&& !pKiller->Owner->IsAlliedWith(pVictim->Owner))
	{
		AddLane(SpawnOf(pKiller), "Out", cell.X, cell.Y);
		AddLane(SpawnOf(pVictim), "In", cell.X, cell.Y);
	}

	return 0;
}
