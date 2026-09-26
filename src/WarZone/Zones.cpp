#include "WarZone/Zones.h"
#include "WarZone/Config.h"
#include "WarZone/MapId.h"

#include <Utilities/Debug.h>

#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

namespace
{
	MapRecord g_map;

	std::string PathFor(std::string const& stem)
	{
		return WarZoneConfig::Instance.ZoneDir + "\\" + stem + ".ini";
	}

	std::string Timestamp()
	{
		char buf[32] = { 0 };
		std::time_t const now = std::time(nullptr);
		std::tm tmv;
		if (localtime_s(&tmv, &now) == 0)
			std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmv);
		return buf;
	}

	// `Terrain.*` is measured map shape, not accumulated evidence: it must NOT be
	// pruned to TopN (a map has far more than TopN cliff buckets, and cutting
	// them would silently amputate the map's geography) and must NOT decay
	// (cliffs don't erode). Everything else is evidence and gets both.
	// `Wealth` is the same kind of fact — the map's ore fields are geography, not
	// evidence to forget — so it shares the static exemption.
	bool IsStatic(std::string const& zone)
	{
		return zone.compare(0, 8, "Terrain.") == 0 || zone == "Wealth";
	}

	// Keep only the hottest TopN buckets so one map file can't grow forever.
	void Prune(std::map<std::string, int>& grid, int const topN)
	{
		if (topN <= 0 || static_cast<int>(grid.size()) <= topN)
			return;
		std::vector<std::pair<std::string, int>> all(grid.begin(), grid.end());
		std::sort(all.begin(), all.end(), [](auto const& a, auto const& b)
			{ return a.second != b.second ? a.second > b.second : a.first < b.first; });
		all.resize(topN);
		grid.clear();
		for (auto const& [bucket, weight] : all)
			grid[bucket] = weight;
	}

	// Sections are "Meta" or "Zone.<Name>" — the zone name is everything after
	// the first dot, so names may themselves contain dots (Lane.0.Out).
	void LoadFromDisk(MapRecord& rec)
	{
		auto const path = PathFor(rec.Stem);
		FILE* const f = std::fopen(path.c_str(), "r");
		if (!f)
		{
			Debug::Log("[WarZoneExt] map '%s': no record yet (%s), starting fresh.\n",
				rec.Stem.c_str(), path.c_str());
			return;
		}

		char line[512];
		std::string section;
		while (std::fgets(line, sizeof(line), f))
		{
			char* s = line;
			while (*s == ' ' || *s == '\t') ++s;
			char* e = s + std::strlen(s);
			while (e > s && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ')) --e;
			*e = '\0';
			if (!*s || *s == ';')
				continue;
			if (*s == '[')
			{
				if (char* const close = std::strchr(s, ']'))
				{
					*close = '\0';
					section = s + 1;
				}
				continue;
			}
			char* const eq = std::strchr(s, '=');
			if (!eq)
				continue;
			*eq = '\0';
			char const* const key = s;
			char const* const value = eq + 1;

			if (section == "Meta")
			{
				if (!std::strcmp(key, "Games")) rec.Games = std::atoi(value);
				else if (!std::strcmp(key, "Width")) rec.Width = std::atoi(value);
				else if (!std::strcmp(key, "Height")) rec.Height = std::atoi(value);
				else if (!std::strcmp(key, "Spawns")) rec.Spawns = std::atoi(value);
			}
			else if (!section.compare(0, 5, "Zone."))
			{
				rec.Zones[section.substr(5)][key] = std::atoi(value);
			}
		}
		std::fclose(f);

		auto const& cfg = WarZoneConfig::Instance;

		// Thin records are noise, not meta — optionally ignore them.
		if (cfg.ForgetBelowGames > 0 && rec.Games < cfg.ForgetBelowGames)
		{
			Debug::Log("[WarZoneExt] map '%s': only %d game(s) recorded, below "
				"ForgetBelowGames=%d — ignoring stored zones.\n",
				rec.Stem.c_str(), rec.Games, cfg.ForgetBelowGames);
			for (auto it = rec.Zones.begin(); it != rec.Zones.end(); )
				it = IsStatic(it->first) ? std::next(it) : rec.Zones.erase(it);
		}
		// Decay keeps a stale meta from dominating after the map's play changes.
		else if (cfg.DecayShift > 0)
		{
			for (auto& [zone, grid] : rec.Zones)
			{
				if (IsStatic(zone))
					continue;           // map shape doesn't erode
				for (auto& [bucket, weight] : grid)
					weight >>= cfg.DecayShift;
			}
		}

		size_t buckets = 0;
		for (auto const& [zone, grid] : rec.Zones)
			buckets += grid.size();
		Debug::Log("[WarZoneExt] map '%s' loaded: games=%d %dx%d spawns=%d, %u zone(s), "
			"%u bucket(s)%s\n",
			rec.Stem.c_str(), rec.Games, rec.Width, rec.Height, rec.Spawns,
			rec.Zones.size(), buckets,
			cfg.DecayShift > 0 ? " (decayed)" : "");
		rec.Loaded = true;
	}
}

