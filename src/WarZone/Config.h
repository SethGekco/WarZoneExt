#pragma once

#include <string>

// [WarZone.*] config, re-read per scenario (game-mode INIs merge into the rules
// INI each match).
struct WarZoneConfig
{
	bool Parsed = false;

	// [WarZone.General]
	bool Enabled = true;
	bool DebugTicks = false;
	std::string ZoneDir = "WarZones";   // one INI per map
	int Bucket = 8;                     // cells per zone bucket
	int TopN = 64;                      // hottest buckets kept per zone on save
	int DecayShift = 0;                 // >0: halve stored weight >>N on load
	int ForgetBelowGames = 0;           // >0: ignore records thinner than this
	int CheckpointInterval = 3000;      // frames between saves
	bool UseHistoryInMultiplayer = false; // reading history into sim = desync

	// Traffic sampling (movement is a state, not an event, so it's sampled).
	int TrafficInterval = 150;          // frames between movement samples

	// Static terrain analysis. Percentages are per bucket, so they describe
	// SPACING: 90% passable = open ground, 20% = mostly wall/water/cliff.
	bool ScanTerrain = true;
	// Force a re-scan even when terrain is already on record. Only needed when
	// the SCANNER itself changed — stored terrain is otherwise permanent.
	bool RescanTerrain = false;
	// Scan the map's ore fields once into the `Wealth` zone (where the money is).
	bool ScanWealth = true;
	int OpenPercent = 85;               // >= this passable -> Terrain.Open
	int CliffPercent = 40;              // >= this rock/water -> Cliff / Water
	int ChokeMinPercent = 10;           // a choke is passable but narrow...
	int ChokeMaxPercent = 60;           // ...and needs >=2 open neighbours

	static WarZoneConfig Instance;

	static void Reset();
	static void EnsureParsed();
};
