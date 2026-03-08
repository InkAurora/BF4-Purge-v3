#include "InputActions.h"
#include "EntityPrediction.h"
#include "VehicleInput.h"

void InputActions::HandleInput(const Vector& pos, ClientPlayer* pLocal, const WeaponData_s& pVehData, const Vector& targetPos, VeniceClientMissileEntity* pMissile) {
  if (!Cfg::AimBot::enable) return;

  BorderInputNode* pNode = BorderInputNode::GetInstance();
  if (!IsValidPtr(pNode) || !IsValidPtr(pLocal)) return;

  if (!IsValidPtr(pNode->m_inputCache)) return;

  auto& input = pNode->m_inputCache->flInputBuffer;

  Vector2D pos2D; Visuals::WorldToScreen(pos, pos2D);
  Vector2D target2D = { 0.0f, 0.0f };
  if (targetPos != ZERO_VECTOR) Visuals::WorldToScreen(targetPos, target2D);

  auto delta = Misc::GetAbsDeltaAtGivenPoints(G::viewPos2D, pos2D);
  auto pVeh = pLocal->GetClientVehicleEntity();

  Vector2D deltaVec = G::viewPos2D - pos2D;
  PreUpdate::isValid = this->isAutoPiloting;

  if (IsValidPtr(pMissile) && IsValidPtr(pMissile->m_pMissileEntityData) && pMissile->m_pMissileEntityData->IsTOW()) {
	PreUpdate::isTOWLocked = true;
	TOWMissileControl(pLocal, pVeh, targetPos, pMissile);
	return;
  }

  PreUpdate::isTOWLocked = false;

  if (IsValidPtr(pVeh) && IsValidPtr(pVeh->m_pData) && (int)pLocal->m_EntryId < 4) {
	if (Cfg::Misc::noOverheat) OverheatControll();

	if (pVeh->m_pData->IsAirVehicle()) {
	  JetSpeedControll(pVeh, input);

	  if (pLocal->m_EntryId != ClientPlayer::EntrySeatType::Gunner)
		JetWeaponsControll(pVeh, pos, delta, input);

	  if (pVeh->m_pData->IsInJet()) {
		JetBombControll(pVeh, targetPos, pVehData, input);
		TriggerControll(targetPos, delta, input);
	  }
	}

	VehicleTurretControll(deltaVec, delta, pMissile);
	TVMissileControll(pVeh, pos, deltaVec, delta, input);
  }
  else if (!pLocal->InVehicle()) {
	SoldierWeaponControll(delta, pos);
  }
}

//Found that somewhere on the internet
float InputActions::ReRange(float x, float inmin, float inmax, float outmin, float outmax) {
  return (x - inmin) * (outmax - outmin) / (inmax - inmin) + outmin;
}

void InputActions::CountermeasuresControll() {
  //TODO:
}

void InputActions::JetSpeedControll(ClientVehicleEntity* pVeh, float* input) {
  if (!IsValidPtr(pVeh) || !pVeh->m_pData->IsInJet() || !IsValidPtr(input)) return;

  if (Cfg::DBG::jetSpeedCtrl
	&& (BorderInputNode::GetInstance()->m_pKeyboard->m_pDevice->m_Current[BYTE(Keyboard::InputKeys::IDK_Space)]
	  || this->isAutoPiloting)) {
	auto velocity = D3DXVec3Length(&pVeh->m_VelocityVec) * 3.6f;

	if (velocity < 314.f && velocity > 312.f) {
	  input[ConceptMoveForward] = 1.0f;
	  input[ConceptMoveFB] = 1.0f;
	  input[ConceptFreeCameraMoveFB] = 1.0f;
	  input[ConceptFreeCameraTurboSpeed] = 0.0f;
	  input[ConceptSprint] = 0.0f;
	}
	else if (velocity >= 316.f) {
	  input[ConceptMoveBackward] = 1.0f;
	  input[ConceptMoveFB] = -1.0f;
	  input[ConceptFreeCameraMoveFB] = -1.0f;
	  input[ConceptFreeCameraTurboSpeed] = 0.0f;
	  input[ConceptSprint] = 0.0f;
	  input[ConceptBrake] = 1.0f;
	  input[ConceptCrawl] = 1.0f;
	  //BorderInputNode::GetInstance()->m_pKeyboard->m_pDevice->buffer[BYTE(Keyboard::InputKeys::IDK_LeftShift)] = false;
	}
	else if (velocity <= 312.f) {
	  input[ConceptMoveFB] = 1.0f;
	  input[ConceptFreeCameraMoveFB] = 1.0f;
	  input[ConceptFreeCameraTurboSpeed] = 1.0f;
	  input[ConceptSprint] = 1.0f;
	}
  }
}

