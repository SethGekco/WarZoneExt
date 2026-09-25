#include "WarZone/Engine.h"
#include "WarZone/Config.h"
#include "WarZone/Zones.h"
#include "WarZone/MapId.h"

#include <HouseClass.h>
#include <MapClass.h>
#include <Unsorted.h>
#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

namespace
{
	bool g_opened = false;
	int g_lastSaveFrame = 0;
	int g_humans = 0;
}

void Engine::Reset()
{
	g_opened = false;
	g_lastSaveFrame = 0;
	g_humans = 0;
}

void Engine::TickHouse(HouseClass* const pHouse)
{
	auto const& cfg = WarZoneConfig::Instance;
	if (!cfg.Parsed || !cfg.Enabled || !pHouse)
		return;

	if (!g_opened)
	{
		g_opened = true;

		Zones::OpenForCurrentMap();

		auto const& b = MapClass::Instance.MapCoordBounds;
		Zones::SetFingerprint(b.Right - b.Left + 1, b.Bottom - b.Top + 1, MapId::SpawnCount());

		// MP policy: recording is synced-safe and always on, but stored history
		// differs per client, so consumers must not feed it into sim decisions
		// with more than one human. Report the verdict once.
		g_humans = 0;
		for (int i = 0; i < HouseClass::Array.Count; ++i)
		{
			auto const pH = HouseClass::Array.GetItem(i);
			if (pH && pH->IsHumanPlayer && !pH->IsObserver() && !pH->IsNeutral())
				++g_humans;
		}
		bool const usable = g_humans <= 1 || cfg.UseHistoryInMultiplayer;
		Debug::Log("[WarZoneExt] %d human(s): stored history is %s for sim-affecting use "
			"(recording is always on and MP-safe).\n",
			g_humans, usable ? "USABLE" : "OFF-LIMITS");

		Zones::LogSummary();
	}

	int const frame = Unsorted::CurrentFrame;
	if (cfg.CheckpointInterval > 0 && frame - g_lastSaveFrame >= cfg.CheckpointInterval)
	{
		g_lastSaveFrame = frame;
		Zones::Save();
		// Summarise at each checkpoint, not only at a clean game end: matches
		// here routinely stop without a verdict, and a summary you never see is
		// a summary that can't be graded.
		if (cfg.DebugTicks)
			Zones::LogSummary();
	}
}

// HouseClass::Update — shared frame pulse (ECX = house). Chains with
// Ares/Antares/DoctrineExt/DossierExt at the same address.
DEFINE_HOOK(0x4F8440, WarZoneExt_HouseClass_Update_Tick, 0x5)
{
	GET(HouseClass* const, pThis, ECX);
	Engine::TickHouse(pThis);
	return 0;
}
