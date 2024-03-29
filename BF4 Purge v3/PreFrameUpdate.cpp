#include "VMTHooking.h"
#include "Globals.h"
#include "EntityPrediction.h"
#include "InputActions.h"
#include "MiscFeatures.h"

int __fastcall HooksManager::PreFrameUpdate(void* pThis, void* EDX, float deltaTime) {
  static auto oPreFrameUpdate = HooksManager::Get()->pPreFrameHook->GetOriginal<PreFrameUpdate_t>(Index::PRE_FRAME_UPDATE);
  auto result = oPreFrameUpdate(pThis, EDX, deltaTime);

 // if (auto pInput = BorderInputNode::GetInstance(); IsValidPtr(pInput) && IsValidPtr(pInput->m_pMouse)) {
	//if (IsValidPtr(pInput->m_pMouse->m_pDevice) && !pInput->m_pMouse->m_pDevice->m_CursorMode) {
	//  //FIX: Load last saved angles to prevent flip after closing menu
	//  pInput->m_pMouse->m_pDevice->m_UIOwnsInput = G::isMenuVisible;
	//  pInput->m_pMouse->m_pDevice->m_UseRawMouseInput = G::isMenuVisible;
	//}
 // }

  static int framecount = 0;
  G::inputFramecount++;

  static bool betweenMeasurings = false;
  static int lastFrame;
  if (!betweenMeasurings) {
	Misc::QPC(true);
	lastFrame = G::inputFramecount;
	betweenMeasurings = true;
  } else if (Misc::QPC(false) > 1000) {
	G::inputFPS = G::inputFramecount - lastFrame;
	betweenMeasurings = false;
  }

  auto pDxRenderer = DxRenderer::GetInstance();
  if (!IsValidPtr(pDxRenderer)) return result;
  auto pScreen = pDxRenderer->m_pScreen;
  if (!IsValidPtr(pScreen)) return result;

  G::screenSize = { float(pScreen->m_Width), float(pScreen->m_Height) };
  G::screenCenter = { G::screenSize.x * .5f, G::screenSize.y * .5f };
  G::viewPos2D = G::screenCenter;

  auto pGameRenderer = GameRenderer::GetInstance();
  if (!IsValidPtr(pGameRenderer) || !IsValidPtr(pGameRenderer->m_pRenderView)) return result;

  auto pManager = PlayerManager::GetInstance();
  if (!IsValidPtr(pManager)) return result;
  auto pLocal = pManager->GetLocalPlayer();
  if (!IsValidPtr(pLocal) || !IsValidPtr(pLocal->GetSoldierEntity()) || !pLocal->GetSoldierEntity()->IsAlive())
	return result;
  
  auto pVehicleTurret = VehicleTurret::GetInstance();
  if (IsValidPtr(pVehicleTurret) && pLocal->InVehicle()) {
	G::viewPos = pVehicleTurret->GetVehicleCameraOrigin();
	Visuals::WorldToScreen(pVehicleTurret->GetVehicleCrosshair(), G::viewPos2D);
  } else G::viewPos = (Vector)&pGameRenderer->m_pRenderView->m_ViewInverse._41;

  pLocal->GetCurrentWeaponData(&PreUpdate::weaponData);

  Matrix shootSpace; pLocal->GetWeaponShootSpace(shootSpace);

  for (int i = 0; i < 70; i++) {
	auto pPlayer = pManager->GetPlayerById(i);
	if (pPlayer == pLocal) continue;
	if (!IsValidPtr(pPlayer->GetSoldierEntity())) continue;
	if (!pPlayer->GetSoldierEntity()->IsAlive()) continue;
	if (pPlayer->m_TeamId == pLocal->m_TeamId) continue;

	PreUpdate::preUpdatePlayersData.visiblePlayers[i] = pPlayer->IsVisible(shootSpace, Cfg::AimBot::bone);
  }

  Vector aimPoint = ZERO_VECTOR;
  auto& d = PreUpdate::preUpdatePlayersData;
  if (!IsValidPtr(d.pBestTarget)) return result;

  if (auto pTargetSoldier = d.pBestTarget->GetSoldierEntity(); IsValidPtr(pTargetSoldier)) {
	if (!pTargetSoldier->IsAlive()) return result;
	if (!IsValidPtr(pTargetSoldier->m_pRagdollComponent) || !pTargetSoldier->m_pRagdollComponent->GetBone(Cfg::AimBot::bone, aimPoint)) {
	  if (auto pVehicle = d.pBestTarget->GetVehicleEntity(); d.pBestTarget->InVehicle() && IsValidPtr(pVehicle)) {
		BoundingBox3D aabb3D;
		Visuals::GetEntityAABB(pVehicle, &aabb3D);
		aimPoint = aabb3D.GetCenter();
	  }
	}
  }

  if (aimPoint == ZERO_VECTOR) return result;

  PreUpdate::isPredicted = Prediction::GetPredictedAimPoint(
	pLocal, d.pBestTarget, aimPoint, &PreUpdate::predictionData, d.pMyMissile, &PreUpdate::weaponData);

  InputActions::Get()->HandleInput(
	PreUpdate::predictionData.hitPos, pLocal, PreUpdate::weaponData, aimPoint, d.pMyMissile);

  F::pFeatures->MinimapSpot(Cfg::Misc::minimapSpot);
  F::pFeatures->Recoil(Cfg::Misc::disableRecoil);
  F::pFeatures->Spread(Cfg::Misc::disableSpread);
  F::pFeatures->UnlockAll(Cfg::Misc::unlockAll);

  return result;
}