void InputActions::JetBombControll(ClientVehicleEntity* pVeh, const Vector& targetPos, const WeaponData_s& pVehData, float* input) {
  this->isJDAMBomb = false;

  if (!IsValidPtr(pVeh) || !pVeh->m_pData->IsInJet() || !IsValidPtr(input) || !pVehData.isValid) return;

  if (!Cfg::AimBot::autoBombs) return;

  if (pVehData.isValid && pVehData.gravity == 0.0f && pVehData.speed.z == 0.0f) {
	auto pBomb = reinterpret_cast<ClientIndirectFireWeapon*>(pVehData.pWeapon);
	if (IsValidPtr(pBomb)) {
	  this->isJDAMBomb = true;
	  this->bombImpactPos = Vector(pBomb->landingpos.x, pBomb->landingpos.y, pBomb->landingpos.z);
	  auto dist = Misc::Distance3D(targetPos, this->bombImpactPos);
	  bool inRange = (dist <= 5.0f); //JDAM bombs has efficient dmg radius equalt to 5m. Fucking joke.

	  if (inRange) input[ConceptFire] = 1.0f;
	}
	else this->isJDAMBomb = false;
  }
  else this->isJDAMBomb = false;
}

void InputActions::JetWeaponsControll(ClientVehicleEntity* pVeh, const Vector& targetPos, float delta, float* input) {
  this->isAutoPiloting = false;

  if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) return;

  if (!IsValidPtr(pVeh) || !IsValidPtr(input) || pVeh->IsTVGuidedMissile() || this->isJDAMBomb) return;

  //JDam aimbot ? Nah..

  Vector dt = targetPos - G::viewPos;
  auto dtLength = D3DXVec3Length(&dt);
  if (dtLength != 0.0f) dt /= dtLength;

  auto CalcDelta = [](const Vector& vec, const Vector& delta, const float& FOV) -> float {
	return asinf(D3DXVec3Dot(&vec, &delta)) / FOV;
  };

  auto halfFov = GameRenderer::GetInstance()->m_pRenderView->m_FovX / 2.0f;
  auto& mat = VehicleTurret::GetInstance()->m_VehicleMatrix;
  auto Left = Vector(mat._11, mat._12, mat._13);
  auto Up = Vector(mat._21, mat._22, mat._23);

  auto yaw = CalcDelta(Left, dt, halfFov);
  auto pitch = CalcDelta(Up, dt, halfFov);
  auto roll = yaw;

  float sens = ReRange(delta, 11.5, 35.f, 11.5f, 12.f);
  if (delta <= 30.0f) sens *= 1.5f;
  if (delta <= 15) sens *= 1.2f;

  VehicleAxesInput axes = {};
  axes.writeYaw = true;
  axes.writePitch = true;
  axes.writeRoll = true;
  axes.yaw = -yaw * sens;
  axes.pitch = -pitch * sens;
  axes.roll = -roll * sens * .85f;
  GetVehicleInputBackend().ApplyAxes(axes);

  this->isAutoPiloting = true;
}

