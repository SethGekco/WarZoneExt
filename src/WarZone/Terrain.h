#pragma once

// Static map analysis — the map's SHAPE, not its history.
//
// This is a different kind of zone from everything else here. Event zones need
// games to accumulate; terrain is a fact about the map, so it is computed ONCE
// the first time a map is seen and is useful on **game 1**, before any play
// history exists. Cliffs don't move.
//
// Zones produced (all prefixed `Terrain.`, weight = percent, 0..100):
//   Terrain.Open   mostly-passable ground — room to manoeuvre, bad to defend
//   Terrain.Cliff  impassable rock/wall — a wall you can anchor a flank on
//   Terrain.Water  naval-only ground
//   Terrain.Choke  a narrow passable connector between open areas — the cells
//                  worth mining, shelling, or refusing to walk into
//
// `Terrain.*` zones are exempt from TopN pruning and decay (see Zones::Save):
// a map has many cliff buckets and they are not evidence to be forgotten.
namespace Terrain
{
	// Scan if this map has no terrain data yet. Cheap: one linear pass over
	// cells, then one pass over buckets — never per-cell-per-cell.
	void EnsureScanned();
}
