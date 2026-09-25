#pragma once

// Phase 1 recording. One hook, the proven kill seat, feeding several zones:
//
//   Kill              every death, anywhere, any house — the core lethality map
//   MinerDeath        harvester deaths — vulnerable harvest spots
//   Lane.<spawn>.Out  where the killer's spawn position strikes
//   Lane.<spawn>.In   where the victim's spawn position gets hit
//
// Why deaths and not "combat": a death is an unambiguous, synced, cheap event
// that every client sees identically. It is the highest-signal thing available
// per unit of cost, which is what makes zone memory converge in a few games.
//
// Both spawn lanes come from the same event, so "this map fights here" becomes
// "a player starting THERE fights HERE" for free.
namespace Recorder
{
	void Reset();
}