void InputActions::TVMissileControll(ClientVehicleEntity* pVeh, const Vector& targetPos, const Vector2D deltaVec, float delta, float* input) {
  if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) return;

  if (!IsValidPtr(pVeh) || !IsValidPtr(input)) return;

  if (pVeh->IsTVGuidedMissile()) {
	Vector dt = targetPos - G::viewPos;
	auto dtLength = D3DXVec3Length(&dt);
	if (dtLength != 0.0f) dt /= dtLength;

	auto CalcDelta = [](const Vector& vec, const Vector& delta, const float& FOV) -> float {
	  return asinf(D3DXVec3Dot(&vec, &delta)) / FOV;
	};

	auto fov = GameRenderer::GetInstance()->m_pRenderView->m_FovX / 2.0f;
	auto& mat = VehicleTurret::GetInstance()->m_VehicleMatrix;
	auto Left = Vector(mat._11, mat._12, mat._13);
	auto Up = Vector(mat._21, mat._22, mat._23);

	auto yaw = CalcDelta(Left, dt, fov);
	auto pitch = CalcDelta(Up, dt, fov);

	float sens = ReRange(delta, .5f, 200.f, .5f, 12.f);
	if (Misc::Distance3D(G::viewPos, targetPos) <= 200.0f)
	  sens = 750.f;

	VehicleAxesInput axes = {};
	axes.writeYaw = true;
	axes.writePitch = true;
	axes.yaw = -yaw * sens;
	axes.pitch = pitch * sens;
	GetVehicleInputBackend().ApplyAxes(axes);
  }
}

