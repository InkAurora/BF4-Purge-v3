#include "Engine.h"
#include "xorstr.hpp"
#include <string>

ClientPlayer* PlayerManager::GetPlayerById(unsigned int id) {
  //You could use GetPlayers() virtual as well, but you wont get observators this way.

  if (id < 70)
    return this->m_ppPlayers[id];
  return nullptr;
}

ClientPlayer* PlayerManager::GetLocalPlayer() {
  return m_pLocalPlayer;
}

PlayerManager* PlayerManager::GetInstance() {
  if (ClientGameContext* pGameCtx = ClientGameContext::GetInstance();
    IsValidPtr(pGameCtx) &&
    IsValidPtr(pGameCtx->m_pPlayerManager))
    return pGameCtx->m_pPlayerManager;
  return nullptr;
}

bool ClientPlayer::InVehicle() {
  if (!IsValidPtr(this)) return false;
  if (!IsValidPtr(m_pControlledControllable)) return false;
  return InVehicleFn();
}

bool ClientPlayer::GetCurrentWeaponData(WeaponData_s* pDataOut) {
  pDataOut->isValid = false;
  pDataOut->gravity = -999.f;
  pDataOut->gunID = -1;
  pDataOut->gunName = xorstr_("Weapon");
  pDataOut->speed = { 0.0f, 0.0f, -1.f };
  pDataOut->pWeapon = nullptr;

  if (!IsValidPtr(this)) return false;

  auto pWeaponFiring = WeaponFiring::GetInstance();
  if (!IsValidPtr(pWeaponFiring)) return false;

  //After debugging I've found that when pFiringData == 0x3893E06 || 0x10F00000030 
  //game crashes (mostly when entering the vehicle).
  //You could implemend some kind of timer and delay it when entering the vehicle but yeah, its lame.

  if (!IsValidPtr(pWeaponFiring->m_pPrimaryFire)) return false;

  auto pFiringData = pWeaponFiring->m_pPrimaryFire->m_FiringData;	// <--- Sometimes crash occurs here as well without any information from debugger.

  if (!IsValidPtr(pFiringData)) return false;

  auto pProjData = pFiringData->m_ShotConfigData.m_ProjectileData;
  if (!IsValidPtr(pProjData)) return false;

  pDataOut->speed.z = pFiringData->m_ShotConfigData.m_Speed.z;

  //Is launcher
  if (pProjData->m_Gravity < 128.f && pProjData->m_Gravity > -128.f)
    pDataOut->gravity = pProjData->m_Gravity;
  else if (auto pLauncher = reinterpret_cast<MissileEntityData*>(pProjData); IsValidPtr(pLauncher)) {
    if (pLauncher->m_Gravity < 128.f && pLauncher->m_Gravity > -128.f) {
      pDataOut->gravity = this->InVehicle() ? 0.0f : pLauncher->m_Gravity;
    }
  }
  else return false;

  if (this->InVehicle()) {
    if (!IsValidPtr(pWeaponFiring->m_weaponComponentData)) return false;
    if (!IsValidPtr(pWeaponFiring->m_weaponComponentData->m_pDamageGiverName)) return false;
    pDataOut->gunName.assign(pWeaponFiring->m_weaponComponentData->m_pDamageGiverName);
    if (pDataOut->gunName.empty()) return false;

    auto pEntry = this->GetEntryComponent();
    if (!IsValidPtr(pEntry)) return false;

    auto stance = pEntry->getActiveStance();

    if (pEntry->isStanceAvailable(stance)) {
      if (pEntry->m_weapons.size() <= 0) return false;

      for (size_t i = 0; i < pEntry->m_weapons.size(); i++) {
        EntryComponent::FiringCallbacks* pFiringCB = pEntry->m_weapons[i];
        if (!IsValidPtr(pFiringCB) || !IsValidPtr(pFiringCB->m_info) || !IsValidPtr(pFiringCB->m_info->GetWeaponFiring())) continue;
        if (pFiringCB->m_info->GetWeaponFiring() == pWeaponFiring) {
          pDataOut->gunID = i;
          pDataOut->pWeapon = pFiringCB->m_info->GetWeapon();
          break;
        }
      }
    }
  }

  pDataOut->isValid = true;
  return true;
}

