#include "VMTHooking.h"
#include "Globals.h"
#include "EntityPrediction.h"
#include "InputActions.h"
#include "Logger.h"
#include "MiscFeatures.h"
#include "SoldierInput.h"
#include "VehicleInput.h"
#include <cmath>
#include "xorstr.hpp"

int __fastcall HooksManager::PreFrameUpdate(void* pThis, void* EDX, float deltaTime) {
  static auto oPreFrameUpdate = HooksManager::Get()->pPreFrameHook->GetOriginal<PreFrameUpdate_t>(Index::PRE_FRAME_UPDATE);
  auto result = oPreFrameUpdate(pThis, EDX, deltaTime);

  auto pCtx = ClientGameContext::GetInstance();
  auto pLevel = (IsValidPtr(pCtx) ? pCtx->m_pLevel : nullptr);
  auto pGameWorld = (IsValidPtr(pLevel) ? pLevel->m_pGameWorld : nullptr);
  static Level* s_lastLevel = nullptr;
  static bool s_vehicleLookWasActive = false;

  auto resetVehicleLookIfNeeded = [&](const char* reason) {
	if (!s_vehicleLookWasActive) return;

	GetVehicleInputBackend().ResetTurretLook();
	s_vehicleLookWasActive = false;

#ifdef _DEBUG
	LOG_INFO("preframe", "vehicle look reset on route transition (%s)", reason);
#endif
  };

#ifdef _DEBUG
  static bool s_gameWorldReady = false;
  const bool gameWorldReady = IsValidPtr(pGameWorld);
  if (gameWorldReady != s_gameWorldReady) {
    s_gameWorldReady = gameWorldReady;
    if (gameWorldReady) {
      LOG_INFO("preframe", "game world is available for level=%p", pLevel);
    } else {
      LOG_WARN("preframe", "game world became unavailable, ending the current match context");
    }
  }
#endif

  if (!IsValidPtr(pGameWorld)) {
    resetVehicleLookIfNeeded("game world unavailable");
    if (s_lastLevel != nullptr) G::matchEnded = true;
    s_lastLevel = nullptr;
    return result;
  }

  if (s_lastLevel && s_lastLevel != pLevel) {
    G::matchEnded = true;
#ifdef _DEBUG
    LOG_INFO("preframe", "level transition detected: %p -> %p", s_lastLevel, pLevel);
#endif
  } else {
    G::matchEnded = false;
  }

#ifdef _DEBUG
  if (s_lastLevel == nullptr && pLevel != nullptr) {
    LOG_INFO("preframe", "entered level=%p", pLevel);
  }
#endif
  s_lastLevel = pLevel;

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

#ifdef _DEBUG
  static bool s_dxRendererReady = true;
  const bool dxRendererReady = IsValidPtr(pDxRenderer);
  if (dxRendererReady != s_dxRendererReady) {
    s_dxRendererReady = dxRendererReady;
    if (dxRendererReady) {
      LOG_INFO("preframe", "DxRenderer recovered");
    } else {
      LOG_WARN("preframe", "DxRenderer is unavailable, skipping frame processing");
    }
  }
#endif
  if (!IsValidPtr(pDxRenderer)) return result;

  auto pScreen = pDxRenderer->m_pScreen;

#ifdef _DEBUG
  static bool s_screenReady = true;
  const bool screenReady = IsValidPtr(pScreen);
  if (screenReady != s_screenReady) {
    s_screenReady = screenReady;
    if (screenReady) {
      LOG_INFO("preframe", "screen information recovered");
    } else {
      LOG_WARN("preframe", "DxRenderer screen is unavailable");
    }
  }
#endif
  if (!IsValidPtr(pScreen)) return result;

  G::screenSize = { float(pScreen->m_Width), float(pScreen->m_Height) };
  G::screenCenter = { G::screenSize.x * .5f, G::screenSize.y * .5f };
  G::viewPos2D = G::screenCenter;

  auto pGameRenderer = GameRenderer::GetInstance();

#ifdef _DEBUG
  static bool s_renderViewReady = true;
  const bool renderViewReady = IsValidPtr(pGameRenderer) && IsValidPtr(pGameRenderer->m_pRenderView);
  if (renderViewReady != s_renderViewReady) {
    s_renderViewReady = renderViewReady;
    if (renderViewReady) {
      LOG_INFO("preframe", "render view recovered");
    } else {
      LOG_WARN("preframe", "render view is unavailable, skipping aim calculations");
    }
  }
#endif
  if (!IsValidPtr(pGameRenderer) || !IsValidPtr(pGameRenderer->m_pRenderView)) return result;

  auto pManager = PlayerManager::GetInstance();

#ifdef _DEBUG
  static bool s_playerManagerReady = true;
  const bool playerManagerReady = IsValidPtr(pManager);
  if (playerManagerReady != s_playerManagerReady) {
    s_playerManagerReady = playerManagerReady;
    if (playerManagerReady) {
      LOG_INFO("preframe", "PlayerManager recovered");
    } else {
      LOG_WARN("preframe", "PlayerManager is unavailable");
    }
  }
#endif
  if (!IsValidPtr(pManager)) return result;

  auto pLocal = pManager->GetLocalPlayer();

#ifdef _DEBUG
  static bool s_localPlayerReady = false;
  const bool localPlayerReady =
    IsValidPtr(pLocal) &&
    IsValidPtr(pLocal->GetSoldierEntity()) &&
    pLocal->GetSoldierEntity()->IsAlive();
  if (localPlayerReady != s_localPlayerReady) {
    s_localPlayerReady = localPlayerReady;
    if (localPlayerReady) {
      LOG_INFO("preframe", "local player is ready and alive");
    } else {
      LOG_WARN("preframe", "local player or soldier entity is unavailable or dead");
    }
  }
#endif
  if (!IsValidPtr(pLocal) || !IsValidPtr(pLocal->GetSoldierEntity()) || !pLocal->GetSoldierEntity()->IsAlive()) {
    resetVehicleLookIfNeeded("local player unavailable");
    return result;
  }
  
  auto pVehicleTurret = VehicleTurret::GetInstance();
  if (IsValidPtr(pVehicleTurret) && pLocal->InVehicle()) {
    G::viewPos = pVehicleTurret->GetVehicleCameraOrigin();
    Visuals::WorldToScreen(pVehicleTurret->GetVehicleCrosshair(), G::viewPos2D);
  } else G::viewPos = (Vector)&pGameRenderer->m_pRenderView->m_ViewInverse._41;

  static bool homeWasDown = false;
  const bool homeDown = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
  if (homeDown && !homeWasDown) {
    if (PreUpdate::debugAimpointOverrideEnabled) {
      PreUpdate::debugAimpointOverrideEnabled = false;
      PreUpdate::debugAimpointOverridePos = ZERO_VECTOR;
      LOG_INFO("preframe", "debug aimpoint override disabled");
    } else {
      auto pRayCaster = Main::GetInstance()->GetRayCaster();
      if (IsValidPtr(pRayCaster)) {
        Vector forward = {
          pGameRenderer->m_pRenderView->m_ViewInverse._31,
          pGameRenderer->m_pRenderView->m_ViewInverse._32,
          pGameRenderer->m_pRenderView->m_ViewInverse._33
        };
        D3DXVec3Normalize(&forward, &forward);

        Vector rayEnd = G::viewPos - (forward * 10000.0f);
        __declspec(align(16)) Vector from = G::viewPos;
        __declspec(align(16)) Vector to = rayEnd;
        RayCastHit hit;

		if (pRayCaster->PhysicsRayQuery(xorstr_("ControllableFinder"), &from, &to, &hit, 0x4 | 0x10 | 0x20 | 0x80, NULL)) {
          const float distance = Misc::Distance3D(G::viewPos, hit.m_position);
          if (std::isfinite(distance) && distance > 1.0f && distance < 10000.0f) {
            PreUpdate::debugAimpointOverrideEnabled = true;
            PreUpdate::debugAimpointOverridePos = hit.m_position;
            LOG_INFO(
              "preframe",
              "debug aimpoint override enabled at (%.2f, %.2f, %.2f)",
              hit.m_position.x,
              hit.m_position.y,
              hit.m_position.z);
          }
          else {
            LOG_WARN("preframe", "debug aimpoint override ignored because the hit distance was invalid");
          }
        } else {
          LOG_WARN("preframe", "debug aimpoint override requested but no controllable was hit");
        }
      }
    }
  }
  homeWasDown = homeDown;

  pLocal->GetCurrentWeaponData(&PreUpdate::weaponData);
  PreUpdate::isPredicted = false;
  PreUpdate::predictionData = {};

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
  Vector debugTargetVelocity = ZERO_VECTOR;
  auto& d = PreUpdate::preUpdatePlayersData;

  const Vector* overrideTargetVelocity = nullptr;
  if (PreUpdate::debugAimpointOverrideEnabled) {
    aimPoint = PreUpdate::debugAimpointOverridePos;
    overrideTargetVelocity = &debugTargetVelocity;
  } else {
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
  }

  if (aimPoint == ZERO_VECTOR) {
    if (!pLocal->InVehicle()) {
      GetSoldierInputBackend().ReleaseDirectAim("no aim point");
    }

    resetVehicleLookIfNeeded("no aim point");
    return result;
  }

  PreUpdate::isPredicted = Prediction::GetPredictedAimPoint(
    pLocal, d.pBestTarget, aimPoint, &PreUpdate::predictionData, d.pMyMissile, &PreUpdate::weaponData, overrideTargetVelocity);

  const Vector inputAimPoint =
  (PreUpdate::isPredicted && PreUpdate::predictionData.hitPos != ZERO_VECTOR)
  ? PreUpdate::predictionData.hitPos
  : aimPoint;

#ifdef _DEBUG
  static bool s_predictionStateInitialized = false;
  static bool s_lastPredictionState = false;
  if (!s_predictionStateInitialized || s_lastPredictionState != PreUpdate::isPredicted) {
    s_predictionStateInitialized = true;
    s_lastPredictionState = PreUpdate::isPredicted;
    if (PreUpdate::isPredicted) {
      LOG_INFO("preframe", "prediction solved successfully");
    } else {
      LOG_WARN("preframe", "prediction did not produce a valid intercept");
    }
  }
#endif

  InputActions::Get()->HandleInput(
    inputAimPoint, pLocal, PreUpdate::weaponData, aimPoint, d.pMyMissile);

  if (pLocal->InVehicle()) {
    s_vehicleLookWasActive = true;
    ApplyPendingVehicleInputNodeOverlay(pThis);
    LogVehicleInputNodeDiagnostics(pThis, static_cast<int>(pLocal->m_EntryId));
  } else {
    resetVehicleLookIfNeeded("on-foot route");
  }

  F::pFeatures->MinimapSpot(Cfg::Misc::minimapSpot);
  F::pFeatures->Recoil(Cfg::Misc::disableRecoil);
  F::pFeatures->Spread(Cfg::Misc::disableSpread);
  F::pFeatures->UnlockAll(Cfg::Misc::unlockAll);

  return result;
}
