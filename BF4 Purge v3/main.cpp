#include "includes.h"
#include "Interface.h"

// --------------------------------------------------------------------------------
// This project uses many things from Menool's HyperHook internal
// Big shout out to this legend!!
// https://www.unknowncheats.me/forum/battlefield-4-a/462540-x64-stealth-internal-hyperhook-source.html
// --------------------------------------------------------------------------------


// ClientGameContext* GameContext = (ClientGameContext*)*(DWORD*)(OFFSET_CLIENTGAMECONTEXT);

HINSTANCE DLLHandle;

bool threadRunningOK = true;

#ifdef DEBUG
bool debugMode = true;
#else
bool debugMode = false;
#endif

// -----------------------------------------------------------------
// -----------------------------------------------------------------

DWORD __stdcall EjectThread(LPVOID lpParameter) {
  Sleep(100);
  FreeLibraryAndExitThread(DLLHandle, 0);
  Sleep(100);
  return 0;
}

void detach() {
  Sleep(100);

  if (!Interface::ShutdownVisuals()) {
    Sleep(100);
  }
  CreateThread(0, 0, EjectThread, 0, 0, 0);
  return;
}

void WINAPI Main(HMODULE hModule) {
  if (!debugMode) Sleep(30000); // Wait for game initialization before hooking dx11 in release mode

  if (!Interface::InitializeVisuals()) {
    threadRunningOK = false;
  }

  //CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)EnableAimbot, NULL, NULL, NULL);

  while (threadRunningOK) {
    Sleep(50);
    if (GetAsyncKeyState(VK_INSERT) & 1) {
      Interface::showMenu = !Interface::showMenu;
      while (GetAsyncKeyState(VK_INSERT) & 1) {}
    }
    if (GetAsyncKeyState(VK_END) & 1) {
      break;
    }
  }

  detach();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    DLLHandle = hModule;
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Main, hModule, NULL, NULL);
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
