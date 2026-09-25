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

	static WarZoneConfig Instance;

	static void Reset();
	static void EnsureParsed();
};
