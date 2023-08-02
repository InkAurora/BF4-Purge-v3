#include "Globals.h"

namespace DX {
  ImFont* Verdana8 = nullptr;
  ImFont* Verdana12 = nullptr;
  ImFont* Verdana18 = nullptr;
  ImFont* Verdana24 = nullptr;
  ImFont* Verdana48 = nullptr;
}

namespace G {
  bool			isMenuVisible = false;
  bool			shouldExit = false;
  ImVec2		screenSize = { -1, -1 };
  ImVec2		screenCenter = { -1, -1 };
  D3DXVECTOR2	viewPos2D = { -1, -1 };
  D3DXVECTOR3	viewPos = { -1, -1, -1 };
  bool			targetLock = false;
  HINSTANCE		hInst = NULL;
}

namespace PreUpdate {
  UpdateData_s preUpdatePlayersData;
  PredictionData_s predictionData;
  WeaponData_s weaponData;

  std::array<Vector, 8> points;
  bool visible;

  bool isValid;
  bool isPredicted;
  double perf = 0;
  float angleY;
}
