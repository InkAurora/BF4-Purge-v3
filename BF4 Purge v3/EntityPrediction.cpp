#include "EntityPrediction.h"

#define PI_2     1.57079632679489661923

float Prediction::ComputeMissileFinalVelocity(float initSpd, float maxSpd, float accel, float engIgnTime, float dist, float* travelTime) {
  //Its easier when you draw that in paint :D
  if ((accel == 0.0f) || (maxSpd < initSpd)) return 0.0f;

  auto beforeIgnDist = initSpd * engIgnTime;
  auto accelerationTime = (maxSpd - initSpd) / accel;
  auto accelerationDist = (initSpd * accelerationTime) + (accel * accelerationTime * accelerationTime) / 2.f;
  auto postAccelerationDist = dist - beforeIgnDist - accelerationDist;
  auto postAccelerationTime = postAccelerationDist / maxSpd;
  auto finalAirTime = engIgnTime + accelerationTime + postAccelerationTime;
  auto finalDistance = beforeIgnDist + accelerationDist + postAccelerationDist;

  if (finalAirTime <= 0.0f) return 0.0f;

  if (travelTime) *travelTime = finalAirTime;

  auto finalVelocity = finalDistance / finalAirTime;
  finalVelocity = std::clamp(finalVelocity, initSpd, maxSpd);

  //Just for debugging...

  /*Renderer::DrawString({ 90, G::scrnCenter.y + 15 }, 0, ImColor::Green(),
	  "beforeIgnDist = %f", beforeIgnDist);
  Renderer::DrawString({ 90, G::scrnCenter.y + 30 }, 0, ImColor::Green(),
	  "accelerationTime = %f", accelerationTime);
  Renderer::DrawString({ 90, G::scrnCenter.y + 45 }, 0, ImColor::Green(),
	  "accelerationDist = %f", accelerationDist);
  Renderer::DrawString({ 90, G::scrnCenter.y + 60 }, 0, ImColor::Green(),
	  "postAccelerationDist = %f", postAccelerationDist);
  Renderer::DrawString({ 90, G::scrnCenter.y + 75 }, 0, ImColor::Green(),
	  "postAccelerationTime = %f", postAccelerationTime);
  Renderer::DrawString({ 90, G::scrnCenter.y + 90 }, 0, ImColor::Green(),
	  "finalTime = %f", finalAirTime);
  Renderer::DrawString({ 90, G::scrnCenter.y + 105 }, 0, ImColor::Green(),
	  "finalVelocity = %f", finalVelocity);*/

  return finalVelocity;
}

