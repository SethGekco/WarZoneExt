#include "WarZone/MapId.h"

#include <HouseClass.h>
#include <ScenarioClass.h>
#include <Utilities/Debug.h>

#include <cstdio>
#include <cstring>

namespace
{
	std::string g_stem;
	bool g_resolved = false;

	// INI-section-safe and dot-free (our own parsers split section names on '.').
	void SanitizeKey(std::string& s)
	{
		for (auto& c : s)
		{
			bool const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
				|| (c >= '0' && c <= '9') || c == '_' || c == '-';
			if (!ok)
				c = '_';
		}
		std::string out;
		for (char const c : s)
			if (c != '_' || (!out.empty() && out.back() != '_'))
				out += c;
		while (!out.empty() && out.back() == '_') out.pop_back();
		while (!out.empty() && out.front() == '_') out.erase(out.begin());
		s = out;
	}

	// spawn.ini [Settings] UIMapName — the only place the real map name appears
	// in a spawned game.
	std::string ReadUIMapName()
	{
		std::string result;
		FILE* const f = std::fopen("spawn.ini", "r");
		if (!f)
			return result;
		char line[256];
		bool inSettings = false;
		while (std::fgets(line, sizeof(line), f))
		{
			char* s = line;
			while (*s == ' ' || *s == '\t') ++s;
			if (*s == '[')
			{
				inSettings = std::strncmp(s, "[Settings]", 10) == 0;
				continue;
			}
			if (!inSettings || std::strncmp(s, "UIMapName=", 10) != 0)
				continue;
			char* v = s + 10;
			char* e = v + std::strlen(v);
			while (e > v && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ')) --e;
			*e = '\0';
			result = v;
			break;
		}
		std::fclose(f);
		return result;
	}
}

void MapId::Reset()
{
	g_stem.clear();
	g_resolved = false;
}

std::string const& MapId::Stem()
{
	if (g_resolved)
		return g_stem;
	g_resolved = true;

	std::string fromFile;
	if (auto const pScen = ScenarioClass::Instance) // DEFINE_REFERENCE pointer, not a call
	{
		fromFile = pScen->FileName;
		auto const slash = fromFile.find_last_of("\\/");
		if (slash != std::string::npos)
			fromFile = fromFile.substr(slash + 1);
		auto const dot = fromFile.find_last_of('.');
		if (dot != std::string::npos)
			fromFile = fromFile.substr(0, dot);
		SanitizeKey(fromFile);
	}

	bool const isSpawnStub = fromFile.empty() || _stricmp(fromFile.c_str(), "spawnmap") == 0;
	std::string ui = isSpawnStub ? ReadUIMapName() : std::string();
	if (!ui.empty())
	{
		// Drop a leading "[8] " player-count tag.
		if (ui.front() == '[')
		{
			auto const close = ui.find(']');
			if (close != std::string::npos)
				ui = ui.substr(close + 1);
		}
		SanitizeKey(ui);
	}

	g_stem = !ui.empty() ? ui : (fromFile.empty() ? "UnknownMap" : fromFile);
	Debug::Log("[WarZoneExt] map identity: FileName stem='%s'%s UIMapName='%s' -> '%s'\n",
		fromFile.empty() ? "(none)" : fromFile.c_str(),
		isSpawnStub ? " (spawn stub)" : "",
		ui.empty() ? "(none)" : ui.c_str(), g_stem.c_str());
	return g_stem;
}

int MapId::SpawnCount()
{
	int spawns = 0;
	for (int i = 0; i < HouseClass::Array.Count; ++i)
	{
		auto const pH = HouseClass::Array.GetItem(i);
		if (pH && !pH->IsObserver() && !pH->IsNeutral() && pH->GetSpawnPosition() >= 0)
			++spawns;
	}
	return spawns;
}
