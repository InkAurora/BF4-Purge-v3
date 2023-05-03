#pragma once

#include <array>
#include "Globals.h"
#include "Renderer.h"
#include "BoundingBox.h"

class Visuals {
public:
  Visuals();

  static bool WorldToScreen(const Vector& origin, Vector& screen);
  static bool WorldToScreen(const Vector& origin, Vector2D& screen);
  static bool WorldToScreen(const Vector& origin, ImVec2& screen);
  static bool WorldToScreen(Vector& origin);
  static BoundingBox GetEntityAABB(ClientControllableEntity* pEntity, BoundingBox3D* pTransformed3D = nullptr);
  static void RenderPlayerCorneredRect(const BoundingBox& bbEntity, const ImColor& color);


public:
  static void RenderVisuals();
};
