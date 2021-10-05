/*****************************************************************************/
/**
 * @file    jdRigidBody.h
 * @author  Sergio Diaz (idv17c.sdiaz@uartesdigitales.edu.mx)
 * @date    02/JUN/2021
 * @brief   temp class for component rigidbody
 *
 *
 *
 * @bug     No known bugs.
 */
 /*****************************************************************************/
#pragma once
#include "jdPrerequisitesCore.h"
#include <PxRigidBody.h>
#include <PxShape.h>
#include <PxActor.h>

#include <jdVector4.h>
#include <jdQuaternion.h>
#include "jdComponent.h"
using namespace physx;

namespace jdEngineSDK {

  namespace RIGIDBODY_TYPE {
    enum E {
      STATIC = 0,
      DYNAMIC
    };
  }

  namespace RIGIDBODY_SHAPE {
    enum E {
      SPHERE = 0,
      BOX
    };
  }

  class JD_CORE_EXPORT RigidBody : public Component {
   friend class PhysicalManager;
   friend class CTransform;
   public:
    /**
     * @brief default constructor
     */
    RigidBody() {};

    /**
     * @brief default destructor
     */
    ~RigidBody() {};
    
    /**
     * @brief funtion to upadate  the matrix
     * @param deltatime is the elpase timen between frames
     */
    void
    Update(const float& deltatime) override;
    
    /**
     * @brief funtion save original postion and rotation;
     */
    void
    saveOriginalData();

    /**
     * @brief funtion to set ratio
     */
    void
    setRatio(float ratio);

    /**
     * @brief funtion to set halfSize
     */
    void
    setHalfSize(JDVector3 halfSize);

    /**
     * @brief funtion save original postion and rotation;
     */
    void
    LoadOriginalData();

    /**
     * @brief funtion to set globalPos (Position and rotation)
     */
    void
    setGlobalPos(JDVector3 pos, Quaternion rot);

    /**
     * @brief funtion to set offset
     */
    void
    setOffset(JDVector3 offset);

    /**
     * @brief getPostion
     */
    JDVector3
    getPostion();

    /**
     * @brief get rotation
     */
    Quaternion
    getRotation();

    /**
     * @brief pointer to rigid actor
     */
    PxRigidActor* m_pRigid = nullptr;
   private:

    /**
     * @brief pointer to the shape
     */
    PxShape* m_pShape = nullptr;

    /**
     * @brief velocity of rigid
     */
    JDVector3 m_velocity = { 0,0,0 };

    /**
     * @brief original postion
     */
    JDVector3 m_originalPosition = { 0, 0, 0 };

    /**
     * @brief rotation in degree
     */
    Quaternion m_originalRotation = { 0, 0, 0 };

    /**
     * @brief linear velocity
     */
    PxVec3 m_linearVelocity = { 0, 0, 0 };

    /**
     * @brief angular velocity
     */
    PxVec3 m_angularVelocity = { 0, 0, 0 };

    /**
     * @brief wake counter
     */
    PxReal m_wakeCounter = 0;

   public:

    /**
     * @brief The type of rigid
     */
    RIGIDBODY_TYPE::E m_typeRigid;

    /**
     * @brief The type of shape
     */
    RIGIDBODY_SHAPE::E m_typeShape;

    /**
     * @brief ratio in case of use
     * @note use for sphere
     */
    float m_ratio;

    /**
     * @brief size of box in case of use
     */
    JDVector3 m_halfSize;

    /**
     * @brief ofset of shape
     */
    JDVector3 m_offset = { 0, 0, 0 };

    PxTransform m_transform;
  };
}