bool Prediction::ComputePredictedPointInSpace(const Vector& src, const Vector& dst, const Vector& dstVel, const float bulletVel, const float bulletGravity, PredictionData_s* out, const float overrideTravelTime, const float zeroying, const AngularPredictionData_s* angularDataIn) {
  Vector relativePos; D3DXVec3Subtract(&relativePos, &dst, &src);

  auto pLocal = PlayerManager::GetInstance()->GetLocalPlayer();
  auto pLSoldier = pLocal->GetSoldierEntity();
  auto srcVel = *pLSoldier->GetVelocity();

  Vector effectiveVel; D3DXVec3Subtract(&effectiveVel, &dstVel, &srcVel);
  Vector scaledVel;

  if (angularDataIn && angularDataIn->valid)
	Cfg::DBG::_internalUseCurve = D3DXVec3LengthSq(&angularDataIn->angularVelocity) != 0.0f;

  if (bulletGravity != 0.0f) {
	Vector gravity = { 0, bulletGravity, 0 };
	double a1 = 0.25f * D3DXVec3LengthSq(&gravity);
	double b1 = -1.0f * D3DXVec3Dot(&gravity, &effectiveVel);
	double c1 = -1.0f * D3DXVec3Dot(&gravity, &relativePos) + D3DXVec3LengthSq(&effectiveVel) - (bulletVel * bulletVel);
	double d1 = 2.0f * D3DXVec3Dot(&effectiveVel, &relativePos);
	double e1 = D3DXVec3LengthSq(&relativePos);

	auto roots = solve_quartic(b1 / a1, c1 / a1, d1 / a1, e1 / a1);
	double t = 0.0f;
	for (int i = 0; i < 4; ++i) {
	  if (roots[i].real() > 0.0f && (roots[i].real() < t || t == 0.0f))
		t = roots[i].real();
	}

	if (overrideTravelTime > 0.0f) t = overrideTravelTime;

	if (t <= 0.0f) return false;

	out->travelTime = t;

	Vector targetVel = dstVel;
	if (Cfg::DBG::_internalUseCurve && Cfg::ESP::_internalCurveIterationCount > 0 && angularDataIn && angularDataIn->valid)
	  PredictFinalRotation(
		dstVel, angularDataIn->angularVelocity, out->travelTime,
		angularDataIn->orientation, dst, &Cfg::DBG::_internalPredictedOrientation,
		&targetVel);

	Vector futurePos;
	scaledVel = targetVel * t;
	D3DXVec3Add(&futurePos, &dst, &scaledVel);
	Vector velAdjustment = srcVel * t;
	Vector drop = gravity * (0.5f * t * t);
	D3DXVec3Subtract(&out->hitPos, &futurePos, &velAdjustment);
	D3DXVec3Subtract(&out->hitPos, &out->hitPos, &drop);

	out->bulletDrop = bulletGravity;
	out->bulletVel = bulletVel;
	out->distance = Misc::Distance3D(src, out->hitPos);
	out->origin = dst;

	if (zeroying != 0.0f)
	  out->hitPos.y += std::sinf(zeroying) * out->distance;

	return true;
  }

  const double a = D3DXVec3LengthSq(&effectiveVel) - (bulletVel * bulletVel);
  const double b = 2.0 * D3DXVec3Dot(&relativePos, &effectiveVel);
  const double c = D3DXVec3LengthSq(&relativePos);

  if (a == 0.0f) return false;
  double disc = b * b - (4 * a * c);

  double t = 0.f;
  if (disc > 0.f) {
	const auto t1 = (-b - sqrt(disc)) / (2 * a);
	const auto t2 = (-b + sqrt(disc)) / (2 * a);
	if (t1 > 0.f && t2 > 0.f) t = std::min<double>(t1, t2);
	else if (t1 < 0.f && t2 > 0.f) t = t2;
	else if (t1 > 0.f && t2 < 0.f) t = t1;
	else t = 0.f;
  }
  else if (disc == 0.0f) t = ((-b) / (2 * a));
  else return false;

  if (overrideTravelTime > 0.0f) t = overrideTravelTime;

  if (t <= 0.0f) return false;

  out->travelTime = t;

  Vector targetVel = dstVel;
  if (Cfg::DBG::_internalUseCurve && Cfg::ESP::_internalCurveIterationCount > 0 && angularDataIn && angularDataIn->valid)
	PredictFinalRotation(
	  dstVel, angularDataIn->angularVelocity, out->travelTime,
	  angularDataIn->orientation, dst, &Cfg::DBG::_internalPredictedOrientation,
	  &targetVel);

  scaledVel = targetVel * t;
  Vector futurePos; D3DXVec3Add(&futurePos, &dst, &scaledVel);
  Vector velAdjustment = srcVel * t;
  D3DXVec3Subtract(&out->hitPos, &futurePos, &velAdjustment);

  out->bulletDrop = bulletGravity;
  out->bulletVel = bulletVel;
  out->distance = Misc::Distance3D(src, out->hitPos);
  out->origin = dst;

  if (zeroying != 0.0f)
	out->hitPos.y += std::sinf(zeroying) * out->distance;

  return true;
}

static int framecount = 0;

