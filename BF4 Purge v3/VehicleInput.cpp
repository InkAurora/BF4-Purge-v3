#include "VehicleInput.h"

#include "Cfg.h"
#include "Logger.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace {
constexpr size_t INPUT_NODE_ACCUM_X0_OFFSET = 0x110;
constexpr size_t INPUT_NODE_ACCUM_Y0_OFFSET = 0x114;
constexpr size_t INPUT_NODE_ACCUM_X1_OFFSET = 0x118;
constexpr size_t INPUT_NODE_ACCUM_Y1_OFFSET = 0x11C;
constexpr float INPUT_NODE_OVERLAY_LIMIT = 1.25f;
constexpr float ACTION_MAP_DEADZONE_PIXELS = 6.0f;
constexpr float ACTION_MAP_RESPONSE_GAIN = 0.12f;
constexpr float INPUT_NODE_ACTION_LOG_THRESHOLD = 0.05f;
constexpr unsigned int INPUT_NODE_DIAGNOSTIC_INTERVAL = 5u;

struct PendingTurretActionMap {
  bool active = false;
  float x = 0.0f;
  float y = 0.0f;
  VehicleTurretInputMode mode = VehicleTurretInputMode::MouseDevice;
};

struct RecentTurretCommand {
  bool active = false;
  float x = 0.0f;
  float y = 0.0f;
  VehicleTurretInputMode mode = VehicleTurretInputMode::MouseDevice;
};

PendingTurretActionMap g_pendingTurretActionMap;
RecentTurretCommand g_recentTurretCommand;
bool g_actionMapHookInstalled = false;

#ifdef _DEBUG
bool g_loggedActionMapArmed = false;
bool g_loggedActionMapDisarmed = false;
bool g_loggedActionMapOverlayApplied = false;
#endif

bool IsActionMapMode(VehicleTurretInputMode mode) {
  return mode == VehicleTurretInputMode::ActionMapCrosshair;
}

void RecordTurretCommand(VehicleTurretInputMode mode, float x, float y) {
  g_recentTurretCommand.active = true;
  g_recentTurretCommand.mode = mode;
  g_recentTurretCommand.x = x;
  g_recentTurretCommand.y = y;
}

void ClearTurretCommand() {
  g_recentTurretCommand = {};
}

bool WriteActionMapPair(float* actionValues, size_t actionCount, int xConcept, int yConcept, float x, float y) {
  if (!actionValues) return false;
  if (xConcept < 0 || yConcept < 0) return false;

  const auto xIndex = static_cast<size_t>(xConcept);
  const auto yIndex = static_cast<size_t>(yConcept);
  if (xIndex >= actionCount || yIndex >= actionCount) return false;

  actionValues[xIndex] = x;
  actionValues[yIndex] = y;
  return true;
}

float ClampOverlayValue(float value) {
  if (value > INPUT_NODE_OVERLAY_LIMIT) return INPUT_NODE_OVERLAY_LIMIT;
  if (value < -INPUT_NODE_OVERLAY_LIMIT) return -INPUT_NODE_OVERLAY_LIMIT;
  return value;
}

float ComputeHorizontalAccumulatorCommand(const Vector2D& deltaVec, float smooth) {
  const float rawError = -deltaVec.x;
  const float absError = std::fabs(rawError);
  if (absError <= ACTION_MAP_DEADZONE_PIXELS) return 0.0f;

  const float effectiveSmooth = (smooth > 0.0f) ? smooth : 1.0f;
  const float scaledError = ((absError - ACTION_MAP_DEADZONE_PIXELS) / effectiveSmooth) * ACTION_MAP_RESPONSE_GAIN;
  const float command = ClampOverlayValue(scaledError);
  return rawError < 0.0f ? -command : command;
}

float ComputeVerticalMouseCommand(const Vector2D& deltaVec, float smooth) {
  if (smooth == 0.0f) return 0.0f;
  return -deltaVec.y / smooth;
}

void WriteAccumulatorPair(uintptr_t inputNodeState, float x, float y) {
  *reinterpret_cast<float*>(inputNodeState + INPUT_NODE_ACCUM_X1_OFFSET) = x;
  *reinterpret_cast<float*>(inputNodeState + INPUT_NODE_ACCUM_Y1_OFFSET) = y;
}

bool WriteMouseBuffer(float x, float y, bool writeX, bool writeY) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pMouse) || !IsValidPtr(pNode->m_pMouse->m_pDevice)) return false;

  auto* pMouse = pNode->m_pMouse->m_pDevice;
  if (writeX) pMouse->m_Buffer.x = x;
  if (writeY) pMouse->m_Buffer.y = y;
  return true;
}

