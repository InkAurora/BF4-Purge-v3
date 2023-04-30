#include "includes.h"
#include "Interface.h"

// ClientGameContext* GameContext = (ClientGameContext*)*(DWORD*)(OFFSET_CLIENTGAMECONTEXT);

HINSTANCE DLLHandle;

bool threadRunningOK = true;

// -----------------------------------------------------------------
// -----------------------------------------------------------------

DWORD __stdcall EjectThread(LPVOID lpParameter) {
  Sleep(100);
  FreeLibraryAndExitThread(DLLHandle, 0);
  Sleep(100);
  return 0;
}

void detach() {
  /*ENABLE_AIMBOT = false;
  DETACH_AIMBOT = true;*/

  Sleep(100);

  if (!Interface::ShutdownVisuals()) {
    Sleep(100);
  }
  CreateThread(0, 0, EjectThread, 0, 0, 0);
  return;
}

int WINAPI Main(HMODULE hModule) {
  //Sleep(30000); // Wait for game initialization before hooking dx11 -- Uncomment before compiling release

  if (!Interface::InitializeVisuals()) {
    threadRunningOK = false;
  }

  //CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)EnableAimbot, NULL, NULL, NULL);

  while (threadRunningOK) {
    Sleep(50);
    if (GetAsyncKeyState(VK_INSERT) & 1) {
      //show_menu = !show_menu;
      while (GetAsyncKeyState(VK_NUMPAD1) & 1) {}
    }
    if (GetAsyncKeyState(VK_END) & 1) {
      break;
    }
  }

  detach();

  return 0;
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