VehicleData::VehicleType VehicleData::GetVehicleType() {
  return *reinterpret_cast<VehicleData::VehicleType*>((uintptr_t)this + 0xE8);
}

VehicleData::VehicleCategory VehicleData::GetVehicleCategory() {
  if (IsAirVehicle()) return VehicleCategory::AIR;
  if (IsGroundVehicle()) return VehicleCategory::GROUND;
  if (IsWaterVehicle()) return VehicleCategory::WATER;
  return VehicleCategory::UNUSED;
}

bool VehicleData::IsAirVehicle() {
  switch (GetVehicleType()) {
  case VehicleType::HELIATTACK:
  case VehicleType::HELISCOUT:
  case VehicleType::HELITRANS:
  case VehicleType::JETBOMBER:
  case VehicleType::JET:
  case VehicleType::MAV:
  case VehicleType::UAV:
    return true;
  default:
    return false;
  }
}

bool VehicleData::IsGroundVehicle() {
  switch (GetVehicleType()) {
  case VehicleType::TANK:
  case VehicleType::TANKAA:
  case VehicleType::TANKARTY:
  case VehicleType::TANKAT:
  case VehicleType::TANKIFV:
  case VehicleType::TANKLC:
  case VehicleType::JEEP:
  case VehicleType::CAR:
  case VehicleType::EODBOT:
  case VehicleType::MORTAR:
  case VehicleType::STATICAA:
  case VehicleType::STATICAT:
  case VehicleType::STATIONARY:
  case VehicleType::STATIONARYWEAPON:
    return true;
  default:
    return false;
  }
}

bool VehicleData::IsWaterVehicle() {
  return (GetVehicleType() == VehicleType::BOAT);
}

bool VehicleData::IsInJet() {
  const auto& type = GetVehicleType();
  return (type == VehicleType::JET || type == VehicleType::JETBOMBER);
}

bool VehicleData::IsInHeli() {
  switch (GetVehicleType()) {
  case VehicleType::HELIATTACK:
  case VehicleType::HELISCOUT:
  case VehicleType::HELITRANS:
    return true;
  default:
    return false;
  }
}

std::string ClientControllableEntity::GetVehicleName() {
  if (!IsValidPtr(m_pData) || !m_pData->m_NameID) return "";
  std::string name = m_pData->m_NameID;
  if (name.size() >= 11) return name.substr(11);
  return name;
}

bool ClientControllableEntity::IsAlive() {
  if (!IsValidPtr(this)) return false;
  if (!IsValidPtr(m_pHealthComp)) return false;
  if (m_pHealthComp->m_Health > 0.0f) return true;
  return false;
}

bool ClientControllableEntity::IsTVGuidedMissile() {
  //Only works for local player (obviously).
  auto pFire = *reinterpret_cast<WeaponFiring**>(OFFSET_PPCURRENTWEAPONFIRING);
  if (!IsValidPtr(pFire) || !IsValidPtr(pFire->m_weaponComponentData)) return true;
  return false;
}

D3DXVECTOR2* ClientSoldierEntity::GetAimAngles() {
  auto pWeaponComp = m_pWeaponComponent;
  if (!IsValidPtr(pWeaponComp)) return nullptr;

  auto pWeapon = pWeaponComp->GetActiveSoldierWeapon();
  if (!IsValidPtr(pWeapon)) return nullptr;

  auto pAA = pWeapon->m_pAuthoritativeAiming;
  if (!IsValidPtr(pAA)) return nullptr;

  if (IsValidPtr(pAA->m_pFPSAimer)) return (&pAA->m_pFPSAimer->m_AimAngles);
  return nullptr;
}

FiringFunctionData* ClientSoldierEntity::GetFiringData() {
  auto p = WeaponFiring::GetInstance();

  if (!IsValidPtr(p)) return nullptr;
  if (!IsValidPtr(p->m_pPrimaryFire)) return nullptr;
  if (!IsValidPtr(p->m_pPrimaryFire->m_FiringData)) return nullptr;

  return p->m_pPrimaryFire->m_FiringData;
}