void ClearAccumulatorPair() {
  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pInputNode)) return;

  WriteAccumulatorPair(reinterpret_cast<uintptr_t>(pNode->m_pInputNode), 0.0f, 0.0f);
}

#ifdef _DEBUG
void LogTrackedActionMapValues(const char* stage, uintptr_t inputNodeState) {
  static unsigned int s_logCounter = 0;
  if (!inputNodeState) return;
  if ((++s_logCounter % 180u) != 0u) return;

  const auto accumX0 = *reinterpret_cast<const float*>(inputNodeState + INPUT_NODE_ACCUM_X0_OFFSET);
  const auto accumY0 = *reinterpret_cast<const float*>(inputNodeState + INPUT_NODE_ACCUM_Y0_OFFSET);
  const auto accumX1 = *reinterpret_cast<const float*>(inputNodeState + INPUT_NODE_ACCUM_X1_OFFSET);
  const auto accumY1 = *reinterpret_cast<const float*>(inputNodeState + INPUT_NODE_ACCUM_Y1_OFFSET);

  LOG_INFO(
    "vehicle-input",
    "%s input node @ %p accum0=(%.4f, %.4f) accum1=(%.4f, %.4f)",
    stage,
    reinterpret_cast<void*>(inputNodeState),
    accumX0,
    accumY0,
    accumX1,
    accumY1);
}
#endif

bool GetInputBuffer(float*& input) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();

#ifdef _DEBUG
  static bool s_inputBufferReady = false;
  const bool inputBufferReady = IsValidPtr(pNode) && IsValidPtr(pNode->m_inputCache);
  if (inputBufferReady != s_inputBufferReady) {
    s_inputBufferReady = inputBufferReady;
    if (inputBufferReady) {
      LOG_INFO("vehicle-input", "input buffer recovered");
    } else {
      LOG_WARN("vehicle-input", "input buffer is unavailable");
    }
  }
#endif
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
  case VehicleTurretInputMode::ActionMapCrosshair:
    return "Action Map Crosshair";
  default:
    return "Unknown";
  }
}

bool InitializeVehicleInputHooks() {
  if (g_actionMapHookInstalled) return true;

  g_actionMapHookInstalled = true;
  LOG_INFO("vehicle-input", "action map overlay ready on post-preframe input-node state");
  return true;
}

void ShutdownVehicleInputHooks() {
  if (!g_actionMapHookInstalled) return;

  ClearAccumulatorPair();
  g_actionMapHookInstalled = false;
  g_pendingTurretActionMap = {};
  ClearTurretCommand();
}

void ApplyPendingVehicleInputNodeOverlay(void* inputNodeState) {
  if (!g_actionMapHookInstalled || !g_pendingTurretActionMap.active || !IsValidPtr(inputNodeState)) return;

  const auto state = reinterpret_cast<uintptr_t>(inputNodeState);
  const float overlayX = ClampOverlayValue(g_pendingTurretActionMap.x);
  const float overlayY = g_pendingTurretActionMap.y;

  switch (g_pendingTurretActionMap.mode) {
  case VehicleTurretInputMode::ActionMapCrosshair:
    WriteAccumulatorPair(state, overlayX, 0.0f);
    WriteMouseBuffer(0.0f, overlayY, false, true);
#ifdef _DEBUG
    if (!g_loggedActionMapOverlayApplied) {
      g_loggedActionMapOverlayApplied = true;
      LOG_INFO(
        "vehicle-input",
        "applied post-preframe overlay state=%p accumX=%.4f mouseY=%.4f",
        inputNodeState,
        overlayX,
        overlayY);
    }
    LogTrackedActionMapValues("post-preframe overlay", state);
#endif
    break;
  default:
    break;
  }
}

