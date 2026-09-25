#include "WarZone/Connectivity.h"
#include "WarZone/Zones.h"
#include "WarZone/Config.h"

#include <MapClass.h>
#include <Utilities/Debug.h>

#include <map>
#include <set>
#include <string>

void Connectivity::EnsureScanned()
{
	auto const& cfg = WarZoneConfig::Instance;
	if (!cfg.ScanTerrain)
		return;
	// Own zone names, own skip-check — deliberately independent of
	// Terrain::EnsureScanned's Open/Cliff check, so a map record saved before
	// this feature existed (which already has Terrain.Open) doesn't
	// short-circuit this scan and leave connectivity permanently unrecorded.
	if (Zones::HasZone("Terrain.GroundZone") || Zones::HasZone("Terrain.WaterZone"))
	{
		Debug::Log("[WarZoneExt] connectivity already on record for this map, skipping scan.\n");
		return;
	}

	auto& map = MapClass::Instance;
	auto const& b = map.MapCoordBounds;

	// One representative cell per bucket — same granularity Terrain.cpp
	// already accepts for Open/Cliff/Water/Choke. `seen` caps the expensive
	// engine call to once per bucket even though the outer walk still has to
	// touch every cell to discover bucket membership.
	std::map<std::string, bool> seen;
	std::set<int> distinctGround, distinctWater;
	int buckets = 0, groundHits = 0, waterHits = 0;

	for (int y = b.Top; y <= b.Bottom; ++y)
	{
		for (int x = b.Left; x <= b.Right; ++x)
		{
			auto const key = Zones::BucketKey(x, y);
			if (seen.count(key))
				continue;

			CellStruct const cs{ static_cast<short>(x), static_cast<short>(y) };
			if (!map.TryGetCellAt(cs))
				continue;
			seen[key] = true;
			++buckets;

			// Stored as (id + 1): 0 must mean "not recorded", but a real zone
			// id can legitimately be 0, so the raw id can't double as the
			// sentinel. Consumers compare two reads for EQUALITY only — the
			// magnitude carries no meaning beyond that.
			int const gz = map.GetMovementZoneType(cs, MovementZone::Normal, false);
			if (gz >= 0)
			{
				Zones::AddRaw("Terrain.GroundZone", key, gz + 1);
				distinctGround.insert(gz);
				++groundHits;
			}
			int const wz = map.GetMovementZoneType(cs, MovementZone::Water, false);
			if (wz >= 0)
			{
				Zones::AddRaw("Terrain.WaterZone", key, wz + 1);
				distinctWater.insert(wz);
				++waterHits;
			}
		}
	}

	Debug::Log("[WarZoneExt] connectivity scanned: %d bucket(s) -> ground=%d bucket(s) in "
		"%u distinct zone(s), water=%d bucket(s) in %u distinct zone(s)%s\n",
		buckets, groundHits, static_cast<unsigned>(distinctGround.size()),
		waterHits, static_cast<unsigned>(distinctWater.size()),
		distinctGround.size() > 1 ? " -- map has disconnected land" : "");
}