ClientSoldierEntity* ClientPlayer::GetSoldierEntity() {
  if (!IsValidPtr(this)) return nullptr;

  if (IsValidPtr(m_pAttachedControllable) && this->InVehicle()) {
    auto pClientSoldierEntity = (ClientSoldierEntity*)((DWORD_PTR)this->GetCharacterEntity() - 0x188);
    return IsValidPtr(pClientSoldierEntity) ? pClientSoldierEntity : nullptr;
  }

  if (IsValidPtr(m_pControlledControllable))
    return m_pControlledControllable;

  return nullptr;
}

ClientControllableEntity* ClientPlayer::GetVehicleEntity() {
  if (this != nullptr && this->InVehicle())
    return (ClientControllableEntity*)m_pAttachedControllable;
  return nullptr;
}

ClientVehicleEntity* ClientPlayer::GetClientVehicleEntity() {
  if (this != nullptr && this->InVehicle())
    return (ClientVehicleEntity*)m_pControlledControllable;
  return nullptr;
}

bool ClientPlayer::GetWeaponTransform(Matrix& out) {
  ClientSoldierEntity* pSoldier = this->GetSoldierEntity();
  if (!IsValidPtr(pSoldier) || !IsValidPtr(pSoldier->m_pWeaponComponent)) return false;
  if (!IsValidPtr(pSoldier->m_pWeaponComponent->m_WeaponTransform)) return false;
  out = pSoldier->m_pWeaponComponent->m_WeaponTransform;
  return true;
}

bool ClientPlayer::GetWeaponShootSpace(Matrix& out) {
  ClientSoldierEntity* pSoldier = this->GetSoldierEntity();
  if (!IsValidPtr(pSoldier) || !IsValidPtr(pSoldier->m_pWeaponComponent)) return false;
  SoldierWeapon* pWeaponC = pSoldier->m_pWeaponComponent->GetActiveSoldierWeapon();
  if (!IsValidPtr(pWeaponC) || !IsValidPtr(pWeaponC->m_pWeapon)) return false;
  out = pWeaponC->m_pWeapon->m_ShootSpace;
  return true;
}

bool ClientPlayer::GetBone(int BoneId, D3DXVECTOR3& BoneOut) {
  auto pSoldier = this->GetSoldierEntity();
  if (!IsValidPtr(pSoldier)) return false;
  if (!pSoldier->IsAlive()) return false;
  if (!IsValidPtr(pSoldier->m_pRagdollComponent)) return false;
  auto pRagdoll = pSoldier->m_pRagdollComponent;
  UpdatePoseResultData PoseResult = pRagdoll->m_PoseResultData;
  if (PoseResult.m_ValidTransforms) {
    QuatTransform* pQuat = PoseResult.m_ActiveWorldTransforms;
    if (!pQuat)
      return false;

    D3DXVECTOR4 Bone = pQuat[BoneId].m_TransAndScale;
    BoneOut = D3DXVECTOR3(Bone.x, Bone.y, Bone.z);
    return true;
  }
  return false;
}

bool ClientPlayer::IsVisible(Matrix shootSpace, int boneId) { 
  auto pRayCaster = Main::GetInstance()->GetRayCaster();
  if (!IsValidPtr(pRayCaster)) return false;

  Vector target = ZERO_VECTOR; this->GetBone(boneId, target);

  Vector origin = { shootSpace._41, shootSpace._42, shootSpace._43 };

  __declspec(align(16)) Vector	from = origin;
  __declspec(align(16)) Vector  to = target;

  RayCastHit hit;

  // Credits to dudeinberlin & stevemk14ebr

  if (!pRayCaster->PhysicsRayQuery(xorstr_("ControllableFinder"), &from, &to, &hit, 0x4 | 0x10 | 0x20 | 0x80, NULL))
    return true;
  else {
    if (!IsValidPtr(hit.m_rigidBody))
      return false;
    if (hit.m_material.isSeeThrough() || hit.m_material.isPenetrable())
      return true;
    if (hit.m_rigidBody == (void*)this)
      return true;
  }

  return false;
}

