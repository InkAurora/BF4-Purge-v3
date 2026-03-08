#pragma once

#include "Engine.h"

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
};

IVehicleInputBackend& GetVehicleInputBackend();
