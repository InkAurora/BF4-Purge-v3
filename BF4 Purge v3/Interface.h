#pragma once

#include "includes.h"
#include "Visuals.h"
#include "PreFrameUpdate.h"

class Interface {
public:
  static bool InitializeVisuals();
  static bool ShutdownVisuals();
};
