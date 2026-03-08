#include "SoldierInput.h"

#include "Logger.h"

namespace {
HybridSoldierInputBackend g_soldierInputBackend;
}

bool HybridSoldierInputBackend::ResolveAimContext(SoldierAimContext& context) {
  context = {};

  auto* pManager = PlayerManager::GetInstance();
  if (!IsValidPtr(pManager)) return false;

  auto* pLocal = pManager->GetLocalPlayer();
  if (!IsValidPtr(pLocal)) return false;

  auto* pLocalSoldier = pLocal->GetSoldierEntity();
  if (!IsValidPtr(pLocalSoldier)) return false;

  auto* pWeaponComponent = pLocalSoldier->m_pWeaponComponent;
  if (!IsValidPtr(pWeaponComponent)) return false;

  context.weapon = pWeaponComponent->GetActiveSoldierWeapon();
  if (!IsValidPtr(context.weapon)) return false;

  context.aimSimulation = context.weapon->m_pAuthoritativeAiming;
  if (!IsValidPtr(context.aimSimulation)) return false;

  context.aimAssist = context.aimSimulation->m_pFPSAimer;
  if (!IsValidPtr(context.aimAssist)) return false;

  return true;
}

bool HybridSoldierInputBackend::ApplyDirectAim(const SoldierAimContext& context, const Vector2D& angles) {
  if (!IsValidPtr(context.weapon) || !IsValidPtr(context.aimSimulation) || !IsValidPtr(context.aimAssist)) {
    return false;
  }

  context.aimAssist->m_AimAngles = angles;

#ifdef _DEBUG
  if (!m_directAimActive || m_lastWeapon != context.weapon || m_lastAimAssist != context.aimAssist) {
    LOG_INFO(
      "soldier-input",
      "direct soldier aim acquired weapon=%p aimer=%p",
      context.weapon,
      context.aimAssist);
  }
#endif

  m_directAimActive = true;
  m_lastWeapon = context.weapon;
  m_lastAimAssist = context.aimAssist;
  return true;
}

void HybridSoldierInputBackend::ReleaseDirectAim(const char* reason) {
  if (!m_directAimActive && !IsValidPtr(m_lastWeapon) && !IsValidPtr(m_lastAimAssist)) return;

#ifdef _DEBUG
  if (m_directAimActive) {
    if (reason && reason[0] != '\0') {
      LOG_INFO("soldier-input", "direct soldier aim released (%s)", reason);
    } else {
      LOG_INFO("soldier-input", "direct soldier aim released");
    }
  }
#endif

  m_directAimActive = false;
  m_lastWeapon = nullptr;
  m_lastAimAssist = nullptr;
}

ISoldierInputBackend& GetSoldierInputBackend() {
  return g_soldierInputBackend;
}