bool Prediction::GetPredictedAimPoint(ClientPlayer* pLocal, ClientPlayer* pTarget, const Vector& aimPoint, PredictionData_s* dataOut, VeniceClientMissileEntity* pDataIn, WeaponData_s* pWeaponData) {
  if (!IsValidPtr(pLocal) || !IsValidPtr(pTarget)) return false;

  framecount++;

  auto pLocalSoldier = pLocal->GetSoldierEntity();
  if (!pLocalSoldier) return false;

  double bulletGravity = -1.f;
  double bulletVelocity = -1.f;
  AngularPredictionData_s angularData;
  angularData.valid = false;

  Vector targetVelocity = ZERO_VECTOR;

  if (auto pVehicle = pTarget->GetClientVehicleEntity(); IsValidPtr(pVehicle)) {
	targetVelocity = pVehicle->m_VelocityVec;

	if (IsValidPtr(pTarget->GetSoldierEntity()->m_pData) && pTarget->GetSoldierEntity()->m_pData->IsInJet()) {
	  if (IsValidPtr(pVehicle->GetChassisComponent())) {
		Matrix modelMatrix;
		pVehicle->GetTransform(&modelMatrix);
		D3DXQuaternionRotationMatrix(&angularData.orientation, &modelMatrix);
		angularData.angularVelocity = pVehicle->GetChassisComponent()->m_AngularVelocity;
		angularData.valid = true;
	  }
	}
  } else if (auto pTargetSoldier = pTarget->GetSoldierEntity(); IsValidPtr(pTargetSoldier)) {
	targetVelocity = *pTargetSoldier->GetVelocity();

	//Thats the same flag you have to set to update non-visible bones.
	*(BYTE*)((uintptr_t)pTargetSoldier + 0x1A) = 159;
  }

  if (IsValidPtr(pWeaponData)) {
	bulletGravity = pWeaponData->gravity;
	bulletVelocity = pWeaponData->speed.z;
  }

  auto weaponType = WeaponClass::None;
  auto pWeaponFiring = WeaponFiring::GetInstance();
  if (IsValidPtr(pWeaponFiring))
	weaponType = pWeaponFiring->GetWeaponClass();

  auto startPosition = G::viewPos;

  Matrix shootSpace; pLocal->GetWeaponShootSpace(shootSpace);
  startPosition = (Vector)&shootSpace._41;

  float overrideRocketTravelTime = -1.0f;
  auto pFiring = pLocalSoldier->GetFiringData();
  if (IsValidPtr(pFiring) && IsValidPtr(pFiring->m_ShotConfigData.m_ProjectileData)) {
	if (pFiring->m_ShotConfigData.m_ProjectileData->m_HitReactionWeaponType
	  == ProjectileEntityData::AntHitReactionWeaponType_Explosion) { //Launcher check
	  auto pMissileData = reinterpret_cast<MissileEntityData*>(pFiring->m_ShotConfigData.m_ProjectileData);
	  if (IsValidPtr(pMissileData) && !pLocal->InVehicle() && weaponType == WeaponClass::At) {
		const auto isTOW = pMissileData->IsTOW();
		const auto& ignTime = pMissileData->m_EngineTimeToIgnition;
		const auto& initSpd = pFiring->m_ShotConfigData.m_Speed.z;
		const auto& accel = pMissileData->m_EngineStrength;
		const auto& maxSpd = pMissileData->m_MaxSpeed;
		startPosition = (!isTOW) ? G::viewPos : (pDataIn != nullptr) ? pDataIn->m_Position : G::viewPos;
		auto dst = Misc::Distance3D(startPosition, aimPoint);

		//Calculating final velocity first time to solve 't' at given 'dst' to target
		bulletVelocity = ComputeMissileFinalVelocity(initSpd, maxSpd, accel, ignTime, dst, &overrideRocketTravelTime);

		//Calculate where target will be after 't' seconds
		const auto posAfterTime = (aimPoint - G::viewPos) + (targetVelocity * overrideRocketTravelTime);
		dst = Misc::Distance3D(posAfterTime, aimPoint);

		//Calculate final velocity again for new target position 
		bulletVelocity = ComputeMissileFinalVelocity(initSpd, maxSpd, accel, ignTime, dst);
		bulletGravity = pMissileData->m_Gravity > 0.f ? 0.0f : pMissileData->m_Gravity;
	  }

	  if (pLocal->InVehicle() && IsValidPtr(pWeaponData)) {
		bulletGravity = pWeaponData->gravity;
		bulletVelocity = pWeaponData->speed.z;

		static bool refresh = false;
		static int prevWeaponID = pWeaponData->gunID;

		//Firing TOW from APC
		if (IsValidPtr(pDataIn) && IsValidPtr(pDataIn->m_pMissileEntityData)
		  && pDataIn->m_pMissileEntityData->IsTOW()) {
		  if (!refresh) { refresh = true; prevWeaponID = pWeaponData->gunID; }
		  if (prevWeaponID == pWeaponData->gunID) {
			//FIX that later
			if (IsValidPtr(pMissileData)) {
			  const auto& ignTime = pMissileData->m_EngineTimeToIgnition;
			  const auto& initSpd = pFiring->m_ShotConfigData.m_Speed.z;
			  const auto& accel = pMissileData->m_EngineStrength;
			  const auto& maxSpd = pMissileData->m_MaxSpeed;
			  startPosition = pDataIn->m_Position;
			  auto dst = Misc::Distance3D(G::viewPos, startPosition);

			  bulletVelocity = ComputeMissileFinalVelocity(initSpd, maxSpd, accel, ignTime, dst);

			  //DICE does some fucked up things with TOW velocity...
			  bulletVelocity -= D3DXVec3Length(&targetVelocity) * 0.5f;
			  bulletGravity = 0.0;
			}
		  }
		}
		//So We are able to aimbotting when changing weapon from TOW to another 
		//when missile is still on the air.
		else refresh = false;
	  }
	}
  }

  auto pLocalVehicle = pLocal->GetClientVehicleEntity();
  if (IsValidPtr(pLocalVehicle) && pLocalVehicle->IsTVGuidedMissile()) {
	bulletGravity = 0.0f;
	bulletVelocity = D3DXVec3Length(&pLocalVehicle->m_VelocityVec);
  }

  if (bulletGravity < -500.f || bulletGravity > 500.f) bulletGravity = 0.f;

  //Zeroing fix
  float zeroEntry = 0.0f;
  if (auto pWeaponComp = pLocalSoldier->m_pWeaponComponent; IsValidPtr(pWeaponComp) && !pLocal->InVehicle()) {
	if (auto pWeapon = pWeaponComp->GetActiveSoldierWeapon(); IsValidPtr(pWeapon)) {
	  if (auto pClientWeapon = pWeapon->m_pWeapon; IsValidPtr(pClientWeapon) && IsValidPtr(pClientWeapon->m_pWeaponModifier)) {
		auto ZeroEntry = WeaponZeroingEntry(-1.0f, -1.0f);

		auto* pZeroing = pClientWeapon->m_pWeaponModifier->m_ZeroingModifier;
		if (IsValidPtr(pZeroing)) {
		  int m_ZeroLevelIndex = pWeaponComp->m_ZeroingDistanceLevel;
		  ZeroEntry = pZeroing->GetZeroLevelAt(m_ZeroLevelIndex);
		}

		if (ZeroEntry.m_ZeroDistance != -1.f
		  && ZeroEntry.m_ZeroDistance != pZeroing->m_DefaultZeroingDistance) {
		  float ZeroAirTime = ZeroEntry.m_ZeroDistance / bulletVelocity;
		  float ZeroDrop = 0.5f * bulletGravity * ZeroAirTime * ZeroAirTime;
		  zeroEntry = atan2(ZeroDrop, ZeroEntry.m_ZeroDistance);
		}

		int zeroAdjust = -1;
		float d = dataOut->distance;

		if (d > 150) zeroAdjust = 0;
		if (d > 250) zeroAdjust = 1;
		if (d > 350) zeroAdjust = 2;
		if (d > 450) zeroAdjust = 3;
		if (d > 750) zeroAdjust = 4;

		if (pWeaponComp->m_ZeroingDistanceLevel != zeroAdjust && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
		  if (IsValidPtr(pFiring) && IsValidPtr(pFiring->m_ShotConfigData.m_ProjectileData)) {
			if (pFiring->m_ShotConfigData.m_ProjectileData->m_HitReactionWeaponType
			  == ProjectileEntityData::AntHitReactionWeaponType_SniperRifle) {
			  CHAR* lparam = new char('V');
			  static int z0 = -1;
			  if (z0 == pWeaponComp->m_ZeroingDistanceLevel || framecount > (G::inputFPS / 2)) {
				framecount = 0;
				CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Misc::Send, lparam, 0, 0);
				z0 = pWeaponComp->m_ZeroingDistanceLevel;
				if (z0 == 4) z0 = -1;
				else z0++;
			  }
			}
		  }
		}

		//Cfg::DBG::testString = to_string(pWeaponComp->m_ZeroingDistanceLevel) + " " + to_string(lround(ZeroEntry.m_ZeroDistance));
	  }
	}
  }

  PredictionData_s data;
  if (ComputePredictedPointInSpace(startPosition, aimPoint, targetVelocity, bulletVelocity, bulletGravity, &data, overrideRocketTravelTime, zeroEntry, &angularData)) {
	if (IsValidPtr(dataOut)) *dataOut = data;
	return true;
  }

  return false;
}

