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

  auto* pMngr = PlayerManager::GetInstance();
  auto* pLocal = pMngr->GetLocalPlayer();
  auto* pLSoldier = pLocal->GetSoldierEntity();
  Vector srcVel = *pLSoldier->GetVelocity();

  if (angularDataIn && angularDataIn->valid)
	Cfg::DBG::_internalUseCurve = D3DXVec3LengthSq(&angularDataIn->angularVelocity) != 0.0f;

  // Ballistic equation
  // https://en.wikipedia.org/wiki/Projectile_motion
  //
  // (0.25*g^2)*(t^4) + (-g*vy1)*(t^3) + (vx1^2+vy1^2+vz1^2 - g*py1 - |v|^2)*(t^2) + 2*(px1*vx1+py1*vy1+pz1*vz1)*(t) + (px1^2+py1^2+pz1^2) = 0
  // x^4 + a*x^3 + b*x^2 + c*x + d
  //
  // G = bullet gravity
  // V = target velocity vector
  // P = target position relative to shooter vector
  // S = bullet speed 
  // T = time to impact
  //
  //     a           b                c               d       e
  // 0.25(G^2)T^4 + (VG)T^3 + (PG + V^2 - S^2)T^2 + 2(PV)T + P^2 = 0    | /e
  //
  // when G == 0
  //
  // (V^2 - S^2)T^2 + 2(PV)T + P^2 = 0



  // another try
  // t^4 * 0.25*|g|^2 - t^3 * g°ES + t^2 * g°BP - g°EP - BS^2 - t * 2(ES°BP - ES°EP) + |BP|^2 + |EP|^2 - 2(BP°EP) = 0
  // (° = Dot Product, |a| = Length of Vector a)
  // t:float = time
  // g:Vector3 = Gravity
  // EP : Vector3 = Enemy Position
  // ES : Vector3 = Enemy Speed
  // BP : Vector3 = Bullet Position(Start Position)
  // BS : float = Bullet Speed

  if (bulletGravity != 0.0f) {
	Vector gravity = { 0, bulletGravity, 0 };
	double a1 = 0.25f * D3DXVec3Length(&gravity) * D3DXVec3Length(&gravity);
	double b1 = D3DXVec3Dot(&gravity, &dstVel) * -1.0f;
	double c1 = D3DXVec3Dot(&gravity, &src) - D3DXVec3Dot(&gravity, &dst) - (bulletVel * bulletVel);
	double d1 = 2 * (D3DXVec3Dot(&dstVel, &src) - D3DXVec3Dot(&dstVel, &dst)) * -1.0f;
	double e1 = (D3DXVec3Length(&src) * D3DXVec3Length(&src)) + (D3DXVec3Length(&dst) * D3DXVec3Length(&dst)) - (2 * D3DXVec3Dot(&src, &dst));

	auto roots = solve_quartic(b1 / (a1), c1 / (a1), d1 / (a1), e1 / (a1));
	float t = 0.0f;
	for (int i = 0; i < 4; ++i) {
	  if (roots[i].real() > 0.0f && (roots[i].real() < t || t == 0.0f))
		t = roots[i].real();
	}

	out->travelTime = t;

	Vector targetVel = dstVel;
	if (Cfg::DBG::_internalUseCurve && Cfg::ESP::_internalCurveIterationCount > 0 && angularDataIn && angularDataIn->valid)
	  PredictFinalRotation(
		dstVel, angularDataIn->angularVelocity, out->travelTime,
		angularDataIn->orientation, dst, &Cfg::DBG::_internalPredictedOrientation,
		&targetVel);

	Vector p = dst + (targetVel * t) - (srcVel * t); // future target position based on time and velocity

	float v = bulletVel;
	float g = bulletGravity;
	float x = Misc::Distance2D({ src.x, src.z }, { p.x, p.z });
	float h = p.y - src.y;
	float quadratic = sqrt((v * v * v * v) - (g * (((x * x) * g) + (2 * -h * (v * v)))));
	vector<double>roots2;
	roots2.push_back(-atan((v * v + quadratic) / (g * x)));
	roots2.push_back(-atan((v * v - quadratic) / (g * x)));

	//Cfg::DBG::testString = to_string(bulletVel) + " " + to_string(bulletGravity) + " " + to_string(range);
	//Cfg::DBG::testString = to_string(roots3[0].real() * (180 / PI)) + " " + to_string(roots3[1].real() * (180 / PI));

	//Cfg::DBG::testString = to_string(p.y);
	
	float theta = -100.0f;
	for (int i = 0; i < 2; i++) {
	  if (roots2[i] > (-PI_2 / 2) && roots2[i] < (PI_2 / 2) && (roots2[i] < theta || theta == -100.0f)) theta = roots2[i];
	}

	if (zeroying != 0.0f) theta += zeroying;

	Cfg::DBG::testString = to_string(zeroying);

	PreUpdate::angleY = theta;

	float correctedY = src.y + x * tan(theta);

	p.y = correctedY;
	
	//Cfg::DBG::testString += " " + to_string(p.y);

	//Cfg::DBG::testString = to_string(roots2[0].real()) + "  " + to_string(roots2[1].real());

	// predicted vector with compenstation = P + VT + 0.5*G*T^2
	out->hitPos.x = p.x;
	out->hitPos.y = p.y;
	out->hitPos.z = p.z;

	//Cfg::DBG::testString += " " + to_string(out->hitPos.y);

	out->bulletDrop = bulletGravity;
	out->bulletVel = bulletVel;
	out->distance = Misc::Distance3D(src, out->hitPos);
	out->origin = dst;

	//Fix for zeroing
	/*if (zeroying != 0.0f)
	  out->hitPos.y += std::sinf(zeroying) * out->distance;*/

	return true;
  }

 // if (bulletGravity != 0.0f) {
	//const double a = 0.25 * bulletGravity * bulletGravity;
	//const double b = dstVel.y * bulletGravity;
	//const double c = (relativePos.y * bulletGravity) + D3DXVec3Dot(&dstVel, &dstVel) - (bulletVel * bulletVel);
	//const double d = 2.0f * (D3DXVec3Dot(&relativePos, &dstVel));
	//const double e = D3DXVec3Dot(&relativePos, &relativePos);

	//out->travelTime = 0.0f;
	//if (overrideTravelTime > 0.0f) out->travelTime = overrideTravelTime;

	//if (out->travelTime == 0.0f) {
	//  const auto& roots = solve_quartic(b / (a), c / (a), d / (a), e / (a));
	//  for (int i = 0; i < 4; ++i) {
	//	if (roots[i].real() > 0.0f && (out->travelTime == 0.0f || roots[i].real() < out->travelTime))
	//	  out->travelTime = roots[i].real();
	//  }
	//}

	//if (out->travelTime <= 0.0f) return false;

	//auto targetVel = dstVel;
	//if (Cfg::DBG::_internalUseCurve && Cfg::ESP::_internalCurveIterationCount > 0 && angularDataIn && angularDataIn->valid)
	//  PredictFinalRotation(
	//	dstVel, angularDataIn->angularVelocity, out->travelTime,
	//	angularDataIn->orientation, dst, &Cfg::DBG::_internalPredictedOrientation,
	//	&targetVel);

	//// predicted bullet velocity vector at given time
	//double hitVelX = (relativePos.x / out->travelTime) + targetVel.x;
	//double hitVelY = (relativePos.y / out->travelTime) + targetVel.y - (0.5f * bulletGravity * out->travelTime);
	//double hitVelZ = (relativePos.z / out->travelTime) + targetVel.z;

	//// predicted vector with compenstation = P + VT + 0.5*G*T^2
	//out->hitPos.x = (src.x + hitVelX * out->travelTime);
	//out->hitPos.y = (src.y + hitVelY * out->travelTime);
	//out->hitPos.z = (src.z + hitVelZ * out->travelTime);

	////Cfg::DBG::testString += " " + to_string(out->hitPos.y);

	//out->bulletDrop = bulletGravity;
	//out->bulletVel = bulletVel;
	//out->distance = Misc::Distance3D(src, out->hitPos);
	//out->origin = dst;

	////Fix for zeroing
	//if (zeroying != 0.0f)
	//  out->hitPos.y += std::sinf(zeroying) * out->distance;

	//return true;
 // }

  const double a = (D3DXVec3Dot(&dstVel, &dstVel)) - (bulletVel * bulletVel);
  const double b = 2.0 * (D3DXVec3Dot(&relativePos, &dstVel));
  const double c = D3DXVec3Dot(&relativePos, &relativePos);

  if (a == 0.0f) return false;
  double d = b * b - (4 * a * c);

  //We have to find smallest non-negative delta time
  double t = 0.f;
  if (d > 0.f) {
	const auto t1 = (-b - sqrt(d)) / (2 * a);
	const auto t2 = (-b + sqrt(d)) / (2 * a);
	if (t1 > 0.f && t2 > 0.f) t = std::min<double>(t1, t2);
	else if (t1 < 0.f && t2 > 0.f) t = t2;
	else if (t1 > 0.f && t2 < 0.f) t = t1;
	else t = 0.f;
  }
  else if (d == 0.0f) t = ((-b) / (2 * a));
  else return false;

  out->travelTime = t;

  if (overrideTravelTime > 0.0f) out->travelTime = overrideTravelTime;

  if (out->travelTime <= 0.0f) return false;


  auto targetVel = dstVel;
  if (Cfg::DBG::_internalUseCurve && Cfg::ESP::_internalCurveIterationCount > 0 && angularDataIn && angularDataIn->valid)
	PredictFinalRotation(
	  dstVel, angularDataIn->angularVelocity, out->travelTime,
	  angularDataIn->orientation, dst, &Cfg::DBG::_internalPredictedOrientation,
	  &targetVel);

  // predicted bullet velocity vector at given time
  double hitVelX = ((relativePos.x / out->travelTime)) + targetVel.x;
  double hitVelY = ((relativePos.y / out->travelTime)) + targetVel.y;
  double hitVelZ = ((relativePos.z / out->travelTime)) + targetVel.z;

  // predicted vector with compenstation = P + VT + 0.5*G*T^2
  out->hitPos.x = (src.x + hitVelX * out->travelTime);
  out->hitPos.y = (src.y + hitVelY * out->travelTime);
  out->hitPos.z = (src.z + hitVelZ * out->travelTime);

  out->bulletDrop = bulletGravity;
  out->bulletVel = bulletVel;
  out->distance = Misc::Distance3D(src, out->hitPos);
  out->origin = dst;

  //Fix for zeroing
  if (zeroying != 0.0f)
	out->hitPos.y += std::sinf(zeroying) * out->distance;

  return true;
}