void LogVehicleInputNodeDiagnostics(void* inputNodeState, int seatId) {
#ifdef _DEBUG
  if (!IsValidPtr(inputNodeState) || !g_recentTurretCommand.active) return;

  static unsigned int s_diagnosticCounter = 0;
  if ((++s_diagnosticCounter % INPUT_NODE_DIAGNOSTIC_INTERVAL) != 0u) return;

  const auto state = reinterpret_cast<uintptr_t>(inputNodeState);
  const auto* actionValues = reinterpret_cast<const float*>(state + 0x24);
  const auto accumX0 = *reinterpret_cast<const float*>(state + INPUT_NODE_ACCUM_X0_OFFSET);
  const auto accumY0 = *reinterpret_cast<const float*>(state + INPUT_NODE_ACCUM_Y0_OFFSET);
  const auto accumX1 = *reinterpret_cast<const float*>(state + INPUT_NODE_ACCUM_X1_OFFSET);
  const auto accumY1 = *reinterpret_cast<const float*>(state + INPUT_NODE_ACCUM_Y1_OFFSET);
  BorderInputNode* borderInputNode = BorderInputNode::GetInstance();

  long mouseBufferX = 0;
  long mouseBufferY = 0;
  long mouseCurrentX = 0;
  long mouseCurrentY = 0;
  float conceptYaw = 0.0f;
  float conceptPitch = 0.0f;
  float conceptCrosshairX = 0.0f;
  float conceptCrosshairY = 0.0f;
  float conceptCameraYaw = 0.0f;
  float conceptCameraPitch = 0.0f;
  float conceptRightStickX = 0.0f;
  float conceptRightStickY = 0.0f;

  if (IsValidPtr(borderInputNode)) {
    if (IsValidPtr(borderInputNode->m_pMouse) && IsValidPtr(borderInputNode->m_pMouse->m_pDevice)) {
      const auto* mouseDevice = borderInputNode->m_pMouse->m_pDevice;
      mouseBufferX = mouseDevice->m_Buffer.x;
      mouseBufferY = mouseDevice->m_Buffer.y;
      mouseCurrentX = mouseDevice->m_Current.x;
      mouseCurrentY = mouseDevice->m_Current.y;
    }

    if (IsValidPtr(borderInputNode->m_inputCache)) {
      const auto* inputBuffer = borderInputNode->m_inputCache->flInputBuffer;
      conceptYaw = inputBuffer[ConceptYaw];
      conceptPitch = inputBuffer[ConceptPitch];
      conceptCrosshairX = inputBuffer[ConceptCrosshairMoveX];
      conceptCrosshairY = inputBuffer[ConceptCrosshairMoveY];
      conceptCameraYaw = inputBuffer[ConceptCameraYaw];
      conceptCameraPitch = inputBuffer[ConceptCameraPitch];
      conceptRightStickX = inputBuffer[ConceptRightStickX];
      conceptRightStickY = inputBuffer[ConceptRightStickY];
    }
  }

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(3);
  stream << "seat=" << seatId
         << " mode=" << GetVehicleTurretInputModeName(g_recentTurretCommand.mode)
         << " cmd=(" << g_recentTurretCommand.x << ", " << g_recentTurretCommand.y << ")"
         << " mouseBuffer=(" << mouseBufferX << ", " << mouseBufferY << ")"
         << " mouseCurrent=(" << mouseCurrentX << ", " << mouseCurrentY << ")"
         << " concepts={yaw:" << conceptYaw
         << ", pitch:" << conceptPitch
         << ", crossX:" << conceptCrosshairX
         << ", crossY:" << conceptCrosshairY
         << ", camYaw:" << conceptCameraYaw
         << ", camPitch:" << conceptCameraPitch
         << ", rsX:" << conceptRightStickX
         << ", rsY:" << conceptRightStickY << '}'
         << " accum0=(" << accumX0 << ", " << accumY0 << ")"
         << " accum1=(" << accumX1 << ", " << accumY1 << ")"
         << " activeActions=[";

  int emitted = 0;
  for (int index = 0; index < 60; ++index) {
    const float value = actionValues[index];
    if (std::fabs(value) < INPUT_NODE_ACTION_LOG_THRESHOLD) continue;

    if (emitted > 0) stream << ", ";
    stream << index << '=' << value;
    ++emitted;
    if (emitted >= 20) break;
  }
  stream << ']';

  LOG_INFO("vehicle-input", "%s", stream.str().c_str());
#else
  (void)inputNodeState;
  (void)seatId;
#endif
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
  const auto mode = static_cast<VehicleTurretInputMode>(Cfg::AimBot::vehicleTurretInputMode);

#ifdef _DEBUG
  static int s_lastTurretMode = -1;
  if (s_lastTurretMode != static_cast<int>(mode)) {
    s_lastTurretMode = static_cast<int>(mode);
    LOG_INFO("vehicle-input", "turret input mode set to %s", GetVehicleTurretInputModeName(mode));
  }
#endif

  switch (mode) {
  case VehicleTurretInputMode::CrosshairConcepts:
  case VehicleTurretInputMode::CameraConcepts:
  case VehicleTurretInputMode::RightStickConcepts:
    return ApplyTurretLookConcepts(deltaVec, smooth, mode);
  case VehicleTurretInputMode::ActionMapCrosshair:
    return ApplyTurretLookActionMap(deltaVec, smooth, mode);
  case VehicleTurretInputMode::MouseDevice:
  default:
    return ApplyTurretLookMouse(deltaVec, smooth);
  }
}

