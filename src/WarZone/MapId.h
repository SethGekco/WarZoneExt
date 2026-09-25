#pragma once

#include <string>

// Map identity. TRAP carried over from DossierExt: in skirmish/CnCNet the
// launcher copies the chosen map to spawnmap.ini, so ScenarioClass::FileName is
// the literal "spawnmap.ini" for EVERY match — keying off it collapses every map
// into one record and silently destroys the whole point of per-map memory. The
// real name lives in spawn.ini [Settings] UIMapName ("[8] Powder Keg").
namespace MapId
{
	void Reset();

	// Sanitized, dot-free map stem, e.g. "Powder_Keg". Cached after first call.
	std::string const& Stem();

	// Number of houses that hold a real spawn position.
	int SpawnCount();
}
