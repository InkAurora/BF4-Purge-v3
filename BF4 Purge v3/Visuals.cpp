#include "Visuals.h"
#include "xorstr.hpp"
#include "Cfg.h"

static bool targetLock = false;
static ClientPlayer* pTargetPlayer = nullptr;

struct ClassInfos_s {
  ClassInfo* MissileEntity = nullptr;
  ClassInfo* ExplosionEntity = nullptr;
  ClassInfo* VehicleEntity = nullptr;
  ClassInfo* WarningComponent = nullptr;
} ClassInfos;

Visuals::Visuals() {
  ClassInfos.MissileEntity = FindClassInfo(xorstr_("VeniceClientMissileEntity"));
  ClassInfos.ExplosionEntity = FindClassInfo(xorstr_("ClientExplosionPackEntity"));
  ClassInfos.VehicleEntity = FindClassInfo(xorstr_("ClientVehicleEntity"));
  ClassInfos.WarningComponent = FindClassInfo(xorstr_("ClientWarningSystemComponent"));
}

bool Visuals::WorldToScreen(const Vector& origin, Vector& screen) {
  auto* pRenderer = GameRenderer::GetInstance();
  if (!pRenderer) return false;

  auto* pRenderView = pRenderer->m_pRenderView;;
  if (!pRenderView) return false;

  auto* pDxRenderer = DxRenderer::GetInstance();
  if (!pDxRenderer) return false;

  auto* pScreen = pDxRenderer->m_pScreen;
  if (!pScreen) return false;

  float mX = static_cast<float>(pScreen->m_Width * 0.5f);
  float mY = static_cast<float>(pScreen->m_Height * 0.5f);

  D3DXMATRIX screenTransform = pRenderView->m_ViewProjection;

  float w =
	screenTransform(0, 3) * origin.x +
	screenTransform(1, 3) * origin.y +
	screenTransform(2, 3) * origin.z +
	screenTransform(3, 3);

  if (w < 0.0001f) {
	screen.z = w;

	return false;
  }

  float x =
	screenTransform(0, 0) * origin.x +
	screenTransform(1, 0) * origin.y +
	screenTransform(2, 0) * origin.z +
	screenTransform(3, 0);

  float y =
	screenTransform(0, 1) * origin.x +
	screenTransform(1, 1) * origin.y +
	screenTransform(2, 1) * origin.z +
	screenTransform(3, 1);

  screen.x = mX + mX * x / w;
  screen.y = mY - mY * y / w;
  screen.z = w;

  return true;
}

bool Visuals::WorldToScreen(const Vector& origin, Vector2D& screen) {
  auto* pRenderer = GameRenderer::GetInstance();
  if (!pRenderer) return false;

  auto* pRenderView = pRenderer->m_pRenderView;;
  if (!pRenderView) return false;

  auto* pDxRenderer = DxRenderer::GetInstance();
  if (!pDxRenderer) return false;

  auto* pScreen = pDxRenderer->m_pScreen;
  if (!pScreen) return false;

  float mX = static_cast<float>(pScreen->m_Width * 0.5f);
  float mY = static_cast<float>(pScreen->m_Height * 0.5f);

  D3DXMATRIX screenTransform = pRenderView->m_ViewProjection;

  float w =
	screenTransform(0, 3) * origin.x +
	screenTransform(1, 3) * origin.y +
	screenTransform(2, 3) * origin.z +
	screenTransform(3, 3);

  if (w < 0.0001f) return false;

  float x =
	screenTransform(0, 0) * origin.x +
	screenTransform(1, 0) * origin.y +
	screenTransform(2, 0) * origin.z +
	screenTransform(3, 0);

  float y =
	screenTransform(0, 1) * origin.x +
	screenTransform(1, 1) * origin.y +
	screenTransform(2, 1) * origin.z +
	screenTransform(3, 1);

  screen.x = mX + mX * x / w;
  screen.y = mY - mY * y / w;

  return true;
}

bool Visuals::WorldToScreen(const Vector& origin, ImVec2& screen) {
  D3DXMATRIXA16 viewProj = GameRenderer::GetInstance()->m_pRenderView->m_ViewProjection;

  float mX = DxRenderer::GetInstance()->m_pScreen->m_Width * 0.5f;
  float mY = DxRenderer::GetInstance()->m_pScreen->m_Height * 0.5f;

  float w =
	viewProj(0, 3) * origin.x +
	viewProj(1, 3) * origin.y +
	viewProj(2, 3) * origin.z +
	viewProj(3, 3);

  if (w < 0.65f) {
	//ScreenPos->z = w;
	return false;
  }

  float x =
	viewProj(0, 0) * origin.x +
	viewProj(1, 0) * origin.y +
	viewProj(2, 0) * origin.z +
	viewProj(3, 0);

  float y =
	viewProj(0, 1) * origin.x +
	viewProj(1, 1) * origin.y +
	viewProj(2, 1) * origin.z +
	viewProj(3, 1);

  screen.x = mX + mX * x / w;
  screen.y = mY - mY * y / w;
  //ScreenPos->z = w;

  return true;
}