bool ClientPlayer::IsAimingAtYou(ClientPlayer* pLocal, float &anglePercentOut) { // Credits to TIGERHax & A200K
  auto pGameRenderer = GameRenderer::GetInstance();
  if (!IsValidPtr(pGameRenderer) || !IsValidPtr(pGameRenderer->m_pRenderView)) return false;
  Vector vLocal = (Vector)&pGameRenderer->m_pRenderView->m_ViewInverse._41;

  Matrix wTransform; GetWeaponTransform(wTransform);

  Vector vTarget = (Vector)&wTransform._41;
  Vector vForward = (Vector)&wTransform._31;
  
  Vector vDistance = vLocal - vTarget;
  D3DXVec3Normalize(&vDistance, &vDistance);

  float fAngle = D3DXToDegree(acos(D3DXVec3Dot(&vForward, &vDistance)));

  if (fAngle <= 60) {
    anglePercentOut = 100 - (fAngle * (100.0f / 60));
    return true;
  }

  return false;
}

WeaponZeroingEntry ZeroingModifier::GetZeroLevelAt(int index) {
  if (index > -1)
    return m_ppZeroLevels[index];
  else
    if (!IsValidPtr(this)) return WeaponZeroingEntry(0.0f, 0.0f);
    return WeaponZeroingEntry(m_DefaultZeroingDistance, 0.0f);
}

const char* AnimationSkeleton::GetBoneNameAt(int i) {
  if (i <= m_BoneCount)
    return m_SkeletonAsset->m_ppBoneNames[i];
  return "";
}

Vector VehicleTurret::GetVehicleCameraOrigin() {
  if (!this) return { 0, 0, 0 };
  return D3DXVECTOR3(m_VehicleMatrix._41, m_VehicleMatrix._42, m_VehicleMatrix._43);
}

Vector VehicleTurret::GetVehicleCrosshair() {
  if (!this) return { 0, 0, 0 };
  auto pos = D3DXVECTOR3(m_VehicleMatrix._41, m_VehicleMatrix._42, m_VehicleMatrix._43);
  auto forward = D3DXVECTOR3(m_VehicleMatrix._31, m_VehicleMatrix._32, m_VehicleMatrix._33);
  return ((forward * 100.0f) + pos);
}

//m_MinTurnAngle > -1 when laser guided
bool MissileEntityData::IsLaserGuided() {
  if (m_MinTurnAngle > 0.0f) return true;
  return false;
}
bool MissileEntityData::IsLockable() {
  if (m_TimeToActivateGuidingSystem > 0.0f) return true;
  return false;
}

SoldierWeapon* SoldierWeaponComponent::GetActiveSoldierWeapon() {
  if (!m_Handler)
    return nullptr;

  if (m_CurrentWeaponIndex < 7)
    return m_Handler->m_pWeaponList[m_CurrentWeaponIndex];
  else
    return nullptr;
}

int SoldierWeaponComponent::GetSlot() {
  return m_CurrentWeaponIndex;
  //0 or 1 is primary or secondary
}

WeaponClass WeaponFiring::GetWeaponClass() {
  auto pData = reinterpret_cast<WeaponEntityData*>(this->m_weaponComponentData);
  if (IsValidPtr(pData)) return pData->m_WeaponClass;
  return WeaponClass::None;
}

IPhysicsRayCaster* Main::GetRayCaster() {
  auto pCtx = ClientGameContext::GetInstance();
  if (!IsValidPtr(pCtx)) return nullptr;
  if (!IsValidPtr(pCtx->m_pLevel)) return nullptr;
  if (!IsValidPtr(pCtx->m_pLevel->m_pHavokPhysics)) return nullptr;
  if (!IsValidPtr(pCtx->m_pLevel->m_pHavokPhysics->m_pRayCaster)) return nullptr;
  return pCtx->m_pLevel->m_pHavokPhysics->m_pRayCaster;
}