void Prediction::PredictFinalRotation(const Vector& linearVel, const Vector& angularVel, const double predTime, const Quaternion& orientation, const Vector& curPosition, Quaternion* predOrientationOut, Vector* predLinearVelOut) {
  auto QuaternionTranslation = [](const Quaternion& quat, const Vector& vec) -> Vector {
	float num = quat.x * 2.f;
	float num2 = quat.y * 2.f;
	float num3 = quat.z * 2.f;
	float num4 = quat.x * num;
	float num5 = quat.y * num2;
	float num6 = quat.z * num3;
	float num7 = quat.x * num2;
	float num8 = quat.x * num3;
	float num9 = quat.y * num3;
	float num10 = quat.w * num;
	float num11 = quat.w * num2;
	float num12 = quat.w * num3;
	Vector result;
	result.x = (1.f - (num5 + num6)) * vec.x + (num7 - num12) * vec.y + (num8 + num11) * vec.z;
	result.y = (num7 + num12) * vec.x + (1.f - (num4 + num6)) * vec.y + (num9 - num10) * vec.z;
	result.z = (num8 - num11) * vec.x + (num9 + num10) * vec.y + (1.f - (num4 + num5)) * vec.z;
	return result;
  };

  Quaternion predOrientation = orientation;
  Vector predLinVel = linearVel;
  Vector predDisplacement = curPosition;
  float deltaTime = (predTime * Cfg::ESP::_internalCurvePredTimeMultiplier / 100.0f) / Cfg::ESP::_internalCurveIterationCount;
  for (int i = 0; i < Cfg::ESP::_internalCurveIterationCount; ++i) {
	PredictLinearMove(predLinVel, deltaTime, predDisplacement, &predDisplacement);
	PredictRotation(angularVel, predOrientation, deltaTime, &predOrientation);

	predLinVel = QuaternionTranslation(predOrientation, Vector(0.0f, 0.0f, D3DXVec3Length(&predLinVel)));
	Cfg::DBG::_internalPredictionCurve[i] = predDisplacement;
  }
  *predLinearVelOut = predLinVel;
  *predOrientationOut = predOrientation;
}

void Prediction::PredictLinearMove(const Vector& linearVelocity, const double predictionTime, const Vector& curPosition, Vector* out) {
  *out = curPosition + (linearVelocity * predictionTime);
}

void Prediction::PredictRotation(const Vector& angularVelocity, const Quaternion& orientation, const double predictionTime, Quaternion* out) {
  const auto rotationAngle = D3DXVec3Length(&angularVelocity) * predictionTime;
  Vector rotationAxis; D3DXVec3Normalize(&rotationAxis, &angularVelocity);

  Quaternion rotationFromAngularVelocity;
  D3DXQuaternionRotationAxis(&rotationFromAngularVelocity, &rotationAxis, rotationAngle);

  *out = orientation * rotationFromAngularVelocity;
}
