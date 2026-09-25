#pragma once

#include <map>
#include <string>

// ─── The zone store ────────────────────────────────────────────────────────
//
// A **zone** is one named grid of accumulated weight for the current map, keyed
// by "bx,by" bucket coordinates. The set of zone names is data, not code: a new
// kind of memory (Traffic, PathError, MinerTravel…) is just a new name, which is
// what lets three consumer projects extend this in parallel without colliding.
//
// Phase 0 records nothing. It proves the whole spine: identify the map, read its
// file if we've played it before, and write it back. Phase 1 starts feeding
// zones from the kill seat.
//
// Zone names are a CONTRACT — consumers query them as strings, so renaming one
// breaks them silently. Add names; never repurpose one.
struct MapRecord
{
	std::string Stem;                 // sanitized map identity (file stem)
	int Games = 0;                    // matches ever played on this map
	int Width = 0, Height = 0, Spawns = 0;
	std::map<std::string, std::map<std::string, int>> Zones; // zone -> bucket -> weight
	bool Loaded = false;
	bool Dirty = false;
};

namespace Zones
{
	void Reset();

	// Identify the map, load its record (or start one), bump the game counter.
	void OpenForCurrentMap();

	// Map shape, once known.
	void SetFingerprint(int width, int height, int spawns);

	// Add weight to a zone bucket. The only write path — every future zone type
	// goes through it, so pruning and decay stay in one place.
	void Add(const char* zone, int cellX, int cellY, int weight = 1);

	// Weight currently stored for a cell (0 if unknown).
	int Weight(const char* zone, int cellX, int cellY);

	// Set an exact weight at an already-computed bucket key. For derived data
	// that is a measurement rather than an accumulation — terrain percentages.
	void AddRaw(const char* zone, std::string const& bucketKey, int weight);

	// Does this map's record already carry this zone? Used to avoid recomputing
	// static data that can never change.
	bool HasZone(const char* zone);

	// Persist if dirty. Prunes each zone to TopN first.
	void Save();

	MapRecord const& Current();

	// "bx,by" for a cell at the configured bucket size.
	std::string BucketKey(int cellX, int cellY);

	void LogSummary();
}