bool Visuals::WorldToScreen(Vector& origin) {
  D3DXMATRIXA16 viewProj = GameRenderer::GetInstance()->m_pRenderView->m_ViewProjection;

  float mX = DxRenderer::GetInstance()->m_pScreen->m_Width * 0.5f;
  float mY = DxRenderer::GetInstance()->m_pScreen->m_Height * 0.5f;

  float w =
	viewProj(0, 3) * origin.x +
	viewProj(1, 3) * origin.y +
	viewProj(2, 3) * origin.z +
	viewProj(3, 3);

  if (w < 0.65f) {
	//ScreenPos->z = w;
	return false;
  }

  float x =
	viewProj(0, 0) * origin.x +
	viewProj(1, 0) * origin.y +
	viewProj(2, 0) * origin.z +
	viewProj(3, 0);

  float y =
	viewProj(0, 1) * origin.x +
	viewProj(1, 1) * origin.y +
	viewProj(2, 1) * origin.z +
	viewProj(3, 1);

  origin.x = mX + mX * x / w;
  origin.y = mY - mY * y / w;
  //ScreenPos->z = w;

  return true;
}

BoundingBox Visuals::GetEntityAABB(ClientControllableEntity* pEntity, BoundingBox3D* pTransformed3D) {
  TransformAABBStruct TransAABB;
  D3DXMATRIX matrix;
  pEntity->GetTransform(&matrix);
  pEntity->GetAABB(&TransAABB);

  auto min = Vector(TransAABB.AABB.m_Min.x, TransAABB.AABB.m_Min.y, TransAABB.AABB.m_Min.z);
  auto max = Vector(TransAABB.AABB.m_Max.x, TransAABB.AABB.m_Max.y, TransAABB.AABB.m_Max.z);

  auto MultiplyMat = [](const Vector& vec, D3DXMATRIX* mat) -> Vector {
	return Vector(mat->_11 * vec.x + mat->_21 * vec.y + mat->_31 * vec.z,
	  mat->_12 * vec.x + mat->_22 * vec.y + mat->_32 * vec.z,
	  mat->_13 * vec.x + mat->_23 * vec.y + mat->_33 * vec.z);
  };

  Vector points[] =
  {
	  Vector(min.x, min.y, min.z),
	  Vector(min.x, max.y, min.z),
	  Vector(max.x, max.y, min.z),
	  Vector(max.x, min.y, min.z),
	  Vector(max.x, max.y, max.z),
	  Vector(min.x, max.y, max.z),
	  Vector(min.x, min.y, max.z),
	  Vector(max.x, min.y, max.z)
  };

  Vector pointsTransformed[8] = {};
  auto pos = Vector(matrix._41, matrix._42, matrix._43);
  for (int i = 0; i < 8; i++)
	pointsTransformed[i] = pos + MultiplyMat(points[i], &matrix);

  if (pTransformed3D) {
	for (int i = 0; i < 8; i++)
	  pTransformed3D->points.at(i) = pointsTransformed[i];
	pTransformed3D->min = pos + MultiplyMat(min, &matrix);
	pTransformed3D->max = pos + MultiplyMat(max, &matrix);
  }

  Vector screenPoints[8] = {};
  for (int i = 0; i < 8; i++) {
	if (!WorldToScreen(pointsTransformed[i], screenPoints[i]))
	  return BoundingBox::Zero();
  }

  auto left = screenPoints[0].x;
  auto top = screenPoints[0].y;
  auto right = screenPoints[0].x;
  auto bottom = screenPoints[0].y;

  for (int i = 1; i < 8; i++) {
	if (left > screenPoints[i].x)
	  left = screenPoints[i].x;
	if (top < screenPoints[i].y)
	  top = screenPoints[i].y;
	if (right < screenPoints[i].x)
	  right = screenPoints[i].x;
	if (bottom > screenPoints[i].y)
	  bottom = screenPoints[i].y;
  }

  return BoundingBox(left, bottom, right, top);
}

VeniceClientMissileEntity* Visuals::GetMissileEntity(ClientGameContext* pCtx, ClientPlayer* pLocal) {
  if (!IsValidPtr(pCtx->m_pLevel) || !IsValidPtr(pCtx->m_pLevel->m_pGameWorld)) return nullptr;

  auto* pGameWorld = pCtx->m_pLevel->m_pGameWorld;
  if (!IsValidPtr(pGameWorld)) return nullptr;

  if (ClassInfos.MissileEntity) {
	EntityIterator<VeniceClientMissileEntity> missiles(pGameWorld, ClassInfos.MissileEntity);
	VeniceClientMissileEntity* pMyMissile = nullptr;

	//Finding own missile
	if (missiles.front()) {
	  do {
		if (auto* pMissile = missiles.front()->getObject(); IsValidPtr(pMissile)) {
		  if (pMissile->m_pOwner.GetData() == pLocal) {
			pMyMissile = pMissile;
			break;
		  }
		}

	  } while (missiles.next());
	}

	if (!IsValidPtr(pMyMissile)) return nullptr;
	if (!IsValidPtr(pMyMissile->m_pMissileEntityData)) return nullptr;

	if (pMyMissile->m_pMissileEntityData->IsLockable()) return nullptr;

	const bool isTOW = pMyMissile->m_pMissileEntityData->IsTOW();

	if (Cfg::ESP::ownMissile && isTOW) {
	  const auto& missileBB = GetEntityAABB((ClientControllableEntity*)pMyMissile);
	  ImVec2 scrn = missileBB.GetMin() + missileBB.GetCenter();

	  std::vector<ImVec2> points =
	  {
		  ImVec2(scrn.x, scrn.y - 5),
		  ImVec2(scrn.x + 5, scrn.y),
		  ImVec2(scrn.x, scrn.y + 5),
		  ImVec2(scrn.x - 5, scrn.y),
		  ImVec2(scrn.x, scrn.y - 5),
	  };

	  auto fill = Cfg::ESP::missileColor;
	  fill.Value.w = std::clamp(Cfg::ESP::missileColor.Value.w - 0.5f, 0.0f, 1.0f);
	  ImGui::GetForegroundDrawList()->AddConvexPolyFilled(points.data(), points.size(), fill);
	  Renderer::DrawLine(points, Cfg::ESP::missileColor);

	}

	return pMyMissile;
  }
  return nullptr;
}

