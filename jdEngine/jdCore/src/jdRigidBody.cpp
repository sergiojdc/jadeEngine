#include "jdRigidBody.h"
#include "jdGameObject.h"
#include "jdComponentTransform.h"
#include "jdRigidBody.h"
#include <PxRigidDynamic.h>

namespace jdEngineSDK {
  void 
  RigidBody::Update(const float& deltatime) {
    auto component = m_myObject->getComponent(COMPONENT_TYPE::TRANSFORM);
    CTransform* trans = reinterpret_cast<CTransform*>(component.get());
    m_transform = m_pRigid->getGlobalPose();
    trans->position.x = m_transform.p.x - m_offset.x;
    trans->position.y = m_transform.p.y - m_offset.y;
    trans->position.z = m_transform.p.z - m_offset.z;

    trans->rotation = Quaternion(m_transform.q.x, m_transform.q.y, m_transform.q.z, m_transform.q.w);
    trans->Update(deltatime);
  }

  void
  RigidBody::saveOriginalData() {
    auto component = m_myObject->getComponent(COMPONENT_TYPE::TRANSFORM);
    CTransform* trans = reinterpret_cast<CTransform*>(component.get());
    m_originalPosition = trans->position + m_offset;
    m_originalRotation = trans->rotation;
    if (RIGIDBODY_TYPE::DYNAMIC == m_typeRigid) {
      auto rigDynamic = reinterpret_cast<PxRigidDynamic*>(m_pRigid);
      m_angularVelocity = rigDynamic->getAngularVelocity();
      m_linearVelocity = rigDynamic->getLinearVelocity();
      m_wakeCounter = rigDynamic->getWakeCounter();
    }
  }

  void 
  RigidBody::setRatio(float ratio) {
    if (RIGIDBODY_SHAPE::SPHERE == m_typeShape) {
      auto rig = reinterpret_cast<PxRigidDynamic*>(m_pRigid);
      PxShape* shape;
      rig->getShapes(&shape, 1, 0);
      if (nullptr != shape && PxGeometryType::eSPHERE == shape->getGeometryType()) {
        PxSphereGeometry sphere;
        shape->getSphereGeometry(sphere);
        sphere.radius = ratio;
        shape->setGeometry(sphere);
        m_ratio = ratio;
      }
    }
  }

  void 
  RigidBody::setHalfSize(JDVector3 halfSize) {
    if (RIGIDBODY_SHAPE::BOX == m_typeShape) {
      auto rig = reinterpret_cast<PxRigidDynamic*>(m_pRigid);
      PxShape* shape;
      rig->getShapes(&shape, 1, 0);
      if (nullptr != shape && PxGeometryType::eBOX == shape->getGeometryType()) {
        PxBoxGeometry Box;
        shape->getBoxGeometry(Box);
        Box.halfExtents = { halfSize.x, halfSize.y, halfSize.z };
        shape->setGeometry(Box);
        m_halfSize = halfSize;
      }
    }
  }

  void 
  RigidBody::LoadOriginalData() {
    auto component = m_myObject->getComponent(COMPONENT_TYPE::TRANSFORM);
    CTransform* trans = reinterpret_cast<CTransform*>(component.get());
    trans->position = m_originalPosition - m_offset;
    trans->rotation = m_originalRotation;
    PxTransform rigidPos;
    rigidPos.p.x = m_originalPosition.x;
    rigidPos.p.y = m_originalPosition.y;
    rigidPos.p.z = m_originalPosition.z;
    rigidPos.q = PxQuat(m_originalRotation.x, 
                        m_originalRotation.y, 
                        m_originalRotation.z, 
                        m_originalRotation.w);

    m_pRigid->setGlobalPose(rigidPos, true);
    if (RIGIDBODY_TYPE::DYNAMIC == m_typeRigid) {
      auto rigDynamic = reinterpret_cast<PxRigidDynamic*>(m_pRigid);
      rigDynamic->setAngularVelocity(m_angularVelocity);
      rigDynamic->setLinearVelocity(m_linearVelocity);
      rigDynamic->setWakeCounter(m_wakeCounter);
    }

    trans->Update(0.0f);
  }

  void
  RigidBody::setGlobalPos(JDVector3 pos, Quaternion rot) {
    JDVector3 rigidPos = pos + m_offset;
    PxQuat quar = { rot.x, rot.y, rot.z, rot.w };
    m_pRigid->setGlobalPose(PxTransform(rigidPos.x, rigidPos.y, rigidPos.z, quar));
  }
  
  void
  RigidBody::setOffset(JDVector3 offset) {
    m_offset = offset;
    auto component = m_myObject->getComponent(COMPONENT_TYPE::TRANSFORM);
    CTransform* trans = reinterpret_cast<CTransform*>(component.get());
    setGlobalPos(trans->position, trans->rotation);
  }

  JDVector3 
  RigidBody::getPostion() {
    return JDVector3(m_transform.p.x, m_transform.p.y, m_transform.p.z);
  }

  Quaternion 
  RigidBody::getRotation() {
    return Quaternion(m_transform.q.x, m_transform.q.y, m_transform.q.z, m_transform.q.w);
  }
}