void Zones::Reset()
{
	g_map = MapRecord{};
	MapId::Reset();
}

std::string Zones::BucketKey(int const cellX, int const cellY)
{
	int const b = WarZoneConfig::Instance.Bucket;
	char buf[24];
	std::snprintf(buf, sizeof(buf), "%d,%d", cellX / b, cellY / b);
	return buf;
}

MapRecord const& Zones::Current()
{
	return g_map;
}

void Zones::OpenForCurrentMap()
{
	if (g_map.Loaded || !g_map.Stem.empty())
		return;
	g_map.Stem = MapId::Stem();
	LoadFromDisk(g_map);
	++g_map.Games;          // this match counts even if it's abandoned
	g_map.Dirty = true;
	Save();                 // prove the write path at match START, not at the end
}

void Zones::SetFingerprint(int const width, int const height, int const spawns)
{
	if (g_map.Width == width && g_map.Height == height && g_map.Spawns == spawns)
		return;
	g_map.Width = width;
	g_map.Height = height;
	g_map.Spawns = spawns;
	g_map.Dirty = true;
}

void Zones::Add(const char* const zone, int const cellX, int const cellY, int const weight)
{
	if (!zone || !*zone || weight == 0)
		return;
	g_map.Zones[zone][BucketKey(cellX, cellY)] += weight;
	g_map.Dirty = true;
}

void Zones::AddRaw(const char* const zone, std::string const& bucketKey, int const weight)
{
	if (!zone || !*zone)
		return;
	g_map.Zones[zone][bucketKey] = weight;
	g_map.Dirty = true;
}

bool Zones::HasZone(const char* const zone)
{
	if (!zone || !*zone)
		return false;
	auto const it = g_map.Zones.find(zone);
	return it != g_map.Zones.end() && !it->second.empty();
}

int Zones::Weight(const char* const zone, int const cellX, int const cellY)
{
	if (!zone || !*zone)
		return 0;
	auto const z = g_map.Zones.find(zone);
	if (z == g_map.Zones.end())
		return 0;
	auto const b = z->second.find(BucketKey(cellX, cellY));
	return b != z->second.end() ? b->second : 0;
}

void Zones::Save()
{
	if (!g_map.Dirty || g_map.Stem.empty())
		return;
	auto const& cfg = WarZoneConfig::Instance;
	CreateDirectoryA(cfg.ZoneDir.c_str(), nullptr); // no-op if it exists

	for (auto& [zone, grid] : g_map.Zones)
		if (!IsStatic(zone))
			Prune(grid, cfg.TopN);

	auto const path = PathFor(g_map.Stem);
	FILE* const f = std::fopen(path.c_str(), "w");
	if (!f)
	{
		Debug::Log("[WarZoneExt] WARNING: cannot write map record %s\n", path.c_str());
		return;
	}
	std::fprintf(f, "; WarZoneExt map memory — generated, hand-editable.\n");
	std::fprintf(f, "[Meta]\nName=%s\nGames=%d\nWidth=%d\nHeight=%d\nSpawns=%d\nLastSeen=%s\n",
		g_map.Stem.c_str(), g_map.Games, g_map.Width, g_map.Height, g_map.Spawns,
		Timestamp().c_str());
	for (auto const& [zone, grid] : g_map.Zones)
	{
		if (grid.empty())
			continue;
		std::fprintf(f, "\n[Zone.%s]\n", zone.c_str());
		for (auto const& [bucket, weight] : grid)
			std::fprintf(f, "%s=%d\n", bucket.c_str(), weight);
	}
	std::fclose(f);
	g_map.Dirty = false;

	if (cfg.DebugTicks)
		Debug::Log("[WarZoneExt] map '%s' saved (%s): games=%d, %u zone(s)\n",
			g_map.Stem.c_str(), path.c_str(), g_map.Games, g_map.Zones.size());
}

void Zones::LogSummary()
{
	Debug::Log("[WarZoneExt] map '%s': games=%d %dx%d spawns=%d zones=%u\n",
		g_map.Stem.c_str(), g_map.Games, g_map.Width, g_map.Height, g_map.Spawns,
		g_map.Zones.size());
	for (auto const& [zone, grid] : g_map.Zones)
	{
		// Name the hottest bucket per zone — a summary you can sanity-check
		// against what actually happened on the field.
		std::string top;
		int best = 0;
		for (auto const& [bucket, weight] : grid)
			if (weight > best) { best = weight; top = bucket; }
		Debug::Log("[WarZoneExt]   zone %s: %u bucket(s), hottest %s=%d\n",
			zone.c_str(), grid.size(), top.empty() ? "-" : top.c_str(), best);
	}
}