bool HybridVehicleInputBackend::ApplyTurretLookMouse(const Vector2D& deltaVec, float smooth) {
  BorderInputNode* pNode = BorderInputNode::GetInstance();

#ifdef _DEBUG
  static bool s_mouseDeviceReady = false;
  const bool mouseDeviceReady = IsValidPtr(pNode) && IsValidPtr(pNode->m_pMouse) && IsValidPtr(pNode->m_pMouse->m_pDevice);
  if (mouseDeviceReady != s_mouseDeviceReady) {
    s_mouseDeviceReady = mouseDeviceReady;
    if (mouseDeviceReady) {
      LOG_INFO("vehicle-input", "mouse device backend recovered");
    } else {
      LOG_WARN("vehicle-input", "mouse device backend is unavailable");
    }
  }
#endif
  if (!IsValidPtr(pNode) || !IsValidPtr(pNode->m_pMouse) || !IsValidPtr(pNode->m_pMouse->m_pDevice)) return false;
  if (smooth == 0.0f) return false;

  auto* pMouse = pNode->m_pMouse->m_pDevice;
  const float x = -deltaVec.x / smooth;
  const float y = -deltaVec.y / smooth;
  pMouse->m_Buffer.x = x;
  pMouse->m_Buffer.y = y;
  RecordTurretCommand(VehicleTurretInputMode::MouseDevice, x, y);
  return true;
}

bool HybridVehicleInputBackend::ApplyTurretLookConcepts(const Vector2D& deltaVec, float smooth, VehicleTurretInputMode mode) {
  if (smooth == 0.0f) return false;

  const float x = -deltaVec.x / smooth;
  const float y = -deltaVec.y / smooth;

  switch (mode) {
  case VehicleTurretInputMode::CrosshairConcepts:
    RecordTurretCommand(mode, x, y);
    return WriteConceptPair(ConceptCrosshairMoveX, ConceptCrosshairMoveY, x, y);
  case VehicleTurretInputMode::CameraConcepts:
    RecordTurretCommand(mode, x, y);
    return WriteConceptPair(ConceptCameraYaw, ConceptCameraPitch, x, y);
  case VehicleTurretInputMode::RightStickConcepts:
    RecordTurretCommand(mode, x, y);
    return WriteConceptPair(ConceptRightStickX, ConceptRightStickY, x, y);
  case VehicleTurretInputMode::MouseDevice:
  default:
    return false;
  }
}

bool HybridVehicleInputBackend::ApplyTurretLookActionMap(const Vector2D& deltaVec, float smooth, VehicleTurretInputMode mode) {
  if (smooth == 0.0f || !IsActionMapMode(mode) || !g_actionMapHookInstalled) return false;

  g_pendingTurretActionMap.active = true;
  g_pendingTurretActionMap.mode = mode;
  g_pendingTurretActionMap.x = ComputeHorizontalAccumulatorCommand(deltaVec, smooth);
  g_pendingTurretActionMap.y = ComputeVerticalMouseCommand(deltaVec, smooth);
  RecordTurretCommand(mode, g_pendingTurretActionMap.x, g_pendingTurretActionMap.y);

#ifdef _DEBUG
  if (!g_loggedActionMapArmed) {
    g_loggedActionMapArmed = true;
    g_loggedActionMapDisarmed = false;
    g_loggedActionMapOverlayApplied = false;
    LOG_INFO(
      "vehicle-input",
      "armed action-map overlay x=%.4f y=%.4f smooth=%.4f",
      g_pendingTurretActionMap.x,
      g_pendingTurretActionMap.y,
      smooth);
  }
#endif
  return true;
}

bool HybridVehicleInputBackend::ResetTurretLook() {
  bool resetAny = ResetTurretLookConcepts();
  resetAny = ResetTurretLookActionMap() || resetAny;
  return ResetTurretLookMouse() || resetAny;
}

bool HybridVehicleInputBackend::ResetTurretLookMouse() {
  return WriteMouseBuffer(0.0f, 0.0f, true, true);
}

bool HybridVehicleInputBackend::ResetTurretLookConcepts() {
  float* input = nullptr;
  if (!GetInputBuffer(input)) return false;

  ClearTurretConcepts(input);
  return true;
}

bool HybridVehicleInputBackend::ResetTurretLookActionMap() {
  const bool wasActive = g_pendingTurretActionMap.active;
  g_pendingTurretActionMap = {};
  ClearAccumulatorPair();
  ClearTurretCommand();

#ifdef _DEBUG
  if (wasActive && !g_loggedActionMapDisarmed) {
    g_loggedActionMapDisarmed = true;
    g_loggedActionMapArmed = false;
    g_loggedActionMapOverlayApplied = false;
    LOG_INFO("vehicle-input", "disarmed action-map overlay");
  }
#endif
  return wasActive;
}

IVehicleInputBackend& GetVehicleInputBackend() {
  static HybridVehicleInputBackend backend;
  return backend;
}
