#include "WarZoneExt.h"

#include <Phobos.h>
#include <Syringe.h>
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
