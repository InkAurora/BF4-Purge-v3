#include "includes.h"
#include "Interface.h"
#include "Logger.h"
#include "TypeInfo.h"
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
  G::hInst = hModule;

  auto EjectWithCleanup = []() -> DWORD {
#ifdef _DEBUG
    DebugLogger::Get().Shutdown();
#endif
    return EjectThread();
  };

#ifdef _DEBUG
  if (DebugLogger::Get().Initialize()) {
    LOG_INFO("main", "logger initialized at %s", DebugLogger::Get().GetLogPath().c_str());
  }
#endif
  LOG_INFO("main", "main thread started, module=%p", hModule);

  LOG_INFO("main", "initializing visuals");
  if (!Interface::InitializeVisuals()) {
    LOG_ERROR("main", "Interface::InitializeVisuals failed");
    return EjectWithCleanup();
  }

  LOG_INFO("main", "visuals initialized successfully");

  G::g_exitEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

  if (!G::g_exitEvent) {
    LOG_ERROR("main", "CreateEventA failed, shutting visuals down");
    Interface::ShutdownVisuals();
    return EjectWithCleanup();
  }

  LOG_INFO("main", "exit event created: %p", G::g_exitEvent);

  LOG_INFO("main", "installing hooks");
  HooksManager::Get()->Install();

  LOG_INFO("main", "dumping RTTI entry component layouts");
  DumpEntryComponentLayouts();

  F::pVisuals = std::make_unique<Visuals>();
  F::pFeatures = std::make_unique<Features>();

  LOG_INFO("main", "feature objects constructed, waiting for exit event");

  WaitForSingleObject(G::g_exitEvent, INFINITE);

  LOG_INFO("main", "exit event signaled, beginning teardown");

  F::pFeatures->MinimapSpot(false);
  F::pFeatures->Recoil(false);
  F::pFeatures->Spread(false);

  HooksManager::Get()->Uninstall();
  LOG_INFO("main", "hooks uninstalled");

  if (!Interface::ShutdownVisuals()) {
    LOG_WARN("main", "Interface::ShutdownVisuals reported a failure");
  } else {
    LOG_INFO("main", "visuals shutdown completed");
  }

  CloseHandle(G::g_exitEvent);
  G::g_exitEvent = nullptr;

  LOG_INFO("main", "exit handle closed, ejecting thread");

#ifdef _DEBUG
  DebugLogger::Get().Shutdown();
#endif

  Sleep(200);

  return EjectThread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
  
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hModule);

    HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Main, hModule, NULL, NULL);
    
    if (hThread) CloseHandle(hThread);
  }

  return TRUE;
}
