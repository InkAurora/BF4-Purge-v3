#pragma once

#include <array>
#include "Globals.h"
#include "Renderer.h"
#include "BoundingBox.h"
#include "Misc.h"
#include <algorithm>

class Visuals {
public:
  Visuals();

  static bool WorldToScreen(const Vector& origin, Vector& screen);
  static bool WorldToScreen(const Vector& origin, Vector2D& screen);
  static bool WorldToScreen(const Vector& origin, ImVec2& screen);
  static bool WorldToScreen(Vector& origin);
  static BoundingBox GetEntityAABB(ClientControllableEntity* pEntity, BoundingBox3D* pTransformed3D = nullptr);
  static VeniceClientMissileEntity* GetMissileEntity(ClientGameContext* pCtx, ClientPlayer* pLocal);
  static void RenderPlayerCorneredRect(const BoundingBox& bbEntity, const ImColor& color);
  static void RenderAimPoint(const PredictionData_s& data, ClientPlayer* pTargetData = nullptr);
  static void RenderBombImpact(const Vector& targetPos, WeaponData_s* pDataIn = nullptr);
  static void RenderExplosives(ClientGameContext* pCtx);
  static void RenderPlayerHealth(const BoundingBox& bbEntity);

public:
  static void RenderVisuals();
};
