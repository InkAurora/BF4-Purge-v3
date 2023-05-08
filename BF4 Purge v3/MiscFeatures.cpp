#include "MiscFeatures.h"

// Cancerous code ahead!

void Features::Recoil(bool state) {
  DWORD64 moduleBase = (DWORD64)GetModuleHandle(NULL);
  DWORD64 address = moduleBase + 0x2D6DE0;
  if (state && !Cfg::DBG::isRecoilDisabled) {
	Misc::WriteBytes(address, (void*)0x9090909090, 5);
	Cfg::DBG::isRecoilDisabled = true;
  } else if (!state && Cfg::DBG::isRecoilDisabled) {
	Misc::WriteBytes(address, (void*)Misc::GetFunctionCallAddress(address, moduleBase + 0x2D8FD0), 5);
	Cfg::DBG::isRecoilDisabled = false;
  }
}

void Features::Spread(bool state) {
  DWORD64 moduleBase = (DWORD64)GetModuleHandle(NULL);
  DWORD64 address = moduleBase + 0x2D84B8;
  if (state && !Cfg::DBG::isSpreadDisabled) {
	Misc::WriteBytes(address, (void*)Misc::ByteToMem(0xF30F119BC0010000, 8), 8);
	Cfg::DBG::isSpreadDisabled = true;
  } else if (!state && Cfg::DBG::isSpreadDisabled) {
	Misc::WriteBytes(address, (void*)Misc::ByteToMem(0xF30F1183C0010000, 8), 8);
	Cfg::DBG::isSpreadDisabled = false;
  }
}