void InputActions::TOWMissileControl(ClientPlayer* pLocal, ClientVehicleEntity* pVeh, const Vector& targetPos, VeniceClientMissileEntity* pMissile) {
  PreUpdate::TOWForwardVec = ZERO_VECTOR;

  if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) return;

  if (!IsValidPtr(pLocal) || !IsValidPtr(pMissile) || !IsValidPtr(pMissile->m_pMissileEntityData)) return;

  bool isInVehicle = pLocal->InVehicle();
  if (isInVehicle && !IsValidPtr(pVeh)) return;

  D3DXMATRIX missileMatrix;
  ((ClientControllableEntity*)pMissile)->GetTransform(&missileMatrix);
  auto missilePos = Vector(missileMatrix._41, missileMatrix._42, missileMatrix._43);
  auto missileForwardVec = Vector(missileMatrix._31, missileMatrix._32, missileMatrix._33);
  PreUpdate::TOWForwardVec = missileForwardVec;

  float distToTarget = Misc::Distance3D(missilePos, targetPos);

  // Track missile identity so elapsed flight time is measured correctly mid-flight
  static VeniceClientMissileEntity* s_trackedMissile = nullptr;
  static ULONGLONG s_launchMs = 0;
  if (pMissile != s_trackedMissile) {
	s_trackedMissile = pMissile;
	s_launchMs = GetTickCount64();
  }

  const float initSpd       = pMissile->m_pMissileEntityData->m_InitialSpeed;
  const float maxSpd        = pMissile->m_pMissileEntityData->m_MaxSpeed;
  const float accel         = pMissile->m_pMissileEntityData->m_EngineStrength;
  const float ignTime       = pMissile->m_pMissileEntityData->m_EngineTimeToIgnition;
  const float elapsedTime   = (GetTickCount64() - s_launchMs) * 0.001f;
  const float accelDuration = (accel > 0.0f && maxSpd > initSpd) ? (maxSpd - initSpd) / accel : 0.0f;

  // Current instantaneous speed based on which flight phase the missile is in
  float missileSpeed;
  if (elapsedTime < ignTime)
	missileSpeed = initSpd;
  else if (elapsedTime < ignTime + accelDuration)
	missileSpeed = initSpd + accel * (elapsedTime - ignTime);
  else
	missileSpeed = maxSpd;

  // Time remaining to target: integrate only the phases still ahead of the missile
  float timeToHit = 0.0f;
  if (elapsedTime >= ignTime + accelDuration) {
	// Cruise phase - constant max speed
	if (maxSpd > 0.0f) timeToHit = distToTarget / maxSpd;
  } else if (elapsedTime >= ignTime) {
	// Acceleration phase - integrate remaining burn, then cruise
	const float remAccelTime    = (ignTime + accelDuration) - elapsedTime;
	const float distDuringAccel = missileSpeed * remAccelTime + 0.5f * accel * remAccelTime * remAccelTime;
	const float distCruise      = distToTarget > distDuringAccel ? distToTarget - distDuringAccel : 0.0f;
	timeToHit = remAccelTime + (maxSpd > 0.0f ? distCruise / maxSpd : 0.0f);
  } else {
	// Ignition (coast) phase - integrate remaining coast, full burn, then cruise
	const float remIgnTime      = ignTime - elapsedTime;
	const float distDuringAccel = initSpd * accelDuration + 0.5f * accel * accelDuration * accelDuration;
	const float distCruise      = distToTarget > initSpd * remIgnTime + distDuringAccel
									? distToTarget - initSpd * remIgnTime - distDuringAccel : 0.0f;
	timeToHit = remIgnTime + accelDuration + (maxSpd > 0.0f ? distCruise / maxSpd : 0.0f);
  }

  PreUpdate::TOWTimeToHit = timeToHit;
  PreUpdate::TOWSteer = { 0.0f, 0.0f };

  if (timeToHit > 0.0f) {
	Vector effectiveTarget = targetPos;
	if (timeToHit < 1.0f && Prediction::TOWPrediction())
	  effectiveTarget = PreUpdate::predictionData.hitPos;

	Vector dt = effectiveTarget - missilePos;
	float dtLength = D3DXVec3Length(&dt);
	if (dtLength == 0.0f) return;
	dt /= dtLength;

	Vector Left = Vector(missileMatrix._11, missileMatrix._12, missileMatrix._13);
	Vector Up   = Vector(missileMatrix._21, missileMatrix._22, missileMatrix._23);

	float deltaAngle = acosf(max(-1.0f, min(1.0f, D3DXVec3Dot(&missileForwardVec, &dt))));

	// Steering components: +1 = 180° right/up, -1 = 180° left/down
	float yaw   = atan2f(-D3DXVec3Dot(&Left, &dt), D3DXVec3Dot(&missileForwardVec, &dt)) / (float)PI;
	float pitch = atan2f( D3DXVec3Dot(&Up,   &dt), D3DXVec3Dot(&missileForwardVec, &dt)) / (float)PI;

	PreUpdate::TOWSteer = { yaw, pitch };
  }

  if (PreUpdate::TOWSteer.x != 0.0000f || PreUpdate::TOWSteer.y != 0.0000f) {
	float sens = ReRange(distToTarget, 0.5f, 500.f, 0.5f, 12.f);
	if (distToTarget <= 200.0f) sens = 750.f;

	VehicleAxesInput axes = {};
	axes.writeYaw = true;
	axes.writePitch = true;
	axes.yaw = PreUpdate::TOWSteer.x * sens;
	axes.pitch = PreUpdate::TOWSteer.y * sens;
	GetVehicleInputBackend().ApplyAxes(axes);
  }
}

void InputActions::VehicleTurretControll(const Vector2D& deltaVec, float delta, VeniceClientMissileEntity* pMissile) {
  if (!(GetAsyncKeyState(VK_MENU) & 0x8000) || this->isAutoPiloting) return;

  //Simulating mouse movement. I was trying to reverse IVehicle interface without success. Maybe you can make it ?

  static bool wasAiming = false;
  bool isInRange = delta <= Cfg::AimBot::radius;
  bool isTOW = false;

  if (IsValidPtr(pMissile)
	&& IsValidPtr(pMissile->m_pMissileEntityData)
	&& pMissile->m_pMissileEntityData->IsTOW()) {
	return;
  }

  if (isInRange) {
	wasAiming = true;
	GetVehicleInputBackend().ApplyTurretLook(deltaVec, Cfg::AimBot::smoothVehicle);
  }
  else if (wasAiming) {
	wasAiming = false;
	GetVehicleInputBackend().ResetTurretLook();
  }
}

