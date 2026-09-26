#pragma once

// Ore/gem WEALTH geography — WHERE the map's money is.
//
// Measured once, like Terrain: a map's ore fields at game start are stable map
// facts (they regrow to roughly the same places), so this is useful on game 1
// before any economy has played out. Recorded as summed tiberium value per
// bucket in the `Wealth` zone, and exempt from TopN pruning and decay (see
// Zones::IsStatic) — every ore field is geography to remember, not evidence to
// forget. Consumers read it by name via WZ_ZoneWeight to seek/expand toward money.
namespace Wealth
{
	void Reset();

	// Scan the map's ore once (cached via the stored zone / a run flag). No-op if
	// already on this map's record or already scanned this session.
	void EnsureScanned();
}
