#include "MiscFeatures.h"

// Cancerous code ahead!

Features::Features() {
  moduleBase = (DWORD64)GetModuleHandle(NULL);;
}

void Features::MinimapSpot(bool state) {
  if (state == Cfg::Misc::isMinimapSpotted) return;
  DWORD64 address1 = moduleBase + 0x292AEC;
  DWORD64 address2 = moduleBase + 0x2943BE;
  if (state) {
	Misc::WriteBytes(address1, (void*)0x9090909090, 5);
	Misc::WriteBytes(address2, (void*)0x9090909090, 5);
	Cfg::Misc::isMinimapSpotted = true;
  } else {
	Misc::WriteBytes(address1, (void*)Misc::GetFunctionCallAddress(address1, moduleBase + 0x2AD530), 5);
	Misc::WriteBytes(address2, (void*)Misc::GetFunctionCallAddress(address2, moduleBase + 0x2A95D0), 5);
	Cfg::Misc::isMinimapSpotted = false;
  }
}

void Features::Recoil(bool state) {
  if (state == Cfg::Misc::isRecoilDisabled) return;
  DWORD64 moduleBase = (DWORD64)GetModuleHandle(NULL);
  DWORD64 address = moduleBase + 0x2D6DE0;
  if (state) {
	Misc::WriteBytes(address, (void*)0x9090909090, 5);
	Cfg::Misc::isRecoilDisabled = true;
  } else {
	Misc::WriteBytes(address, (void*)Misc::GetFunctionCallAddress(address, moduleBase + 0x2D8FD0), 5);
	Cfg::Misc::isRecoilDisabled = false;
  }
}

void Features::Spread(bool state) {
  if (state == Cfg::Misc::isSpreadDisabled) return;
  DWORD64 moduleBase = (DWORD64)GetModuleHandle(NULL);
  DWORD64 address = moduleBase + 0x2D84B8;
  if (state) {
	Misc::WriteBytes(address, (void*)Misc::ByteToMem(0xF30F119BC0010000, 8), 8);
	Cfg::Misc::isSpreadDisabled = true;
  } else {
	Misc::WriteBytes(address, (void*)Misc::ByteToMem(0xF30F1183C0010000, 8), 8);
	Cfg::Misc::isSpreadDisabled = false;
  }
}

void Features::UnlockAll(bool state) {
  auto pSynced = SyncedBFSettings::GetInstance();
  if (!IsValidPtr(pSynced)) return;
  pSynced->m_AllUnlocksUnlocked = state;
}