void Visuals::RenderPlayerCorneredRect(const BoundingBox& bbEntity, const ImColor& color) {
  BoundingBox bb = bbEntity;

  float width = abs(bb.GetSize().x * 0.2f);
  float height = abs(bb.GetSize().x * 0.2f);

  if (bb.GetSize().x <= width) width = bb.GetSize().x * 0.2f;
  if (bb.GetSize().x <= height) height = bb.GetSize().x * 0.2f;

  bb.left = std::floorf(bbEntity.left);
  bb.top = std::floorf(bbEntity.top);
  bb.right = std::floorf(bbEntity.right);
  bb.bot = std::floorf(bbEntity.bot);

  ImColor cBG = ImColor::Black((int)(color.Value.w * 255));
  ImColor cFG = color;

  /*	2			3
	   ____	____
  1	|a		   b|	4
	  |			|



  8	|			|	5
	  |d___	___c|

	  7			6
  */

  // 1
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMin().x - 1, bb.GetMin().y),
	ImVec2(bb.GetMin().x + 1, bb.GetMin().y + height + 1),
	cBG);

  // 2
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMin().x - 1, bb.GetMin().y - 1),
	ImVec2(bb.GetMin().x + width + 1, bb.GetMin().y + 1),
	cBG);

  // 3
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMax().x - width - 1, bb.GetMin().y - 1),
	ImVec2(bb.GetMax().x + 1, bb.GetMin().y + 1),
	cBG);

  // 4
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMax().x - 1, bb.GetMin().y - 1),
	ImVec2(bb.GetMax().x + 1, bb.GetMin().y + height + 1),
	cBG);

  // 5
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMax().x - 1, bb.GetMax().y - height - 1),
	ImVec2(bb.GetMax().x + 1, bb.GetMax().y),
	cBG);

  // 6
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMax().x - width - 1, bb.GetMax().y - 1),
	ImVec2(bb.GetMax().x + 1, bb.GetMax().y + 1),
	cBG);

  // 7
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMin().x - 1, bb.GetMax().y - 1),
	ImVec2(bb.GetMin().x + width + 1, bb.GetMax().y + 1),
	cBG);

  // 8
  Renderer::DrawRectFilled(
	ImVec2(bb.GetMin().x - 1, bb.GetMax().y - height - 1),
	ImVec2(bb.GetMin().x + 1, bb.GetMax().y + 1),
	cBG);

  // a
  Renderer::DrawLine(bb.GetMin(), ImVec2(bb.GetMin().x + width, bb.GetMin().y), cFG);
  Renderer::DrawLine(bb.GetMin(), ImVec2(bb.GetMin().x, bb.GetMin().y + height), cFG);

  // b
  Renderer::DrawLine(ImVec2(bb.GetMax().x - width, bb.GetMin().y), bb.GetMaxTop(), cFG);
  Renderer::DrawLine(ImVec2(bb.GetMax().x - 1, bb.GetMin().y + height - 1), ImVec2(bb.GetMaxTop().x - 1, bb.GetMaxTop().y), cFG);

  // c
  Renderer::DrawLine(ImVec2(bb.GetMax().x - width, bb.GetMax().y - 1), ImVec2(bb.GetMax().x, bb.GetMax().y - 1), cFG);
  Renderer::DrawLine(ImVec2(bb.GetMax().x - 1, bb.GetMax().y - height), ImVec2(bb.GetMax().x - 1, bb.GetMax().y), cFG);

  // d
  Renderer::DrawLine(ImVec2(bb.GetMinBot().x, bb.GetMinBot().y - 1), ImVec2(bb.GetMin().x + width, bb.GetMax().y - 1), cFG);
  Renderer::DrawLine(ImVec2(bb.GetMin().x, bb.GetMax().y - height), bb.GetMinBot(), cFG);
}