bool Prediction::GetPredictedAimPoint(ClientPlayer* pLocal, ClientPlayer* pTarget, const Vector& aimPoint, PredictionData_s* dataOut, VeniceClientMissileEntity* pDataIn, WeaponData_s* pWeaponData) {
  if (!IsValidPtr(pLocal) || !IsValidPtr(pTarget)) return false;

  auto pLocalSoldier = pLocal->GetSoldierEntity();
  if (!pLocalSoldier) return false;

  double bulletGravity = -1.f;
  double bulletVelocity = -1.f;
  AngularPredictionData_s angularData;
  angularData.valid = false;

  Vector targetVelocity = ZERO_VECTOR;

  if (auto pVehicle = pTarget->GetClientVehicleEntity(); IsValidPtr(pVehicle)) {
	targetVelocity = pVehicle->m_VelocityVec;

	if (!IsBadPtr((intptr_t*)pVehicle->GetChassisComponent())) {
	  Matrix modelMatrix;
	  pVehicle->GetTransform(&modelMatrix);
	  D3DXQuaternionRotationMatrix(&angularData.orientation, &modelMatrix);
	  angularData.angularVelocity = pVehicle->GetChassisComponent()->m_AngularVelocity;
	  angularData.valid = true;
	}
  }
  else if (auto pTargetSoldier = pTarget->GetSoldierEntity(); IsValidPtr(pTargetSoldier)) {
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
  float overrideRocketTravelTime = 0.0f;
  auto pFiring = pLocalSoldier->GetFiringData();
  if (IsValidPtr(pFiring) && IsValidPtr(pFiring->m_ShotConfigData.m_ProjectileData)) {
	if (IsValidPtr(pFiring->m_ShotConfigData.m_ProjectileData) &&
	  pFiring->m_ShotConfigData.m_ProjectileData->m_HitReactionWeaponType
	  == ProjectileEntityData::AntHitReactionWeaponType_Explosion) //Launcher check
	{
	  auto pMissileData = reinterpret_cast<MissileEntityData*>(pFiring->m_ShotConfigData.m_ProjectileData);
	  if (IsValidPtr(pMissileData) && !pLocal->InVehicle() && weaponType == WeaponClass::At) {
		const auto isLG = pMissileData->IsLaserGuided();
		const auto& ignTime = pMissileData->m_EngineTimeToIgnition;
		const auto& initSpd = pFiring->m_ShotConfigData.m_Speed.z;
		const auto& accel = pMissileData->m_EngineStrength;
		const auto& maxSpd = pMissileData->m_MaxSpeed;
		startPosition = (!isLG) ? G::viewPos : (pDataIn != nullptr) ? pDataIn->m_Position : G::viewPos;
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
		  && pDataIn->m_pMissileEntityData->IsLaserGuided()) {
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
	if (auto pWeapon = pWeaponComp->GetActiveSoldierWeapon(); !IsBadPtr((intptr_t*)pWeapon)) {
	  if (auto pClientWeapon = pWeapon->m_pWeapon; !IsBadPtr((intptr_t*)pClientWeapon) && IsValidPtr(pClientWeapon->m_pWeaponModifier)) {
		auto ZeroEntry = WeaponZeroingEntry(-1.0f, -1.0f);

		auto* pZeroing = pClientWeapon->m_pWeaponModifier->m_ZeroingModifier;
		if (!IsBadPtr((intptr_t*)pZeroing)) {
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
