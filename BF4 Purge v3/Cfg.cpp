#include "Cfg.h"

namespace Cfg {
  namespace ESP {
	bool aimbotFov = false;
	bool watermark = false;

	bool enable = true;
	bool use3DplayerBox = true;
	bool team = false;
	bool name = false;
	bool spectators = true;

	bool vehicles = true;
	bool use3DvehicleBox = true;
	bool vehicleCenter = true;

	bool prediction = true;
	bool predictionBombImpact = true;
	bool predictionImpactData = true;
	int predictionCrossRadius = 15;
	bool predictionUseAngularVelocity = false;

	int _internalCurveIterationCount = 16;
	float _internalCurvePredTimeMultiplier = 69.0f;

	bool lines = false;
	bool alliesLines = false;
	bool linesVehicles = true;
	bool vehicleIndicator = true;
    bool vehicleReticle = true;

	namespace Radar {
	  bool enable = false;
	  bool onlyInVehicle = false;
	  int radius = 100;
	  bool showVehicles = true;
	  float zoom = 1.f;
	  int posX = 300;
	  int posY = 300;
	  int iconScale = 32;

	  ImColor soldierColor = ImColor::Red();
	}

	bool explosives = true;
	bool ownMissile = true;

	std::vector<unsigned int> _internalPlayerIDs;
	std::vector<unsigned int> _internalSelectedPlayerIDs;

	ImColor fovColor = ImColor::White(130);
	ImColor teamColor = ImColor(52, 192, 235);
	ImColor linesColor = ImColor(252, 119, 3);
	ImColor linesVehicleAirColor = ImColor(0, 142, 255, 90);
	ImColor linesVehicleGroundColor = ImColor::Orange(90);
	ImColor enemyColor = ImColor(235, 146, 52);
	ImColor enemyColorVisible = ImColor(255, 10, 25);
	ImColor vehicleAirColor = ImColor(0, 142, 255, 90);
	ImColor vehicleGroundColor = ImColor(255, 156, 56, 90);
	ImColor missileColor = ImColor::Red();

	ImColor explosivesColor = ImColor::Pink();

	ImColor predictionCrossColor = ImColor(23, 206, 176);
	ImColor predictionDataColor = ImColor(23, 206, 176);
	ImColor predictionCrossOverrideColor = ImColor::Pink();

	std::string validPlayers = "";
  }

  namespace AimBot {
	bool enable = true;
	UpdatePoseResultData::BONES bone = UpdatePoseResultData::BONES::Head;
	bool autoTrigger = false;
    bool targetLock = false;
	bool teslaAutoPilot = false;
	float _internalSens = 12.f;
	float radius = (float)ESP::predictionCrossRadius * 3;
	float smoothSoldier = 3.2f;
	float smoothVehicle = 2.5f;

	float smoothTV = 1.f;

	bool autoBombs = true;

	bool forceTVAimbot = false;
	bool useTVPrediction = false;

	float TEST = 0.7f;
  }

  namespace DBG {
	bool vehicleCross = true;
	bool spectators = false;
	bool overlayBorder = false;
	bool console = true;
	bool watermark = true;
	bool debugOutput = false;
	bool debugEntities = false;
	bool jetSpeedCtrl = true;
	bool debugVar = false;

	bool dbgMode = false;
	float dbgPos[3] = { 0.0f, 0.0f, 0.0f };

	bool performanceTest = false;
	std::string testString = "";

	std::array<Vector, 16> _internalPredictionCurve;
	Quaternion _internalPredictedOrientation;
	bool _internalUseCurve = false;

	unsigned int _internalFFSS = 0;

	bool _internalSpoof = false;
	bool _internalRestore = false;
	char _internalName[16];
	char spoofedName[16];
  }

  namespace Misc {
	bool showStats = true;
	bool minimapSpot = true;
	bool isMinimapSpotted = false;
	bool disableSpread = false;
	bool isSpreadDisabled = false;
	bool disableRecoil = false;
	bool isRecoilDisabled = false;
	bool unlockAll = false;
	bool noOverheat = false;
  }
}
