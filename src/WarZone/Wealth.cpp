#include "WarZone/Wealth.h"
#include "WarZone/Zones.h"
#include "WarZone/Config.h"

#include <CellClass.h>
#include <MapClass.h>
#include <Utilities/Debug.h>

namespace
{
	bool g_scanned = false;
}

void Wealth::Reset()
{
	g_scanned = false;
}

void Wealth::EnsureScanned()
{
	auto const& cfg = WarZoneConfig::Instance;
	if (!cfg.ScanWealth || g_scanned)
		return;

	// Already recorded for this map (from a previous game)? Don't rescan — the ore
	// geography is stable, and this keeps a mid-game depletion snapshot from
	// overwriting the map's full-field record.
	if (Zones::HasZone("Wealth"))
	{
		g_scanned = true;
		return;
	}

	auto const& b = MapClass::Instance.MapCoordBounds;
	long long total = 0;
	int cells = 0;
	for (int y = b.Top; y <= b.Bottom; ++y)
		for (int x = b.Left; x <= b.Right; ++x)
		{
			CellStruct cs;
			cs.X = static_cast<short>(x);
			cs.Y = static_cast<short>(y);
			auto const pCell = MapClass::Instance.TryGetCellAt(cs);
			if (!pCell)
				continue;
			int const value = pCell->GetContainedTiberiumValue();
			if (value <= 0)
				continue;
			// Sum value into the cell's bucket — richer fields score higher.
			Zones::Add("Wealth", x, y, value);
			total += value;
			++cells;
		}
	g_scanned = true;

	Debug::Log("[WarZoneExt] wealth scanned: %d ore cell(s), total value %lld across the map\n",
		cells, total);
}
