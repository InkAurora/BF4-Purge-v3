#include "Globals.h"

namespace DX {
  ImFont* Verdana8 = nullptr;
  ImFont* Verdana12 = nullptr;
  ImFont* Verdana18 = nullptr;
  ImFont* Verdana24 = nullptr;
  ImFont* Verdana48 = nullptr;
}

namespace G {
  int           FPS = 0;
  int           inputFPS = 0;
  int           framecount = 0;
  int           inputFramecount = 0;
  bool			isMenuVisible = false;
  bool			shouldExit = false;
  ImVec2		screenSize = { -1, -1 };
  ImVec2		screenCenter = { -1, -1 };
  D3DXVECTOR2	viewPos2D = { -1, -1 };
  D3DXVECTOR3	viewPos = { -1, -1, -1 };
  bool			targetLock = false;
  HINSTANCE		hInst = NULL;
  bool            matchEnded = false;
}

namespace F {
  std::unique_ptr<Visuals>    pVisuals;
  std::unique_ptr<Features>   pFeatures;
}

namespace PreUpdate {
  UpdateData_s preUpdatePlayersData;
  PredictionData_s predictionData;
  WeaponData_s weaponData;

  std::array<Vector, 8> points;
  bool visible;

  bool isValid;
  bool isPredicted;
  float angleY;
}
