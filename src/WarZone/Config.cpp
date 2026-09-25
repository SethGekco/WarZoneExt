#include "WarZone/Config.h"
#include "WarZone/Zones.h"
#include "WarZone/Engine.h"
#include "WarZone/Recorder.h"

#include <CCINIClass.h>
#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

WarZoneConfig WarZoneConfig::Instance;

void WarZoneConfig::Reset()
{
	Instance = WarZoneConfig{};
}

void WarZoneConfig::EnsureParsed()
{
	auto& cfg = Instance;
	if (cfg.Parsed)
		return;
	auto const pINI = CCINIClass::INI_Rules;
	if (!pINI)
		return;
	cfg.Parsed = true;

	char buf[256] = { 0 };
	cfg.Enabled = pINI->ReadBool("WarZone.General", "Enabled", cfg.Enabled);
	cfg.DebugTicks = pINI->ReadBool("WarZone.General", "DebugTicks", cfg.DebugTicks);
	pINI->ReadString("WarZone.General", "ZoneDir", cfg.ZoneDir.c_str(), buf, sizeof(buf));
	cfg.ZoneDir = buf;
	cfg.Bucket = pINI->ReadInteger("WarZone.General", "Bucket", cfg.Bucket);
	cfg.TopN = pINI->ReadInteger("WarZone.General", "TopN", cfg.TopN);
	cfg.DecayShift = pINI->ReadInteger("WarZone.General", "DecayShift", cfg.DecayShift);
	cfg.ForgetBelowGames = pINI->ReadInteger("WarZone.General", "ForgetBelowGames", cfg.ForgetBelowGames);
	cfg.CheckpointInterval = pINI->ReadInteger("WarZone.General", "CheckpointInterval", cfg.CheckpointInterval);
	cfg.UseHistoryInMultiplayer = pINI->ReadBool("WarZone.General", "UseHistoryInMultiplayer",
		cfg.UseHistoryInMultiplayer);

	if (cfg.Bucket < 1)
		cfg.Bucket = 1;

	Debug::Log("[WarZoneExt] [WarZone.General]: Enabled=%d DebugTicks=%d ZoneDir=%s Bucket=%d "
		"TopN=%d DecayShift=%d ForgetBelowGames=%d CheckpointInterval=%d "
		"UseHistoryInMultiplayer=%d\n",
		cfg.Enabled, cfg.DebugTicks, cfg.ZoneDir.c_str(), cfg.Bucket, cfg.TopN,
		cfg.DecayShift, cfg.ForgetBelowGames, cfg.CheckpointInterval,
		cfg.UseHistoryInMultiplayer);
	if (cfg.UseHistoryInMultiplayer)
		Debug::Log("[WarZoneExt] WARNING: UseHistoryInMultiplayer=yes — stored history differs "
			"per client, so letting it influence the sim WILL desync a 2+ human game.\n");
}

// Scenario::ClearClasses — per-scenario reset and re-parse. Same seat
// DoctrineExt/DossierExt clear at; same-address hooks chain legally.
DEFINE_HOOK(0x685659, WarZoneExt_Scenario_ClearClasses, 0xA)
{
	Zones::Reset();
	Engine::Reset();
	Recorder::Reset();
	WarZoneConfig::Reset();
	WarZoneConfig::EnsureParsed();
	return 0;
}
