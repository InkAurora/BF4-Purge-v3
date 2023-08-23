#pragma once

#include "Cfg.h"
#include "Misc.h"

class Features {
public:
  Features();

  DWORD64 moduleBase;

  void MinimapSpot(bool state);
  void Recoil(bool state);
  void Spread(bool state);
  void UnlockAll(bool state);
};
