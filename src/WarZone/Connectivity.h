#pragma once

// Ground/water reachability — the map's CONNECTIVITY, not its density.
//
// A sibling to Terrain.cpp: same "measured once, useful on game 1, never
// recomputed" shape, but Terrain.* answers "how open is this bucket" while
// this answers "can you actually get from bucket A to bucket B". Two buckets
// are reachable by a given movement type iff their recorded zone id matches —
// nothing here is a live pathfind, it's a one-time read of YR's own
// precomputed per-cell connectivity grid (MapClass::GetMovementZoneType).
//
// Zones produced (prefixed `Terrain.` so Zones::IsStatic exempts them from
// TopN pruning and decay, same as Open/Cliff/Water/Choke):
//   Terrain.GroundZone   the bucket's MovementZone::Normal region id, +1
//   Terrain.WaterZone    the bucket's MovementZone::Water region id, +1
//
// Weight convention (NOT a count or percentage like the other Terrain.*
// zones): the stored int is (engine zone id + 1). 0 means "not recorded" —
// either the bucket has no zone for that movement type (an inland bucket has
// no water zone) or connectivity hasn't been scanned. Two non-zero values
// from the SAME zone name are comparable ONLY for equality: equal = connected,
// different = not. The magnitude itself means nothing — do not sum, rank, or
// threshold it like Terrain.Open's percentage.
//
// A bucket samples ONE cell (the first on-map cell touched while scanning),
// same bucket-granularity approximation Terrain.cpp already accepts for
// Open/Cliff/Water/Choke. A bucket straddling a zone boundary records
// whichever side that one cell landed on.
namespace Connectivity
{
	// Scan if this map has no connectivity data yet. Same cost class as
	// Terrain::EnsureScanned: one linear pass over cells, two engine calls per
	// newly-seen bucket.
	void EnsureScanned();
}
