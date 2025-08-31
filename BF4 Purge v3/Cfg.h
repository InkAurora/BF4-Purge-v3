#pragma once

#include "imgui.h"
#include "Engine.h"
#include <array>
#include <vector>
#include <string>

namespace Cfg {
  namespace ESP {
	extern bool aimbotFov;
	extern bool enable;
	extern bool use3DplayerBox;
	extern bool team;
	extern bool name; //TODO
	extern bool vehicles;
	extern bool spectators;

	extern bool prediction;
	extern bool predictionImpactData;
	extern bool predictionBombImpact;
	extern int predictionCrossRadius;
	extern bool predictionUseAngularVelocity;

	extern bool lines;
	extern bool alliesLines;
	extern bool linesVehicles;
	extern bool use3DvehicleBox;
	extern bool vehicleCenter;
	extern bool vehicleIndicator;

	namespace Radar {
	  extern bool enable;
	  extern bool onlyInVehicle;
	  extern int radius;
	  extern bool showVehicles;
	  extern float zoom;
	  extern int posX;
	  extern int posY;
	  extern int iconScale;

	  extern ImColor soldierColor;
	}

	extern bool explosives;
	extern bool ownMissile;

	extern int _internalCurveIterationCount;
	extern float _internalCurvePredTimeMultiplier;

	extern std::vector<unsigned int> _internalPlayerIDs;
	extern std::vector<unsigned int> _internalSelectedPlayerIDs;

	extern ImColor fovColor;
	extern ImColor teamColor;
	extern ImColor linesColor;
	extern ImColor linesVehicleAirColor;
	extern ImColor linesVehicleGroundColor;
	extern ImColor enemyColor;
	extern ImColor enemyColorVisible;
	extern ImColor vehicleAirColor;
	extern ImColor vehicleGroundColor;
	extern ImColor missileColor;
	extern ImColor explosivesColor;

	extern ImColor predictionDataColor;
	extern ImColor predictionCrossColor;
	extern ImColor predictionCrossOverrideColor;

	extern std::string validPlayers;
  }

  namespace AimBot {
	extern bool enable;
	extern UpdatePoseResultData::BONES bone;
	extern bool autoTrigger;
	extern bool teslaAutoPilot;
	extern float _internalSens;
	extern float radius;
	extern float smoothSoldier;
	extern float smoothVehicle;

	extern float smoothTV;

	extern bool autoBombs;

	extern bool forceTVAimbot;
	extern bool useTVPrediction;

	extern float TEST;
  }

  namespace DBG {
	extern bool vehicleCross;
	extern bool spectators;
	extern bool overlayBorder;
	extern bool console;
	extern bool watermark;
	extern bool debugOutput;
	extern bool debugEntities;
	extern bool jetSpeedCtrl;
	extern bool debugVar;

	extern bool dbgMode;
	extern float dbgPos[3];

	extern bool performanceTest;
	extern std::string testString;

	extern bool _internalSpoof;
	extern bool _internalRestore;
	extern char _internalName[16];

	extern std::array<Vector, 16> _internalPredictionCurve;
	extern Quaternion _internalPredictedOrientation;
	extern bool _internalUseCurve;

	extern unsigned int _internalFFSS;

	extern char spoofedName[16];
  }

  namespace Misc {
	extern bool showStats;
	extern bool minimapSpot;
	extern bool isMinimapSpotted;
	extern bool disableSpread;
	extern bool isSpreadDisabled;
	extern bool disableRecoil;
	extern bool isRecoilDisabled;
	extern bool unlockAll;
	extern bool noOverheat;
  }
}