void Visuals::RenderAimPoint(const PredictionData_s& data, ClientPlayer* pTargetData) {
  ImVec2 predAimPoint, currPos;
#ifndef AIMPOINT_DBG
  if (!WorldToScreen(data.hitPos, predAimPoint)) return;
  WorldToScreen(data.origin, currPos);
#endif

#ifdef AIMPOINT_DBG
  predAimPoint = { 200, 400 };
  currPos = { 269, 600 };
#endif

  ImVec2 lastCurvePoint;
  if (Cfg::DBG::_internalUseCurve && Cfg::ESP::_internalCurveIterationCount > 0) {
	const auto& points = Cfg::DBG::_internalPredictionCurve;
	auto colorDelta = 255 / points.size();
	for (int i = 0; i < Cfg::ESP::_internalCurveIterationCount - 1; i++) {
	  ImVec2 screen, screen2;
	  if (WorldToScreen(points.at(i), screen) && WorldToScreen(points.at(i + 1), screen2)) {
		auto color = ImColor(255 - (colorDelta * i), 0 + (colorDelta * i), 128);
		ImGui::GetBackgroundDrawList()->AddLine(screen, screen2, ImColor::Black(255 * Cfg::ESP::predictionCrossColor.Value.w), 2.0f);
		Renderer::DrawLine(screen, screen2, color);
		lastCurvePoint = screen2;
	  }
	}

	if (IsValidPtr(pTargetData) && !pTargetData->InVehicle()) lastCurvePoint = currPos;
  }
  else lastCurvePoint = currPos;

  Vector2D crossDelta;
  Vector2D tmp = { predAimPoint.x, predAimPoint.y };
  D3DXVec2Subtract(&crossDelta, &G::viewPos2D, &tmp);

  const ImVec2& p1 = predAimPoint;
  const ImVec2& p2 = lastCurvePoint;

  float angle;
  auto v = ImVec2(p2.x - p1.x, p2.y - p1.y);

  const float a = fabsf(p2.y - p1.y);
  const float b = fabsf(p2.x - p1.x);
  const float c = sqrtf((v.x * v.x) + (v.y * v.y));
  const auto& rad = Cfg::ESP::predictionCrossRadius;
  static constexpr float quarterPI = D3DXToRadian(90.f);

  if (c > rad) {
	//Cancer way of rotating point around the circle...
	if (p2.y <= p1.y) {
	  if (p2.x >= p1.x) angle = asinf(a / c);
	  else angle = acosf(a / c) + quarterPI;
	}
	else {
	  if (p2.x <= p1.x) angle = acosf(b / c) + 2 * quarterPI;
	  else angle = asinf(b / c) + 3 * quarterPI;
	}

	ImVec2 point =
	{
		p1.x + (rad * cosf(-angle)),
		p1.y + (rad * sinf(-angle))
	};

	ImGui::GetBackgroundDrawList()->AddLine(point, lastCurvePoint, ImColor::Black(255 * Cfg::ESP::predictionCrossColor.Value.w), 2.0f);
	Renderer::DrawLine(point, lastCurvePoint, Cfg::ESP::predictionCrossColor);
	Renderer::DrawCircleFilled(lastCurvePoint, 2, 0, Cfg::ESP::predictionCrossColor);
  }

  const bool release = D3DXVec2Length(&crossDelta) <= rad;
  ImColor crossColor = release ? Cfg::ESP::predictionCrossOverrideColor : Cfg::ESP::predictionCrossColor;

  if (pTargetData) {
	float maxHp = 0.0f, currHp = 0.0f;
	if (auto pVeh = pTargetData->GetClientVehicleEntity(); pTargetData->InVehicle() && IsValidPtr(pVeh) && IsValidPtr(pVeh->m_pData) && IsValidPtr(pVeh->m_pHealthComp)) {
	  maxHp = pVeh->m_pHealthComp->m_MaxHealth;
	  currHp = pVeh->m_pHealthComp->m_VehicleHealth;
	}
	else if (auto pSold = pTargetData->GetSoldierEntity(); IsValidPtr(pSold) && IsValidPtr(pSold->m_pHealthComp)) {
	  maxHp = pSold->m_pHealthComp->m_MaxHealth;
	  currHp = pSold->m_pHealthComp->m_Health;
	}

	currHp = std::clamp(currHp, 0.0f, maxHp);
	const auto percHp = (currHp * maxHp) / 100.f;

	if (percHp < 100.f && percHp > 0.f) {
	  auto hpCol = Cfg::ESP::predictionCrossColor(190);
	  BoundingBox bbHp = { { p1.x - rad, p1.y - rad - 20.f }, { p1.x + rad, p1.y - rad - 16.f } };

	  Renderer::DrawRectFilled(bbHp.GetMin(), { bbHp.GetMax().x + 2, bbHp.GetMax().y + 1 }, ImColor::Black(120));
	  Renderer::DrawRectFilled(
		bbHp.GetMin() + 1.f,
		{ bbHp.GetMin().x + 1.f + (currHp * bbHp.GetSize().x / maxHp), bbHp.GetMax().y },
		hpCol);
	}
  }

  ImVec2 lastPoint[2];
  Renderer::DrawCircleFilled(p1, rad, 25, ImColor::Black(50));
  Renderer::DrawCircle(p1, 2, 25, crossColor);
  Renderer::DrawCircleOutlined(p1, rad, 25, crossColor);
  Renderer::DrawCircleProgressBar(p1, rad - 2, 25, data.travelTime, 4.f, crossColor, 1.0f, false, lastPoint);
  Renderer::DrawLine(lastPoint[1], lastPoint[0], crossColor);

  //Cross
  ImGui::GetBackgroundDrawList()->AddLine({ p1.x - 20, p1.y }, { p1.x - rad, p1.y }, ImColor::Black(255 * Cfg::ESP::predictionCrossColor.Value.w), 2.0f);
  ImGui::GetBackgroundDrawList()->AddLine({ p1.x + rad, p1.y }, { p1.x + 20, p1.y }, ImColor::Black(255 * Cfg::ESP::predictionCrossColor.Value.w), 2.0f);
  ImGui::GetBackgroundDrawList()->AddLine({ p1.x, p1.y - rad }, { p1.x, p1.y - 20 }, ImColor::Black(255 * Cfg::ESP::predictionCrossColor.Value.w), 2.0f);
  ImGui::GetBackgroundDrawList()->AddLine({ p1.x, p1.y + rad }, { p1.x, p1.y + 20 }, ImColor::Black(255 * Cfg::ESP::predictionCrossColor.Value.w), 2.0f);

  Renderer::DrawLine({ p1.x - 20, p1.y }, { p1.x - rad, p1.y }, crossColor);
  Renderer::DrawLine({ p1.x + rad, p1.y }, { p1.x + 20, p1.y }, crossColor);
  Renderer::DrawLine({ p1.x, p1.y - rad }, { p1.x, p1.y - 20 }, crossColor);
  Renderer::DrawLine({ p1.x, p1.y + rad }, { p1.x, p1.y + 20 }, crossColor);

  if (Cfg::ESP::predictionImpactData) {
	ImGui::PushFont(DX::Verdana8);
	Renderer::DrawString({ p1.x + 30, p1.y - 13 }, 0, Cfg::ESP::predictionDataColor, xorstr_("T: %.1fs."), data.travelTime);
	Renderer::DrawString({ p1.x + 30, p1.y - 4 }, 0, Cfg::ESP::predictionDataColor, xorstr_("D: %dm."), (int)data.distance);
	Renderer::DrawString({ p1.x + 30, p1.y + 5 }, 0, Cfg::ESP::predictionDataColor, xorstr_("V: %dm/s."), (int)data.bulletVel);
	ImGui::PopFont();
  }

  if (targetLock) {
	Renderer::DrawString({ p1.x - 32, p1.y - 2 }, StringFlag::RIGHT_X | StringFlag::CENTER_Y, ImColor::Black(), xorstr_("LOCKED"));
	Renderer::DrawString({ p1.x - 30, p1.y }, StringFlag::RIGHT_X | StringFlag::CENTER_Y, ImColor(223, 32, 32), xorstr_("LOCKED"));
  }
}

