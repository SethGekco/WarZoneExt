#pragma once

class HouseClass;

// Per-house tick, chained at the proven HouseClass::Update seat. WarZoneExt has
// no per-house logic of its own — it just needs *a* frame pulse, so everything
// here self-gates on frame. Phase 0: open the map record on the first tick,
// capture the fingerprint, and checkpoint-save.
namespace Engine
{
	void Reset();
	void TickHouse(HouseClass* pHouse);
}
