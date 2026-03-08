#pragma once

#include "Engine.h"

enum class VehicleTurretInputMode {
  MouseDevice = 0,
  CrosshairConcepts = 1,
  CameraConcepts = 2,
  RightStickConcepts = 3,
  ActionMapCrosshair = 4,
};

const char* GetVehicleTurretInputModeName(VehicleTurretInputMode mode);
bool InitializeVehicleInputHooks();
void ShutdownVehicleInputHooks();
void ApplyPendingVehicleInputNodeOverlay(void* inputNodeState);
void LogVehicleInputNodeDiagnostics(void* inputNodeState, int seatId);

struct VehicleAxesInput {
  bool writeYaw = false;
  bool writePitch = false;
  bool writeRoll = false;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
};

class IVehicleInputBackend {
public:
  virtual ~IVehicleInputBackend() = default;

  virtual bool ApplyAxes(const VehicleAxesInput& axes) = 0;
  virtual bool ApplyTurretLook(const Vector2D& deltaVec, float smooth) = 0;
  virtual bool ResetTurretLook() = 0;
};

class HybridVehicleInputBackend final : public IVehicleInputBackend {
public:
  bool ApplyAxes(const VehicleAxesInput& axes) override;
  bool ApplyTurretLook(const Vector2D& deltaVec, float smooth) override;
  bool ResetTurretLook() override;

private:
  bool ApplyTurretLookMouse(const Vector2D& deltaVec, float smooth);
  bool ApplyTurretLookConcepts(const Vector2D& deltaVec, float smooth, VehicleTurretInputMode mode);
  bool ApplyTurretLookActionMap(const Vector2D& deltaVec, float smooth, VehicleTurretInputMode mode);
  bool ResetTurretLookMouse();
  bool ResetTurretLookConcepts();
  bool ResetTurretLookActionMap();
};

IVehicleInputBackend& GetVehicleInputBackend();
