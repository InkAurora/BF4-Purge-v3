#pragma once

#include "Engine.h"
#include "imgui.h"
#include "Visuals.h"
#include <vector>
#include <memory>
#include "MiscFeatures.h"

namespace DX {
  extern ImFont* Verdana8;
  extern ImFont* Verdana12;
  extern ImFont* Verdana18;
  extern ImFont* Verdana24;
  extern ImFont* Verdana48;
}

namespace G {
  extern int FPS;
  extern int inputFPS;
  extern bool isMenuVisible;
  extern bool shouldExit;
  extern ImVec2 screenSize;
  extern ImVec2 screenCenter;
  extern D3DXVECTOR2 viewPos2D;
  extern D3DXVECTOR3 viewPos;
  extern bool targetLock;
  extern HINSTANCE hInst;
}

class Visuals;
class Features;

namespace F {
  extern std::unique_ptr<Visuals>     pVisuals;
  extern std::unique_ptr<Features>    pFeatures;
}

struct UpdateData_s {
  ClientPlayer* pBestTarget = nullptr;
  VeniceClientMissileEntity* pMyMissile = nullptr;
  Vector origin = {};
  std::vector<bool> visiblePlayers = std::vector<bool>(70, false);
};

namespace PreUpdate {
  extern UpdateData_s preUpdatePlayersData;
  extern PredictionData_s predictionData;
  extern WeaponData_s weaponData;

  extern std::array<Vector, 8> points;
  extern bool visible;

  extern bool isValid;
  extern bool isPredicted;
  extern float angleY;
}
