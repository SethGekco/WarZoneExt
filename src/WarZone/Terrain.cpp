#include "WarZone/Terrain.h"
#include "WarZone/Zones.h"
#include "WarZone/Config.h"

#include <MapClass.h>
#include <CellClass.h>
#include <Utilities/Debug.h>

#include <map>
#include <string>

namespace
{
	struct Tally
	{
		int Total = 0;
		int Passable = 0;
		int Rock = 0;
		int Water = 0;
	};

	bool IsBlocking(LandType const lt)
	{
		return lt == LandType::Rock || lt == LandType::Wall || lt == LandType::Water;
	}

	int Pct(int const part, int const total)
	{
		return total > 0 ? (part * 100) / total : 0;
	}
}

void Terrain::EnsureScanned()
{
	auto const& cfg = WarZoneConfig::Instance;
	if (!cfg.ScanTerrain)
		return;
	// Already known for this map — terrain never changes, so never redo it.
	if (Zones::HasZone("Terrain.Open") || Zones::HasZone("Terrain.Cliff"))
	{
		Debug::Log("[WarZoneExt] terrain already on record for this map, skipping scan.\n");
		return;
	}

	auto const& map = MapClass::Instance;
	auto const& b = map.MapCoordBounds;

	// ── Pass 1: one visit per cell, tallied straight into buckets ──────────
	std::map<std::string, Tally> buckets;
	int cells = 0;
	for (int y = b.Top; y <= b.Bottom; ++y)
	{
		for (int x = b.Left; x <= b.Right; ++x)
		{
			CellStruct cs{ static_cast<short>(x), static_cast<short>(y) };
			auto const pCell = map.TryGetCellAt(cs);
			if (!pCell)
				continue;
			++cells;
			auto& t = buckets[Zones::BucketKey(x, y)];
			++t.Total;
			auto const lt = pCell->LandType;
			if (lt == LandType::Rock || lt == LandType::Wall)
				++t.Rock;
			else if (lt == LandType::Water)
				++t.Water;
			if (!IsBlocking(lt))
				++t.Passable;
		}
	}

	// ── Pass 2: classify each bucket ───────────────────────────────────────
	std::map<std::string, int> passPct;
	for (auto const& [key, t] : buckets)
		passPct[key] = Pct(t.Passable, t.Total);

	int open = 0, cliff = 0, water = 0;
	for (auto const& [key, t] : buckets)
	{
		int const p = passPct[key];
		int const rockPct = Pct(t.Rock, t.Total);
		int const waterPct = Pct(t.Water, t.Total);

		if (p >= cfg.OpenPercent)
		{
			Zones::AddRaw("Terrain.Open", key, p);
			++open;
		}
		if (rockPct >= cfg.CliffPercent)
		{
			Zones::AddRaw("Terrain.Cliff", key, rockPct);
			++cliff;
		}
		if (waterPct >= cfg.CliffPercent)
		{
			Zones::AddRaw("Terrain.Water", key, waterPct);
			++water;
		}
	}

	// ── Pass 3: chokes — a partly-blocked bucket that still connects open
	//    ground. Neighbour lookup is a map probe per bucket, so this stays
	//    linear in buckets, not quadratic in cells.
	int chokes = 0;
	for (auto const& [key, t] : buckets)
	{
		int const p = passPct[key];
		if (p <= 0 || p < cfg.ChokeMinPercent || p > cfg.ChokeMaxPercent)
			continue;

		int bx = 0, by = 0;
		if (std::sscanf(key.c_str(), "%d,%d", &bx, &by) != 2)
			continue;

		int openNeighbours = 0;
		for (int dy = -1; dy <= 1; ++dy)
		{
			for (int dx = -1; dx <= 1; ++dx)
			{
				if (!dx && !dy)
					continue;
				char nk[24];
				std::snprintf(nk, sizeof(nk), "%d,%d", bx + dx, by + dy);
				auto const it = passPct.find(nk);
				if (it != passPct.end() && it->second >= cfg.OpenPercent)
					++openNeighbours;
			}
		}
		// Two or more open neighbours means it joins separate open ground —
		// that's a corridor, not just a ragged map edge.
		if (openNeighbours >= 2)
		{
			Zones::AddRaw("Terrain.Choke", key, p);
			++chokes;
		}
	}

	Debug::Log("[WarZoneExt] terrain scanned: %d cells, %u bucket(s) -> open=%d cliff=%d "
		"water=%d choke=%d (open>=%d%%, cliff>=%d%%, choke %d-%d%%)\n",
		cells, buckets.size(), open, cliff, water, chokes,
		cfg.OpenPercent, cfg.CliffPercent, cfg.ChokeMinPercent, cfg.ChokeMaxPercent);
}
