#include "PreFrameUpdate.h"

int Hook::PreFrameUpdate() {
  if (auto pInput = BorderInputNode::GetInstance(); IsValidPtr(pInput) && IsValidPtr(pInput->m_pMouse)) {
	if (IsValidPtr(pInput->m_pMouse->m_pDevice) && !pInput->m_pMouse->m_pDevice->m_CursorMode) {
	  //FIX: Load last saved angles to prevent flip after closing menu
	  pInput->m_pMouse->m_pDevice->m_UIOwnsInput = G::isMenuVisible;
	  pInput->m_pMouse->m_pDevice->m_UseRawMouseInput = G::isMenuVisible;
	}
  }

  auto pDxRenderer = DxRenderer::GetInstance();
  if (!IsValidPtr(pDxRenderer)) return -1;
  auto pScreen = pDxRenderer->m_pScreen;
  if (!IsValidPtr(pScreen)) return -1;

  G::screenSize = { float(pScreen->m_Width), float(pScreen->m_Height) };
  G::screenCenter = { G::screenSize.x * .5f, G::screenSize.y * .5f };
  G::viewPos2D = G::screenCenter;

  auto pGameRenderer = GameRenderer::GetInstance();
  if (!IsValidPtr(pGameRenderer) || !IsValidPtr(pGameRenderer->m_pRenderView)) return -1;
  G::viewPos = (Vector)&pGameRenderer->m_pRenderView->m_ViewInverse._41;

  auto pManager = PlayerManager::GetInstance();
  if (!IsValidPtr(pManager)) return -1;
  auto pLocal = pManager->GetLocalPlayer();
  if (!IsValidPtr(pLocal) || !IsValidPtr(pLocal->GetSoldierEntity()) || !pLocal->GetSoldierEntity()->IsAlive())
	return 1;

  pLocal->GetCurrentWeaponData(&PreUpdate::weaponData);

  Vector aimPoint = ZERO_VECTOR;
  auto& d = PreUpdate::preUpdatePlayersData;
  if (!IsValidPtr(d.pBestTarget)) return 1;

  if (auto pTargetSoldier = d.pBestTarget->GetSoldierEntity(); IsValidPtr(pTargetSoldier)) {
	if (!pTargetSoldier->IsAlive()) return 1;
	if (!IsValidPtr(pTargetSoldier->m_pRagdollComponent) || !pTargetSoldier->m_pRagdollComponent->GetBone(UpdatePoseResultData::Head, aimPoint)) {
	  if (auto pVehicle = d.pBestTarget->GetVehicleEntity(); d.pBestTarget->InVehicle() && IsValidPtr(pVehicle)) {
		BoundingBox3D aabb3D;
		Visuals::GetEntityAABB(pVehicle, &aabb3D);
		aimPoint = aabb3D.GetCenter();
	  }
	}
  }

  if (aimPoint == ZERO_VECTOR) return 1;

  PreUpdate::isPredicted = Prediction::GetPredictedAimPoint(
	pLocal, d.pBestTarget, aimPoint, &PreUpdate::predictionData, d.pMyMissile, &PreUpdate::weaponData);

  InputActions::Get()->HandleInput(
	PreUpdate::predictionData.hitPos, pLocal, PreUpdate::weaponData, aimPoint, d.pMyMissile);

  Features::Recoil(Cfg::DBG::disableRecoil);
  Features::Spread(Cfg::DBG::disableSpread);

  return 0;
}