bool IPhysicsRayCaster::IsPointVisible(const Vector& src, const Vector& dst, RayCastHit* outHit) {
  __declspec(align(16)) Vector from = { src.x, src.y, src.z };
  __declspec(align(16)) Vector to = { dst.x, dst.y, dst.z };

  return !this->PhysicsRayQuery(
    xorstr_("ControllableFinder"), &from, &to, outHit,
    (0x4 | 0x10 | 0x20 | 0x80), NULL);
}

bool IPhysicsRayCaster::IsIntersectingEntity(const Ray& ray, ClientPlayer* const pEntity) {
  auto forwardRay = ray.origin + (ray.direction * 9999999.f);

  __declspec(align(16)) Vector from = { ray.origin.x, ray.origin.y, ray.origin.z };
  __declspec(align(16)) Vector to = { forwardRay.x, forwardRay.y, forwardRay.z };

  RayCastHit hit;
  auto visible = !this->PhysicsRayQuery(
    xorstr_("ControllableFinder"), &from, &to, &hit,
    (0x4 | 0x10 | 0x20 | 0x80), NULL);

  if (visible && (hit.m_rigidBody == (void*)pEntity))
    return true;

  //TODO: if its penetrable - cast another rays in a while loop from hit pos 
  //to direction(or desired end pos) and check for intersection

  return false;
}

bool IPhysicsRayCaster::IsIntersectingAABB(const Vector& dir, const AABB& aabb, float* rayHitLenght) {
  Vector dirfrac = {};

  dirfrac.x = 1.0f / dir.x;
  dirfrac.y = 1.0f / dir.y;
  dirfrac.z = 1.0f / dir.z;

  // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
  // r.org is origin of ray
  float t1 = (aabb.min.x - aabb.origin.x) * dirfrac.x;
  float t2 = (aabb.max.x - aabb.origin.x) * dirfrac.x;
  float t3 = (aabb.min.y - aabb.origin.y) * dirfrac.y;
  float t4 = (aabb.max.y - aabb.origin.y) * dirfrac.y;
  float t5 = (aabb.min.z - aabb.origin.z) * dirfrac.z;
  float t6 = (aabb.max.z - aabb.origin.z) * dirfrac.z;

  float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
  float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

  // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
  if (tmax < 0) {
    *rayHitLenght = tmax;
    return false;
  }

  // if tmin > tmax, ray doesn't intersect AABB
  if (tmin > tmax) {
    *rayHitLenght = tmax;
    return false;
  }

  *rayHitLenght = tmin;
  return true;
}

