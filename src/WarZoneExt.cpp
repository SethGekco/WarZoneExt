#include "WarZoneExt.h"

#include "WarZone/Zones.h"
#include "WarZone/Config.h"

#include <Phobos.h>
#include <Syringe.h>
#include <TechnoClass.h>
#include <HouseClass.h>
#include <Utilities/Patch.h>
#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

HANDLE WarZoneExtDLL::hInstance = nullptr;

char WarZoneExtDLL::readBuffer[WarZoneExtDLL::readLength];
wchar_t WarZoneExtDLL::wideBuffer[WarZoneExtDLL::readLength];

void WarZoneExtDLL::ExeRun()
{
	Patch::ApplyStatic();
}

bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		WarZoneExtDLL::hInstance = hInstance;
		Phobos::hInstance = hInstance; // needed by Patch::ApplyStatic
	}
	return true;
}

SYRINGE_HANDSHAKE(pInfo)
{
	pInfo->Message = const_cast<char*>("WarZoneExt");
	return S_OK;
}

// Main-loop entry, so static patches apply at the right time. NOTE: Debug::Log
// from here is discarded — the log isn't open yet. All echoing happens from the
// per-scenario parse instead.
DEFINE_HOOK(0x7CD810, WarZoneExt_ExeRun, 0x9)
{
	WarZoneExtDLL::ExeRun();
	return 0;
}

// Flush the deferred debug log once the command line has been parsed.
DEFINE_HOOK(0x52F639, WarZoneExt_CmdLineParse, 0x5)
{
	Debug::LogDeferredFinalize();
	return 0;
}

// ─── AirDeath zone (AA P2, driven by DoctrineExt) ────────────────────────────
// A SECOND read-only hook at the kill seat (Recorder already owns one). Records
// WHERE aircraft get their kills, so any consumer learns a map's air-contested
// ground — persistent, player-independent, useful from game 1 on a known map.
// Same-address chaining is legal (Syringe runs both handlers); DEFINE_HOOK_AGAIN
// gives this one a distinct symbol. Additive: a NEW zone name, nothing repurposed.
DEFINE_HOOK_AGAIN(0x702D40, WarZoneExt_RegisterDestruction_AirDeath, 0x5)
{
	GET(TechnoClass* const, pVictim, ECX);
	GET_STACK(TechnoClass* const, pKiller, 0x4);

	auto const& cfg = WarZoneConfig::Instance;
	if (!cfg.Parsed || !cfg.Enabled || !pVictim || !pKiller)
		return 0;
	if (pKiller->WhatAmI() != AbstractType::Aircraft)
		return 0;

	auto const cell = pVictim->GetMapCoords();
	Zones::Add("AirDeath", cell.X, cell.Y);
	if (cfg.DebugTicks)
		Debug::Log("[WarZoneExt] air kill at %d,%d (bucket %s)\n",
			cell.X, cell.Y, Zones::BucketKey(cell.X, cell.Y).c_str());
	return 0;
}

// ─── Exported query API — v1 ─────────────────────────────────────────────────
// The FIRST exported surface (the changelog's "Phase 3", brought forward for
// DoctrineExt's air-defense). Consumers bind OPTIONALLY:
//   GetModuleHandleA("WarZoneExt.dll") + GetProcAddress("WZ_Version"/"WZ_ZoneWeight")
// so they run unchanged when WarZoneExt isn't installed and light up when it is.
// WZ_Version() is the gate: absent or too low => don't rely on the rest.
// Zone names are the contract (see CHANGELOG); query them by string.
extern "C" __declspec(dllexport) int __cdecl WZ_Version()
{
	return 1;
}

extern "C" __declspec(dllexport) int __cdecl WZ_ZoneWeight(const char* zone, int cellX, int cellY)
{
	if (!zone) return 0;
	return Zones::Weight(zone, cellX, cellY);
}
