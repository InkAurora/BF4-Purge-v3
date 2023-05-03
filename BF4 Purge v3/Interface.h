#pragma once

#include "includes.h"
#include "Visuals.h"

class Interface {
public:
  static bool showMenu;
  static bool showSpectators;

  static bool InitializeVisuals();
  static bool ShutdownVisuals();
};