void Visuals::RenderBombImpact(const Vector& targetPos, WeaponData_s* pDataIn) {
  if (Cfg::ESP::predictionBombImpact && IsValidPtr(pDataIn)) {
	if (pDataIn->isValid) {
	  if (pDataIn->gravity == 0.0f && pDataIn->speed.z == 0.0f) {
		auto pBomb = reinterpret_cast<ClientIndirectFireWeapon*>(pDataIn->pWeapon);
		if (IsValidPtr(pBomb)) {
		  auto impactPos = Vector(pBomb->landingpos.x, pBomb->landingpos.y, pBomb->landingpos.z);
		  bool inRange = false;

		  if (targetPos != ZERO_VECTOR)
			inRange = Misc::Distance2D(Vector2D(targetPos.x, targetPos.z), Vector2D(impactPos.x, impactPos.z)) <= 5.f;;

		  ImVec2 impactPos2D, target2D;
		  WorldToScreen(impactPos, impactPos2D);

		  if (inRange) {
			Renderer::DrawString({ impactPos2D.x, impactPos2D.y - Cfg::ESP::predictionCrossRadius - 10.f },
			  StringFlag::CENTER_X, ImColor::Black(), xorstr_("DROP NOW!"));
			Renderer::DrawString({ impactPos2D.x + 1, impactPos2D.y - Cfg::ESP::predictionCrossRadius - 9.f },
			  StringFlag::CENTER_X, ImColor::Red(), xorstr_("DROP NOW!"));
		  }

		  Renderer::DrawCircleFilled(impactPos2D, Cfg::ESP::predictionCrossRadius, 0,
			inRange ? ImColor::Green(120) : ImColor::Red(120));
		  Renderer::DrawCircleOutlined(impactPos2D, Cfg::ESP::predictionCrossRadius, 0, ImColor::White(120));

		  if (targetPos != ZERO_VECTOR) {
			WorldToScreen(targetPos, target2D);

			if (target2D != ImVec2(0, 0))
			  Renderer::DrawLine(impactPos2D, target2D, ImColor::Green(120));
		  }
		}
	  }
	}
  }
}

