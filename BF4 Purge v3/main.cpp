#include "includes.h"
#include "Interface.h"
#include "VMTHooking.h"

// --------------------------------------------------------------------------------
// This project uses many things from Menool's HyperHook internal
// Big shout out to this legend!!
// https://www.unknowncheats.me/forum/battlefield-4-a/462540-x64-stealth-internal-hyperhook-source.html
// --------------------------------------------------------------------------------


// ClientGameContext* GameContext = (ClientGameContext*)*(DWORD*)(OFFSET_CLIENTGAMECONTEXT);

// -----------------------------------------------------------------
// -----------------------------------------------------------------

DWORD WINAPI EjectThread() {
  FreeLibraryAndExitThread(G::hInst, 0);
  return 1;
}

DWORD WINAPI Main(HMODULE hModule) {
  //Sleep(5000);

  if (!Interface::InitializeVisuals()) {
    G::shouldExit = true;
  }

  HooksManager::Get()->Install();

  F::pVisuals = std::make_unique<Visuals>();
  F::pFeatures = std::make_unique<Features>();

  while (!G::shouldExit) Sleep(300);

  F::pFeatures->MinimapSpot(false);
  F::pFeatures->Recoil(false);
  F::pFeatures->Spread(false);

  HooksManager::Get()->Uninstall();

  Interface::ShutdownVisuals();

  Sleep(100);

  return EjectThread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
  
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    G::hInst = hModule;
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Main, hModule, NULL, NULL);
  }

  return TRUE;
}
