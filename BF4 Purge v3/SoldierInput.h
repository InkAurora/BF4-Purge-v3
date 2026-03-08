#pragma once

#include "Engine.h"

struct SoldierAimContext {
  SoldierWeapon* weapon = nullptr;
  SoldierAimingSimulation* aimSimulation = nullptr;
  AimAssist* aimAssist = nullptr;
};

class ISoldierInputBackend {
public:
  virtual ~ISoldierInputBackend() = default;

  virtual bool ResolveAimContext(SoldierAimContext& context) = 0;
  virtual bool ApplyDirectAim(const SoldierAimContext& context, const Vector2D& angles) = 0;
  virtual void ReleaseDirectAim(const char* reason = nullptr) = 0;
};

class HybridSoldierInputBackend final : public ISoldierInputBackend {
public:
  bool ResolveAimContext(SoldierAimContext& context) override;
  bool ApplyDirectAim(const SoldierAimContext& context, const Vector2D& angles) override;
  void ReleaseDirectAim(const char* reason = nullptr) override;

private:
  bool m_directAimActive = false;
  SoldierWeapon* m_lastWeapon = nullptr;
  AimAssist* m_lastAimAssist = nullptr;
};

ISoldierInputBackend& GetSoldierInputBackend();