void Visuals::RenderExplosives(ClientGameContext* pCtx) {
  if (!IsValidPtr(pCtx->m_pLevel) || !IsValidPtr(pCtx->m_pLevel->m_pGameWorld)) return;

  ClientGameWorld* pGameWorld = pCtx->m_pLevel->m_pGameWorld;
  if (!IsValidPtr(pGameWorld)) return;

  if (ClassInfos.MissileEntity) {
	EntityIterator<ClientExplosionEntity> explosives(pGameWorld, ClassInfos.ExplosionEntity);

	if (explosives.front()) {
	  do {
		if (auto* pExplosives = explosives.front()->getObject(); IsValidPtr(pExplosives)) {
		  BoundingBox3D explosivesAABB3D;
		  GetEntityAABB((ClientControllableEntity*)pExplosives, &explosivesAABB3D);
		  std::array<ImVec2, 8> points2D;

		  bool valid = true;
		  for (int i = 0; i < 8; i++) {
			if (!WorldToScreen(explosivesAABB3D.points[i], points2D[i])) {
			  valid = false;
			  break;
			}
		  }
		  if (valid) Renderer::DrawBox(points2D, Cfg::ESP::explosivesColor);

		}

	  } while (explosives.next());
	}
  }
}

void Visuals::RenderPlayerHealth(const BoundingBox& bbEntity) {
  //TODO:
}

void Visuals::RenderStats() {
  uintptr_t p_Stats = *(uintptr_t*)(0x142737A40);
  int Shots = *(uintptr_t*)(p_Stats + 0x4C);
  static int oldShots = 0;
  int Hit = *(uintptr_t*)(p_Stats + 0x54);
  static int oldHit = 0;
  if (G::matchEnded) {
    oldShots = Shots;
    oldHit = Hit;
  }
  float accuracy = (Shots - oldShots) > 0 ? ((float)Hit - oldHit) / (Shots - oldShots) * 100.0f : 0.0f;
  Renderer::DrawString({ G::screenSize.x / 2, 10 }, StringFlag::CENTER_X,ImColor(245, 150, 10), xorstr_("Accuracy: %.1f%%"), accuracy);
}

