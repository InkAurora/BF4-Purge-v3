#include "VehicleInput.h"

#include "Cfg.h"

namespace {
bool GetInputBuffer(float*& input) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_inputCache)) return false;

  input = pNode->m_inputCache->flInputBuffer;
  return true;
}

bool WriteConceptPair(int xConcept, int yConcept, float x, float y) {
  float* input = nullptr;
  if (!GetInputBuffer(input)) return false;

  input[xConcept] = x;
  input[yConcept] = y;
  return true;
}

void ClearTurretConcepts(float* input) {
  input[ConceptCrosshairMoveX] = 0.0f;
  input[ConceptCrosshairMoveY] = 0.0f;
  input[ConceptCameraYaw] = 0.0f;
  input[ConceptCameraPitch] = 0.0f;
  input[ConceptRightStickX] = 0.0f;
  input[ConceptRightStickY] = 0.0f;
}
}

const char* GetVehicleTurretInputModeName(VehicleTurretInputMode mode) {
  switch (mode) {
  case VehicleTurretInputMode::MouseDevice:
    return "Mouse Device";
  case VehicleTurretInputMode::CrosshairConcepts:
    return "Crosshair Concepts";
  case VehicleTurretInputMode::CameraConcepts:
    return "Camera Concepts";
  case VehicleTurretInputMode::RightStickConcepts:
    return "Right Stick Concepts";
  default:
    return "Unknown";
  }
}

bool HybridVehicleInputBackend::ApplyAxes(const VehicleAxesInput& axes) {
  float* input = nullptr;
  if (!GetInputBuffer(input)) return false;

  if (axes.writeYaw) input[ConceptYaw] = axes.yaw;
  if (axes.writePitch) input[ConceptPitch] = axes.pitch;
  if (axes.writeRoll) input[ConceptRoll] = axes.roll;
  return true;
}

bool HybridVehicleInputBackend::ApplyTurretLook(const Vector2D& deltaVec, float smooth) {
  switch (static_cast<VehicleTurretInputMode>(Cfg::AimBot::vehicleTurretInputMode)) {
  case VehicleTurretInputMode::CrosshairConcepts:
  case VehicleTurretInputMode::CameraConcepts:
  case VehicleTurretInputMode::RightStickConcepts:
    return ApplyTurretLookConcepts(deltaVec, smooth, static_cast<VehicleTurretInputMode>(Cfg::AimBot::vehicleTurretInputMode));
  case VehicleTurretInputMode::MouseDevice:
  default:
    return ApplyTurretLookMouse(deltaVec, smooth);
  }
}

bool HybridVehicleInputBackend::ApplyTurretLookMouse(const Vector2D& deltaVec, float smooth) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pMouse) || !IsValidPtr(pNode->m_pMouse->m_pDevice)) return false;
  if (smooth == 0.0f) return false;

  auto* pMouse = pNode->m_pMouse->m_pDevice;
  pMouse->m_Buffer.x = -deltaVec.x / smooth;
  pMouse->m_Buffer.y = -deltaVec.y / smooth;
  return true;
}

bool HybridVehicleInputBackend::ApplyTurretLookConcepts(const Vector2D& deltaVec, float smooth, VehicleTurretInputMode mode) {
  if (smooth == 0.0f) return false;

  const float x = -deltaVec.x / smooth;
  const float y = -deltaVec.y / smooth;

  switch (mode) {
  case VehicleTurretInputMode::CrosshairConcepts:
    return WriteConceptPair(ConceptCrosshairMoveX, ConceptCrosshairMoveY, x, y);
  case VehicleTurretInputMode::CameraConcepts:
    return WriteConceptPair(ConceptCameraYaw, ConceptCameraPitch, x, y);
  case VehicleTurretInputMode::RightStickConcepts:
    return WriteConceptPair(ConceptRightStickX, ConceptRightStickY, x, y);
  case VehicleTurretInputMode::MouseDevice:
  default:
    return false;
  }
}

bool HybridVehicleInputBackend::ResetTurretLook() {
  bool resetAny = ResetTurretLookConcepts();
  return ResetTurretLookMouse() || resetAny;
}

bool HybridVehicleInputBackend::ResetTurretLookMouse() {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pMouse) || !IsValidPtr(pNode->m_pMouse->m_pDevice)) return false;

  auto* pMouse = pNode->m_pMouse->m_pDevice;
  pMouse->m_Buffer.x = 0.0f;
  pMouse->m_Buffer.y = 0.0f;
  return true;
}

bool HybridVehicleInputBackend::ResetTurretLookConcepts() {
  float* input = nullptr;
  if (!GetInputBuffer(input)) return false;

  ClearTurretConcepts(input);
  return true;
}

IVehicleInputBackend& GetVehicleInputBackend() {
  static HybridVehicleInputBackend backend;
  return backend;
}
