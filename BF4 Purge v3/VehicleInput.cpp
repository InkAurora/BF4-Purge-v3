#include "VehicleInput.h"

bool HybridVehicleInputBackend::ApplyAxes(const VehicleAxesInput& axes) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_inputCache)) return false;

  auto* input = pNode->m_inputCache->flInputBuffer;
  if (axes.writeYaw) input[ConceptYaw] = axes.yaw;
  if (axes.writePitch) input[ConceptPitch] = axes.pitch;
  if (axes.writeRoll) input[ConceptRoll] = axes.roll;
  return true;
}

bool HybridVehicleInputBackend::ApplyTurretLook(const Vector2D& deltaVec, float smooth) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pMouse) || !IsValidPtr(pNode->m_pMouse->m_pDevice)) return false;
  if (smooth == 0.0f) return false;

  auto* pMouse = pNode->m_pMouse->m_pDevice;
  pMouse->m_Buffer.x = -deltaVec.x / smooth;
  pMouse->m_Buffer.y = -deltaVec.y / smooth;
  return true;
}

bool HybridVehicleInputBackend::ResetTurretLook() {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pMouse) || !IsValidPtr(pNode->m_pMouse->m_pDevice)) return false;

  auto* pMouse = pNode->m_pMouse->m_pDevice;
  pMouse->m_Buffer.x = 0.0f;
  pMouse->m_Buffer.y = 0.0f;
  return true;
}

IVehicleInputBackend& GetVehicleInputBackend() {
  static HybridVehicleInputBackend backend;
  return backend;
}