bool IPhysicsRayCaster::IsIntersectingOBB(const Ray& ray, const AxisAlignedBoundingBox& aabb, Matrix& matrix, float* rayHitLenghtOut) {
  //https://github.com/opengl-tutorials/ogl/blob/master/misc05_picking/misc05_picking_custom.cpp
  // Intersection method from Real-Time Rendering and Essential Mathematics for Games

  float tMin = 0.0f;
  float tMax = 100000.0f;

  Vector OBBposition_worldspace(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]);

  Vector delta = OBBposition_worldspace - ray.origin;

  // Test intersection with the 2 planes perpendicular to the OBB's X axis
  {
    Vector xaxis(matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]);
    float e = D3DXVec3Dot(&xaxis, &delta);
    float f = D3DXVec3Dot(&ray.direction, &xaxis);

    if (fabs(f) > 0.001f) { // Standard case

      float t1 = (e + aabb.min.x) / f; // Intersection with the "left" plane
      float t2 = (e + aabb.max.x) / f; // Intersection with the "right" plane
      // t1 and t2 now contain distances betwen ray origin and ray-plane intersections

      // We want t1 to represent the nearest intersection, 
      // so if it's not the case, invert t1 and t2
      if (t1 > t2) {
        float w = t1; t1 = t2; t2 = w; // swap t1 and t2
      }

      // tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
      if (t2 < tMax)
        tMax = t2;
      // tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
      if (t1 > tMin)
        tMin = t1;

      // And here's the trick :
      // If "far" is closer than "near", then there is NO intersection.
      // See the images in the tutorials for the visual explanation.
      if (tMax < tMin)
        return false;

    }
    else { // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
      if (-e + aabb.min.x > 0.0f || -e + aabb.max.x < 0.0f)
        return false;
    }
  }


  // Test intersection with the 2 planes perpendicular to the OBB's Y axis
  // Exactly the same thing than above.
  {
    Vector yaxis(matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]);
    float e = D3DXVec3Dot(&yaxis, &delta);
    float f = D3DXVec3Dot(&ray.direction, &yaxis);

    if (fabs(f) > 0.001f) {

      float t1 = (e + aabb.min.y) / f;
      float t2 = (e + aabb.max.y) / f;

      if (t1 > t2) { float w = t1; t1 = t2; t2 = w; }

      if (t2 < tMax)
        tMax = t2;
      if (t1 > tMin)
        tMin = t1;
      if (tMin > tMax)
        return false;

    }
    else {
      if (-e + aabb.min.y > 0.0f || -e + aabb.max.y < 0.0f)
        return false;
    }
  }


  // Test intersection with the 2 planes perpendicular to the OBB's Z axis
  // Exactly the same thing than above.
  {
    Vector zaxis(matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]);
    float e = D3DXVec3Dot(&zaxis, &delta);
    float f = D3DXVec3Dot(&ray.direction, &zaxis);

    if (fabs(f) > 0.001f) {

      float t1 = (e + aabb.min.z) / f;
      float t2 = (e + aabb.max.z) / f;

      if (t1 > t2) { float w = t1; t1 = t2; t2 = w; }

      if (t2 < tMax)
        tMax = t2;
      if (t1 > tMin)
        tMin = t1;
      if (tMin > tMax)
        return false;

    }
    else {
      if (-e + aabb.min.z > 0.0f || -e + aabb.max.z < 0.0f)
        return false;
    }
  }

  *rayHitLenghtOut = tMin;
  return true;
}

bool AxisAlignedBox::Intersect(const Vector& point, const Vector& dir) const {
  float
    tmin = 0.0f, tmax = 0.0f,
    tymin = 0.0f, tymax = 0.0f,
    tzmin = 0.0f, tzmax = 0.0f;

  if (dir.x >= 0.0f) {
    tmin = (m_Min.x - point.x) / dir.x;
    tmax = (m_Max.x - point.x) / dir.x;
  }
  else {
    tmin = (m_Max.x - point.x) / dir.x;
    tmax = (m_Min.x - point.x) / dir.x;
  }

  if (dir.y >= 0) {
    tymin = (m_Min.y - point.y) / dir.y;
    tymax = (m_Max.y - point.y) / dir.y;
  }
  else {
    tymin = (m_Max.y - point.y) / dir.y;
    tymax = (m_Min.y - point.y) / dir.y;
  }

  if (tmin > tymax || tymin > tmax)
    return false;

  if (tymin > tmin)
    tmin = tymin;

  if (tymax < tmax)
    tmax = tymax;

  if (dir.z >= 0) {
    tzmin = (m_Min.z - m_Min.z) / dir.z;
    tzmax = (m_Max.z - point.z) / dir.z;
  }
  else {
    tzmin = (m_Max.z - point.z) / dir.z;
    tzmax = (m_Min.z - point.z) / dir.z;
  }

  if (tmin > tzmax || tzmin > tmax)
    return false;

  //behind us
  if (tmin < 0 || tmax < 0)
    return false;

  return true;
}

IPhysicsRayCaster::Ray::Ray(const Vector& _origin, const Vector& _direction) {
  this->origin = _origin;
  this->direction = _direction;
}

void IPhysicsRayCaster::Ray::Init(const Vector& _origin, const Vector& _direction) {
  this->origin = _origin;
  this->direction = _direction;
}

IPhysicsRayCaster::AxisAlignedBoundingBox::AxisAlignedBoundingBox(const Vector& _min, const Vector& _max) {
  this->min = _min;
  this->max = _max;
}
