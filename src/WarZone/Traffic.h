#pragma once

// Where units actually GO, as opposed to where they die.
//
// Sampled rather than hooked: every TrafficInterval frames we walk the unit and
// infantry arrays and bucket anything that is currently moving with intent
// (`Destination != nullptr`). Sampling is the right tool here — movement is a
// continuous state, not an event, and a hook on it would fire absurdly often.
//
// Zones produced:
//   Traffic        all hostile-capable movement — the map's road network as
//                  players actually use it, not as the terrain suggests
//   MinerTravel    harvester movement — the ore commute, i.e. where an
//                  interdiction actually hurts
namespace Traffic
{
	void Reset();

	// Self-gates on frame; safe to call from every house tick.
	void MaybeSample();
}
