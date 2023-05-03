#include "Visuals.h"
#include "xorstr.hpp"
#include "Cfg.h"

bool targetLock = false;

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

void Visuals::RenderVisuals() {
  if (!Cfg::ESP::enable) return;

  PlayerManager* pPlayerMgr = PlayerManager::GetInstance();

  ClientPlayer* pLocal = pPlayerMgr->GetLocalPlayer();

  if (!IsValidPtr(pLocal)) return;

  static ClientPlayer* pTargetPlayer = nullptr;
  static Vector			  aimPoint3D = ZERO_VECTOR;
  static float			longestDelta = FLT_MAX;
  static int				targetID = -1;

  if (!targetLock) {
	pTargetPlayer = nullptr;
	aimPoint3D = ZERO_VECTOR;
	longestDelta = FLT_MAX;
	targetID = -1;
  }

  float count = 0.0f;

  Cfg::ESP::validPlayers = "";

  for (int i = 0; i < 70; i++) {

	ClientPlayer* pPlayer = pPlayerMgr->GetPlayerById(i);
	if (!IsValidPtr(pPlayer)) continue;

	/*if (Cfg::ESP::Radar::enable)
	  RenderRadar(pLocal, pPlayer, i);*/

	if (pPlayer == pLocal) continue;

	Cfg::ESP::validPlayers += to_string(i) + ", ";

	bool isInTeam = pPlayer->m_TeamId == pLocal->m_TeamId;
	if (!Cfg::ESP::team && isInTeam) continue;

	//if (!isInTeam)
	//	PlayerList::list;

	//TESTING SPECTATOR LIST!
	{
	  if (Cfg::ESP::spectators && pPlayer->m_IsSpectator) {
		Renderer::DrawString({ 20, 600 + count }, 0, ImColor::Purple(), xorstr_("%s is spectating"),
		  pPlayer->m_Name);
		count += 15.0f;
	  }
	}

	auto* pSoldier = pPlayer->GetSoldierEntity();
	if (!IsValidPtr(pSoldier) || !pSoldier->IsAlive()) continue;

	//Dont aim at passengers (velocity is always 0 here idk why)
	//if (pPlayer->m_EntryId == ClientPlayer::EntrySeatType::Passenger) continue;

	ImColor color = isInTeam ? Cfg::ESP::teamColor : Cfg::ESP::enemyColor;
	Vector vehicleCenter3D;
	Vector playerHead3D;

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

		if (Cfg::ESP::lines)
		  Renderer::DrawLine(
			{ 2560 * 0.5f, 1080 },
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

		//There are many hardcoded colors here which you could change to your desire
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

		if (Cfg::ESP::linesVehicles) {
		  ImVec2 boxCenter = { vehicleBB.GetMin().x + vehicleBB.GetCenter().x, vehicleBB.GetMax().y };
		  if (boxCenter != ZERO_VECTOR2D)
			Renderer::DrawLine(
			  { 2560 * 0.5f, 1080 },
			  boxCenter,
			  color);
		}
		else if (Cfg::ESP::vehicleIndicator && pLocal->InVehicle()) {
		  ImVec2 boxCenter = { vehicleBB.GetMin().x + vehicleBB.GetCenter().x, vehicleBB.GetMax().y };
		  if (boxCenter != ZERO_VECTOR2D)
			Renderer::DrawLine(
			  { 2560 * 0.5f, 1080 * 0.5f },
			  boxCenter,
			  targetID == i ? ImColor::White(180) : color);
		}
	  }
	}

  }

  return;
}