void InputActions::SoldierWeaponControll(float delta, const Vector targetPos) {
  if (!(GetAsyncKeyState(VK_MENU) & 0x8000) || (delta > Cfg::AimBot::radius)) return;

  auto* pLocal = PlayerManager::GetInstance()->GetLocalPlayer();
  if (!IsValidPtr(pLocal)) return;

  auto* pLocalSoldier = pLocal->GetSoldierEntity();
  if (!IsValidPtr(pLocalSoldier)) return;

  auto* pWepComp = pLocalSoldier->m_pWeaponComponent;
  if (!IsValidPtr(pWepComp)) return;

  auto* pWeapon = pWepComp->GetActiveSoldierWeapon();
  if (!IsValidPtr(pWeapon)) return;

  auto* pClientWeapon = pWeapon->m_pWeapon;
  if (!IsValidPtr(pClientWeapon)) return;

  auto* pAimSim = pWeapon->m_pAuthoritativeAiming;
  if (!IsValidPtr(pAimSim)) return;

  auto* pAimAssist = pAimSim->m_pFPSAimer;
  if (!IsValidPtr(pAimAssist)) return;

  //Simple aimbot with angle calculations. Awful in every aspect...

  auto angles = (Vector2D)Misc::CalcAngles2D(G::viewPos, targetPos);
  //angles.y = PreUpdate::angleY;
  angles.x -= pAimSim->m_Sway.x;
  angles.y -= pAimSim->m_Sway.y;

  auto Delta = (angles - pAimAssist->m_AimAngles);
  auto lDelta = D3DXVec2Length(&Delta);
  Misc::ClampAngles(&Delta);
  angles = (pAimAssist->m_AimAngles + Delta / (lDelta > .002f ? Cfg::AimBot::smoothSoldier : 1.f));
  Misc::ClampAngles(&angles);

  pAimAssist->m_AimAngles = angles;

  //Old code with mouse simulation.

  //pAimAssist->m_AimAngles.x = angleDiff.x / Cfg::AimBot::smoothSoldier;
  //pAimAssist->m_AimAngles.y = angleDiff.y / Cfg::AimBot::smoothSoldier;

  //pAimAssist->m_AimAngles = angles;

  //pMouse->m_Buffer.x = -deltaVec.x / Cfg::AimBot::smoothSoldier;
  //pMouse->m_Buffer.y = -deltaVec.y / Cfg::AimBot::smoothSoldier;

  /*auto pMD = BorderInputNode::GetInstance()->m_pMouse;
  if (!IsValidPtr(pMD)) return;

  auto pMouse = BorderInputNode::GetInstance()->m_pMouse->m_pDevice;
  if (!IsValidPtr(pMouse)) return;

  static bool wasAiming = false;
  if (delta <= Cfg::AimBot::radius)
  {
	  wasAiming = true;
	  pMouse->m_Buffer.x = -deltaVec.x / Cfg::AimBot::smoothSoldier;
	  pMouse->m_Buffer.y = -deltaVec.y / Cfg::AimBot::smoothSoldier;
  }
  else if (wasAiming)
  {
	  wasAiming = false;
	  pMouse->m_Buffer.x = 0.f;
	  pMouse->m_Buffer.y = 0.f;
  }*/
}