void Visuals::RenderVisuals() {
  if (!Cfg::ESP::enable) return;

  if (Cfg::Misc::showStats) RenderStats();

  Renderer::DrawString({ 50 - 32, 9 - 2 }, StringFlag::CENTER_Y, ImColor::Black(),
	xorstr_("FPS: %d        BIN_FPS: %d"), G::FPS, G::inputFPS);
  Renderer::DrawString({ 50 - 30, 9 }, StringFlag::CENTER_Y, ImColor(223, 32, 32),
	xorstr_("FPS: %d        BIN_FPS: %d"), G::FPS, G::inputFPS);

  ClientGameContext* pGameCtx = ClientGameContext::GetInstance();

  PlayerManager* pPlayerMgr = PlayerManager::GetInstance();

  ClientPlayer* pLocal = pPlayerMgr->GetLocalPlayer();

  // Prevent crashes when leaving the server or loading
  if (auto* pLocalSoldier = pLocal->GetSoldierEntity();
	!IsValidPtr(pLocal) ||
	!IsValidPtr(pLocalSoldier) ||
	!pLocalSoldier->IsAlive()) return;

  auto pMyMissile = GetMissileEntity(pGameCtx, pLocal);

  if (Cfg::ESP::explosives)
	RenderExplosives(pGameCtx);

  if (Cfg::ESP::vehicleReticle && pLocal->InVehicle()) {
	static auto prevPos = G::viewPos2D;
	auto delta = Misc::GetAbsDeltaAtGivenPoints(G::viewPos2D, prevPos);
	if (delta > 3) {
	  Vector2D d = G::viewPos2D - prevPos;
	  prevPos += d / 6.f;
	}
	else { prevPos = G::viewPos2D; }
	// TODO: Draw reticle based on actual world position (raycast forward vector). i.e. render world3dtoscreen ray hit position
	Renderer::DrawCircleOutlined(prevPos, 5, 15, ImColor::Red());
	Renderer::DrawRectFilled(prevPos, { prevPos.x + 1.f, prevPos.y + 1.f }, ImColor::Red());
  }

  Vector aimPoint3D = ZERO_VECTOR;
  float longestDelta = FLT_MAX;
  static int targetID = -1;

  if (!targetLock && Cfg::AimBot::enable && Cfg::AimBot::targetLock) pTargetPlayer = nullptr;
  targetLock = Cfg::AimBot::enable && Cfg::AimBot::targetLock;

  if (!targetLock || !IsValidPtr(pTargetPlayer) || !IsValidPtr(pTargetPlayer->GetSoldierEntity()) || !pTargetPlayer->GetSoldierEntity()->IsAlive()) {
	pTargetPlayer = nullptr;
	targetID = -1;
  }

  float count = 0.0f;

  float aimingPercentMax = 0;
  ClientPlayer* pPlayerAimingAtYou = nullptr;

  for (int i = 0; i < 70; i++) {

	ClientPlayer* pPlayer = pPlayerMgr->GetPlayerById(i);
	if (!IsValidPtr(pPlayer)) continue;

	if (pPlayer == pLocal) continue;

	bool isInTeam = pPlayer->m_TeamId == pLocal->m_TeamId;
	if (!Cfg::ESP::team && isInTeam) continue;

	/*if (Cfg::ESP::Radar::enable)
	  RenderRadar(pLocal, pPlayer, i);*/

	if (Cfg::ESP::spectators && pPlayer->m_IsSpectator) {
	  Renderer::DrawString({ 20, 600 + count }, 0, ImColor::Purple(), xorstr_("%s is spectating"),
		pPlayer->m_Name);
	  count += 15.0f;
	}

	auto* pSoldier = pPlayer->GetSoldierEntity();
	if (!IsValidPtr(pSoldier) || !pSoldier->IsAlive()) continue;

	ImColor color = isInTeam ? Cfg::ESP::teamColor : PreUpdate::preUpdatePlayersData.visiblePlayers[i] ?
	  Cfg::ESP::enemyColorVisible : Cfg::ESP::enemyColor;

	//Dont aim at passengers (velocity is always 0 here idk why)
	if (pPlayer->m_EntryId == ClientPlayer::EntrySeatType::Passenger) continue;

	if (!pPlayer->InVehicle()) {
	  BoundingBox3D playerBB3D;
	  const auto& playerBB = GetEntityAABB(pSoldier, &playerBB3D);

	  if (playerBB != BoundingBox::Zero()) {
		if (Cfg::ESP::use3DplayerBox) {
		  std::array<ImVec2, 8> playerPoints2D;
		  bool valid = true;
		  for (int i = 0; i < 8; i++) {
			WorldToScreen(playerBB3D.points[i], playerPoints2D[i]);
			if (playerPoints2D[i].x == 0.f || playerPoints2D[i].y == 0.f) {
			  valid = false;
			  break;
			}
		  }
		  if (valid) Renderer::DrawBox(playerPoints2D, color);
		}
		else RenderPlayerCorneredRect(playerBB, color);

		if (Cfg::ESP::lines && (Cfg::ESP::alliesLines == isInTeam || !isInTeam))
		  Renderer::DrawLine(
			{ G::screenSize.x * 0.5f, G::screenSize.y },
			{ playerBB.left + (playerBB.GetSize().x * 0.5f), playerBB.bot },
			Cfg::ESP::linesColor);
	  }
	}

	std::array<ImVec2, 8> vehiclePoints2D;
	BoundingBox3D vehicleBB3D;
	if (Cfg::ESP::vehicles && pPlayer->InVehicle()) {
	  auto* pVehicle = pPlayer->GetVehicleEntity();
	  if (pVehicle) {
		const auto& vehicleBB = GetEntityAABB(pVehicle, &vehicleBB3D);
		ImColor color = Cfg::ESP::vehicleAirColor;

		switch (pVehicle->m_pData->GetVehicleType()) {
		case VehicleData::VehicleType::JET:
		case VehicleData::VehicleType::HELIATTACK:
		case VehicleData::VehicleType::JETBOMBER:
		  color = ImColor(0, 179, 255, 90); break;
		case VehicleData::VehicleType::HELISCOUT:
		case VehicleData::VehicleType::HELITRANS:
		case VehicleData::VehicleType::UAV:
		  color = ImColor(212, 255, 0, 90); break;
		case VehicleData::VehicleType::BOAT:
		  color = ImColor(255, 111, 0, 90); break;
		case VehicleData::VehicleType::CAR:
		case VehicleData::VehicleType::EODBOT:
		case VehicleData::VehicleType::JEEP:
		case VehicleData::VehicleType::STATICAA:
		case VehicleData::VehicleType::STATICAT:
		case VehicleData::VehicleType::STATIONARY:
		case VehicleData::VehicleType::STATIONARYWEAPON:
		  color = ImColor(77, 255, 0, 90); break;
		case VehicleData::VehicleType::TANK:
		case VehicleData::VehicleType::TANKARTY:
		case VehicleData::VehicleType::TANKAT:
		case VehicleData::VehicleType::TANKIFV:
		case VehicleData::VehicleType::TANKLC:
		  color = Cfg::ESP::vehicleGroundColor; break;
		case VehicleData::VehicleType::TANKAA:
		  color = ImColor(255, 0, 119, 90); break;
		default:
		  color = Cfg::ESP::vehicleAirColor; break;
		}

		if (Cfg::ESP::use3DvehicleBox) {
		  bool valid = true;
		  for (int i = 0; i < 8; i++) {
			WorldToScreen(vehicleBB3D.points[i], vehiclePoints2D[i]);
			if (vehiclePoints2D[i].x == 0.f || vehiclePoints2D[i].y == 0.f) {
			  valid = false;
			  break;
			}
		  }
		  if (valid) Renderer::DrawBox(vehiclePoints2D, color);
		}
		else RenderPlayerCorneredRect(vehicleBB, color);

		if (Cfg::ESP::linesVehicles && (Cfg::ESP::alliesLines == isInTeam || !isInTeam)) {
		  ImVec2 boxCenter = { vehicleBB.GetMin().x + vehicleBB.GetCenter().x, vehicleBB.GetMax().y };
		  if (boxCenter != ZERO_VECTOR2D)
			Renderer::DrawLine(
			  { G::screenSize.x * 0.5f, G::screenSize.y },
			  boxCenter,
			  color);
		}
		else if (Cfg::ESP::vehicleIndicator && pLocal->InVehicle()) {
		  ImVec2 boxCenter = { vehicleBB.GetMin().x + vehicleBB.GetCenter().x, vehicleBB.GetMax().y };
		  if (boxCenter != ZERO_VECTOR2D)
			Renderer::DrawLine(
			  { G::screenSize.x * 0.5f, G::screenSize.y * 0.5f },
			  boxCenter,
			  targetID == i ? ImColor::White(180) : color);
		}
	  }
	}

	Vector tmpAimPoint3D = ZERO_VECTOR;
	auto pRagdoll = pSoldier->m_pRagdollComponent;

	if (!IsValidPtr(pRagdoll) || !pRagdoll->GetBone(UpdatePoseResultData::BONES::Head, tmpAimPoint3D)) {
	  if (pPlayer->InVehicle()) {
		tmpAimPoint3D = vehicleBB3D.GetCenter();
		ImVec2 o;
		if (WorldToScreen(tmpAimPoint3D, o)) {
		  std::vector<ImVec2> points =
		  {
			  ImVec2(o.x, o.y - 5),
			  ImVec2(o.x + 5, o.y),
			  ImVec2(o.x, o.y + 5),
			  ImVec2(o.x - 5, o.y),
			  ImVec2(o.x, o.y - 5),
		  };

		  Renderer::DrawLine(points, Cfg::ESP::predictionCrossColor);
		}
	  }
	}

	if (tmpAimPoint3D == ZERO_VECTOR) {
      BoundingBox3D playerBB3D;
      GetEntityAABB(pSoldier, &playerBB3D);
      tmpAimPoint3D = playerBB3D.GetCenter();

      if (tmpAimPoint3D == ZERO_VECTOR) continue;
	}

	//if (!isInTeam) RenderPlayerNames(pPlayer); //TODO;

	Vector2D aimPoint2D;
	WorldToScreen(tmpAimPoint3D, aimPoint2D);
	const auto delta = Misc::GetAbsDeltaAtGivenPoints(G::screenCenter, aimPoint2D);
	// TODO: Add better target selection
	if (targetID == i && targetLock && IsValidPtr(pTargetPlayer)) {
	  //If we already have a target, just keep it
      aimPoint3D = tmpAimPoint3D;
    }
	else if (delta < longestDelta && !isInTeam) {
	  if (targetLock && !IsValidPtr(pTargetPlayer)) {
		longestDelta = delta;
		aimPoint3D = tmpAimPoint3D;
		targetID = i;
	  }
	  else if (!targetLock && (PreUpdate::preUpdatePlayersData.visiblePlayers[i] || pPlayer->InVehicle())) {
		longestDelta = delta;
		aimPoint3D = tmpAimPoint3D;
		targetID = i;
	  }
	}

	float aimingPercent = 0;
	if (pPlayer->IsAimingAtYou(pLocal, aimingPercent) && PreUpdate::preUpdatePlayersData.visiblePlayers[i]) {
	  if (aimingPercent > aimingPercentMax) {
		aimingPercentMax = aimingPercent;
		pPlayerAimingAtYou = pPlayer;
	  }
	}
  }

  if (aimingPercentMax > 50 && IsValidPtr(pPlayerAimingAtYou)) {
	Renderer::DrawString(
	  { G::screenSize.x / 2, G::screenSize.y / 2 - 100 }, 0, ImColor(230, 30, 30),
	  xorstr_("%s is aiming at you %.0f%%"), pPlayerAimingAtYou->m_Name, aimingPercentMax
	);
	Vector vPos; pPlayerAimingAtYou->GetBone(UpdatePoseResultData::BONES::Head, vPos);
	Vector2D vPos2D; WorldToScreen(vPos, vPos2D);
	Renderer::DrawLine(G::screenCenter, vPos2D, ImColor(230, 30, 30, 130));
  }

  RenderBombImpact(aimPoint3D, &PreUpdate::weaponData);

  if (targetID != -1) pTargetPlayer = pPlayerMgr->GetPlayerById(targetID);

  if (!IsValidPtr(pTargetPlayer)
	|| !IsValidPtr(pLocal)
	|| !IsValidPtr(pTargetPlayer->GetSoldierEntity())
	|| !pTargetPlayer->GetSoldierEntity()->IsAlive()) {
	pTargetPlayer = nullptr;
	return;
  }

  auto& preUpd = PreUpdate::preUpdatePlayersData;

  preUpd.pBestTarget = pTargetPlayer;
  preUpd.pMyMissile = pMyMissile;

  if (PreUpdate::predictionData.hitPos == ZERO_VECTOR || !PreUpdate::isPredicted) return;

  RenderAimPoint(PreUpdate::predictionData, pTargetPlayer);

  //Draws target's predicted AABB when flying in jet
  if (!PreUpdate::isValid) return;

  std::array<ImVec2, 8> pts;
  bool isValid = true;
  for (int i = 0; i < pts.size(); i++) {
	if (!WorldToScreen(PreUpdate::points[i], pts[i])) {
	  isValid = false;
	  break;
	}
  }

  if (isValid) Renderer::DrawBox(pts, ImColor::Green(90));
}
