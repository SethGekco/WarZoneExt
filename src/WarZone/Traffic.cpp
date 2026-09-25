#include "WarZone/Traffic.h"
#include "WarZone/Zones.h"
#include "WarZone/Config.h"

#include <UnitClass.h>
#include <UnitTypeClass.h>
#include <InfantryClass.h>
#include <HouseClass.h>
#include <Unsorted.h>
#include <Utilities/Debug.h>

namespace
{
	int g_lastSample = -1;
	long long g_samples = 0;

	bool Movable(FootClass* const pFoot)
	{
		return pFoot && pFoot->IsAlive && !pFoot->InLimbo
			&& pFoot->Destination != nullptr; // moving with intent
	}

	void Sample(FootClass* const pFoot, bool const harvester)
	{
		if (!Movable(pFoot))
			return;
		auto const cell = pFoot->GetMapCoords();
		Zones::Add("Traffic", cell.X, cell.Y);
		if (harvester)
			Zones::Add("MinerTravel", cell.X, cell.Y);
		++g_samples;
	}
}

void Traffic::Reset()
{
	g_lastSample = -1;
	g_samples = 0;
}

void Traffic::MaybeSample()
{
	auto const& cfg = WarZoneConfig::Instance;
	if (cfg.TrafficInterval <= 0)
		return;

	int const frame = Unsorted::CurrentFrame;
	if (g_lastSample >= 0 && frame - g_lastSample < cfg.TrafficInterval)
		return;
	g_lastSample = frame;

	for (auto const pUnit : UnitClass::Array)
	{
		auto const pType = pUnit ? static_cast<UnitTypeClass*>(pUnit->GetTechnoType()) : nullptr;
		Sample(pUnit, pType && pType->Harvester);
	}
	for (auto const pInf : InfantryClass::Array)
		Sample(pInf, false);

	if (cfg.DebugTicks)
		Debug::Log("[WarZoneExt] traffic sampled at f%d (%lld movement sample(s) so far)\n",
			frame, g_samples);
}