void InputActions::OverheatControll() {
  isAboutToOverheat = false;
  auto pFire = WeaponFiring::GetInstance();
  if (!IsValidPtr(pFire)) return;

  if (pFire->m_FirstSlotBullets != -1) return;

  if (!IsValidPtr(BorderInputNode::GetInstance()->m_pMouse)
	|| !IsValidPtr(BorderInputNode::GetInstance()->m_pMouse->m_pDevice)) return;

  auto pM = BorderInputNode::GetInstance()->m_pMouse->m_pDevice;

  //You can play around that value but I've found it as the best.
  if (pFire->m_Overheat >= 0.90f) {
	isAboutToOverheat = true;
	pM->m_Buffer.buttons[0] = false;
  }
  else if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
	BorderInputNode::GetInstance()->m_inputCache->flInputBuffer[ConceptFire] = 1.f;
  }
}

void InputActions::TriggerControll(const Vector& targetPos, float delta, float* input) {
  if (!(GetAsyncKeyState(VK_MENU) & 0x8000) || !IsValidPtr(input) || this->isJDAMBomb) return;

  if (isAboutToOverheat) return;

  if (this->isAutoPiloting) { //Only when autopiloting jet
	auto pRayCaster = Main::GetInstance()->GetRayCaster();
	if (!IsValidPtr(pRayCaster)) return;

	auto pTarget = PreUpdate::preUpdatePlayersData.pBestTarget;

	if (!IsValidPtr(pTarget)) return;

	if (pTarget->InVehicle() && IsValidPtr(pTarget->GetClientVehicleEntity())) {
	  D3DXMATRIX matrix;
	  TransformAABBStruct transAABB;
	  D3DXMatrixRotationQuaternion(&matrix, &Cfg::DBG::_internalPredictedOrientation);

	  if (pTarget->InVehicle() && IsValidPtr(pTarget->GetClientVehicleEntity()))
		pTarget->GetClientVehicleEntity()->GetAABB(&transAABB);
	  else if (IsValidPtr(pTarget->GetSoldierEntity()))
		pTarget->GetSoldierEntity()->GetAABB(&transAABB);
	  else return;

	  auto m = (Vector)ViewAnglesClass::GetInstance()->m_VehicleMatrix.m[2];
	  Vector forward; D3DXVec3Normalize(&forward, &m);

	  Vector min = { transAABB.AABB.m_Min.x, transAABB.AABB.m_Min.y, transAABB.AABB.m_Min.z };
	  Vector max = { transAABB.AABB.m_Max.x, transAABB.AABB.m_Max.y, transAABB.AABB.m_Max.z };

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

	  auto MultiplyMat = [](const Vector& vec, D3DXMATRIX* mat) -> Vector {
		return Vector(
		  mat->_11 * vec.x + mat->_21 * vec.y + mat->_31 * vec.z,
		  mat->_12 * vec.x + mat->_22 * vec.y + mat->_32 * vec.z,
		  mat->_13 * vec.x + mat->_23 * vec.y + mat->_33 * vec.z);
	  };

	  Vector pts[8] = {};
	  std::array<ImVec2, 8> pts2D = {};
	  for (int i = 0; i < 8; i++) {
		PreUpdate::points[i] = PreUpdate::predictionData.hitPos + MultiplyMat(points[i], &matrix);
		Visuals::WorldToScreen(pts[i], pts2D[i]);
	  }

	  auto ray = G::viewPos + (forward * 9999999.f);
	  float rayLenght = -1.0f;
	  Vector dir; D3DXVec3Normalize(&dir, &ray);

	  IPhysicsRayCaster::Ray Ray(G::viewPos, dir);
	  IPhysicsRayCaster::AxisAlignedBoundingBox AABB(min, max);

	  D3DXMatrixTransformation(&matrix, NULL, NULL, NULL, NULL, NULL, &PreUpdate::predictionData.hitPos);

	  if (IPhysicsRayCaster::IsIntersectingOBB(Ray, AABB, matrix, &rayLenght))
		input[ConceptFire] = 1.0f;
	}
  }
  else if (delta <= Cfg::AimBot::radius)
	input[ConceptFire] = 1.0f;
}
