//$Id$
//------------------------------------------------------------------------------
//                               CSFixed
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); 
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either 
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Author: Wendy C. Shoan/GSFC
// Created: 2006.03.24
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under MOMS Task
// Order 124.
//
/**
 * Class implementation for the CSFixed class.
 */
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>
#include "Attitude.hpp"
#include "AttitudeException.hpp"
#include "CSFixed.hpp"
#include "GmatBaseException.hpp"
#include <typeinfo>
#include "MessageInterface.hpp"
#include "AttitudeConversionUtility.hpp"
#include "StringUtil.hpp"
#include <cmath>

namespace {
Rvector NormalizedFixedOffset(const Rvector &value)
{
   if (value.GetSize() != 4)
      throw AttitudeException("OffsetQuaternion must have four components (x y z w).");
   Real scale = 0.0;
   for (Integer i = 0; i < 4; ++i)
   {
      if (!std::isfinite(value[i]))
         throw AttitudeException("OffsetQuaternion components must be finite.");
      if (std::abs(value[i]) > scale) scale = std::abs(value[i]);
   }
   if (scale == 0.0)
      throw AttitudeException("OffsetQuaternion magnitude must be greater than 1e-10.");
   Real square = 0.0;
   for (Integer i = 0; i < 4; ++i)
      square += (value[i] / scale) * (value[i] / scale);
   const Real norm = std::sqrt(square);
   if (scale < GmatAttitudeConstants::QUAT_MIN_MAG / norm)
      throw AttitudeException("OffsetQuaternion magnitude must be greater than 1e-10.");
   Rvector normalized(4);
   for (Integer i = 0; i < 4; ++i) normalized[i] = (value[i] / scale) / norm;
   return normalized;
}
}

//#define DEBUG_CSFIXED

//------------------------------------------------------------------------------
// static data
//------------------------------------------------------------------------------
// none 

//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  CSFixed(const std::string &itsName)
//------------------------------------------------------------------------------
/**
 * This method creates an object of the CSFixed class (constructor).
 */
//------------------------------------------------------------------------------
CSFixed::CSFixed(const std::string &itsName) : 
   Kinematic("CoordinateSystemFixed",itsName),
   offsetQuaternion(4, 0.0, 0.0, 0.0, 1.0)
{
   parameterCount = CSFixedParamCount;
   objectTypeNames.push_back("CoordinateSystemFixed");
   attitudeModelName         = "CoordinateSystemFixed";
   setInitialAttitudeAllowed = false;
   // Reserve spaces to handle attribute comments for owned object
   // LOJ: 2013.03.01 for GMT-3353 FIX
   FinalizeCreation();
}

//------------------------------------------------------------------------------
//  CSFixed(const CSFixed &att)
//------------------------------------------------------------------------------
/**
 * This method creates an object of the CSFixed class as a copy of the
 * specified CSFixed class (copy constructor).
 *
 * @param <att> CSFixed object to copy.
 */
//------------------------------------------------------------------------------
CSFixed::CSFixed(const CSFixed& att) :
   Kinematic(att),
   offsetQuaternion(att.offsetQuaternion)
{
}
 
//------------------------------------------------------------------------------
//  CSFixed& operator= (const CSFixed& att)
//------------------------------------------------------------------------------
/**
 * Assignment operator for the CSFixed class.
 *
 * @param att the CSFixed object whose data to assign to "this"
 *            CSFixed.
 *
 * @return "this" CSFixed with data of input CSFixed att.
 */
//------------------------------------------------------------------------------
CSFixed& CSFixed::operator=(const CSFixed& att)
{
   if (this != &att)
   {
      Kinematic::operator=(att);
      offsetQuaternion = att.offsetQuaternion;
   }
   return *this;
}

//------------------------------------------------------------------------------
//  ~CSFixed()
//------------------------------------------------------------------------------
/**
 * Destroys the CSFixed class (constructor).
 */
//------------------------------------------------------------------------------
CSFixed::~CSFixed()
{
   // nothing really to do here ... la la la la la
}


//---------------------------------------------------------------------------
//  bool Initialize() 
//---------------------------------------------------------------------------
 /**
 * Initializes the attitude.
 * 
 * @return Success flag.
 *
  */
//---------------------------------------------------------------------------
bool CSFixed::Initialize()
{
   const Rvector normalized = NormalizedFixedOffset(offsetQuaternion);
   if (!Kinematic::Initialize()) return false;
   offsetQuaternion = normalized;
   return true;
}

bool CSFixed::Validate()
{
   NormalizedFixedOffset(offsetQuaternion);
   return Kinematic::Validate();
}

std::string CSFixed::GetParameterText(const Integer id) const
{
   if (id == OFFSET_QUATERNION) return "OffsetQuaternion";
   return Kinematic::GetParameterText(id);
}

Integer CSFixed::GetParameterID(const std::string &label) const
{
   if (label == "OffsetQuaternion") return OFFSET_QUATERNION;
   return Kinematic::GetParameterID(label);
}

Gmat::ParameterType CSFixed::GetParameterType(const Integer id) const
{
   if (id == OFFSET_QUATERNION) return Gmat::RVECTOR_TYPE;
   return Kinematic::GetParameterType(id);
}

bool CSFixed::IsParameterReadOnly(const Integer id) const
{
   if (id == OFFSET_QUATERNION) return false;
   return Kinematic::IsParameterReadOnly(id);
}

bool CSFixed::IsParameterCloaked(const Integer id) const
{
   // Preserve the existing default script. The GUI still offers this optional
   // field; any non-default quaternion is serialized by the normal vector path.
   if (id == OFFSET_QUATERNION)
      return offsetQuaternion[0] == 0.0 && offsetQuaternion[1] == 0.0 &&
             offsetQuaternion[2] == 0.0 && offsetQuaternion[3] == 1.0;
   return Kinematic::IsParameterCloaked(id);
}

const Rvector& CSFixed::GetRvectorParameter(const Integer id) const
{
   if (id == OFFSET_QUATERNION) return offsetQuaternion;
   return Kinematic::GetRvectorParameter(id);
}

const Rvector& CSFixed::SetRvectorParameter(const Integer id, const Rvector &value)
{
   if (id != OFFSET_QUATERNION) return Kinematic::SetRvectorParameter(id, value);
   const Rvector normalized = NormalizedFixedOffset(value);
   offsetQuaternion = normalized;
   if (isInitialized) needsReinit = true;
   return offsetQuaternion;
}

bool CSFixed::SetStringParameter(const Integer id, const std::string &value)
{
   if (id == OFFSET_QUATERNION)
   {
      // Command-mode vector literals reach the model through the string
      // wrapper. Use the same real-array parser as the other attitude vectors
      // and commit through the validated vector setter.
      const RealArray components = GmatStringUtil::ToRealArray(value);
      Rvector candidate(components.size());
      for (UnsignedInt i = 0; i < components.size(); ++i)
         candidate[i] = components[i];
      SetRvectorParameter(id, candidate);
      return true;
   }
   return Kinematic::SetStringParameter(id, value);
}

Real CSFixed::GetRealParameter(const Integer id, const Integer index) const
{
   if (id != OFFSET_QUATERNION) return GmatBase::GetRealParameter(id, index);
   if (index < 0 || index >= 4)
      throw AttitudeException("OffsetQuaternion component index must be between 0 and 3.");
   return offsetQuaternion[index];
}

Real CSFixed::SetRealParameter(const Integer id, const Real value, const Integer index)
{
   if (id != OFFSET_QUATERNION) return Kinematic::SetRealParameter(id, value, index);
   if (index < 0 || index >= 4)
      throw AttitudeException("OffsetQuaternion component index must be between 0 and 3.");
   Rvector candidate = offsetQuaternion;
   candidate[index] = value;
   NormalizedFixedOffset(candidate);
   // Indexed script initialization supplies components in sequence. Normalize
   // the complete quartet at Initialize, rather than altering later components.
   offsetQuaternion = candidate;
   if (isInitialized) needsReinit = true;
   return value;
}

//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the CSFixed.
 *
 * @return clone of the CSFixed.
 *
 */
//------------------------------------------------------------------------------
GmatBase* CSFixed::Clone() const
{
   std::unique_ptr<CSFixed> value(new CSFixed(*this));
   value->RebindFunctionValueContext(*this);
   return value.release();
}


//------------------------------------------------------------------------------
//  protected methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  virtual void ComputeCosineMatrixAndAngularVelocity(Real atTime)
//------------------------------------------------------------------------------
/**
 * This method computes the current CosineMatrix at the input time atTime.
 *
 * @param atTime the A1Mjd time at which to compute the attitude.
 *
 * @note This method will update the CosineMatrix parameter of the class.
 */
//------------------------------------------------------------------------------
void CSFixed::ComputeCosineMatrixAndAngularVelocity(Real atTime)
{
   if (!isInitialized || needsReinit) Initialize();
   #ifdef DEBUG_CSFIXED
      MessageInterface::ShowMessage("Entering CSFixed::Compute ... time = %lf \n", atTime);
      MessageInterface::ShowMessage("      refCsName = %s\n", refCSName.c_str());
      MessageInterface::ShowMessage("      refCS = %s\n", (refCS? "NOT NULL" : "NULL"));
      if (refCS)
         MessageInterface::ShowMessage("      refCS is %s\n", (refCS->GetName()).c_str());
      MessageInterface::ShowMessage("      RBi    = %s\n", RBi.ToString().c_str());
      MessageInterface::ShowMessage("      angVel = %18.12le %18.12le %18.12le\n",
         angVel[0] * GmatMathConstants::DEG_PER_RAD, angVel[1] * GmatMathConstants::DEG_PER_RAD,
         angVel[2] * GmatMathConstants::DEG_PER_RAD);
   #endif

   // We know RBi, since it is computed on initialization
   // Get the rotation matrix from the reference coordinate system
   Rvector bogus(6,100.0,200.0,300.0,400.0,500.0,600.0);
   Rvector bogus2 = refCS->FromBaseSystem(A1Mjd(atTime), bogus, true, true);  // @todo - do we need FromMJ2000Eq here?
   Rmatrix33 RiI  = (refCS->GetLastRotationMatrix()).Transpose();
   // compute/set current attitude matrix
   const Rmatrix33 offset = AttitudeConversionUtility::ToCosineMatrix(offsetQuaternion);
   dcm              = offset * RiI;
   // Get the rotation dot matrix from the reference coordinate system
   // (it was computed on the last call to FromBaseSystem)         // @todo - do we need FromMJ2000Eq here?
   Rmatrix33 RiIDot = (refCS->GetLastRotationDotMatrix()).Transpose();
   #ifdef DEBUG_CSFIXED
      MessageInterface::ShowMessage("      RiI = %s\n", RBi.ToString().c_str());
      MessageInterface::ShowMessage("      RiIDot = %s\n", RBi.ToString().c_str());
   #endif
   
   Rmatrix33 wxIBB  = - (offset * RiIDot * (dcm.Transpose()));
   // set current angular velocity from this matrix
   angVel(0)   = wxIBB(2,1);
   angVel(1)   = wxIBB(0,2);
   angVel(2)   = wxIBB(1,0);
   #ifdef DEBUG_CSFIXED
   MessageInterface::ShowMessage("Matrix RiIDot = %s\n", RiIDot.ToString().c_str());
   MessageInterface::ShowMessage("Matrix RBi = %s\n", RBi.ToString().c_str());
   MessageInterface::ShowMessage("Matrix dcm = %s\n", dcm.ToString().c_str());
   MessageInterface::ShowMessage("Matrix wxIBB = %s\n", wxIBB.ToString().c_str());
   MessageInterface::ShowMessage(
   "EXITING CSFixed::Compute ... angVel = %18.12le %18.12le %18.12le\n",
   angVel[0] * GmatMathConstants::DEG_PER_RAD,
   angVel[1] * GmatMathConstants::DEG_PER_RAD,
   angVel[2] * GmatMathConstants::DEG_PER_RAD);
   MessageInterface::ShowMessage("   and dcm = %s\n", dcm.ToString().c_str());
   #endif
}


//------------------------------------------------------------------------------
//  virtual void ComputeCosineMatrixAndAngularVelocity(GmatTime &atTime)
//------------------------------------------------------------------------------
/**
* This method computes the current CosineMatrix at the input time atTime.
*
* @param atTime the GmatTime time at which to compute the attitude.
*
* @note This method will update the CosineMatrix parameter of the class.
*/
//------------------------------------------------------------------------------
void CSFixed::ComputeCosineMatrixAndAngularVelocity(GmatTime &atTime)
{
   ///@todo: it needs to add code for high precision of time
   ComputeCosineMatrixAndAngularVelocity(atTime.GetMjd());
}


//------------------------------------------------------------------------------
//  private methods
//------------------------------------------------------------------------------
// none 

// Reset/copy through the inherited polymorphic contract without slicing.
void CSFixed::Copy(const GmatBase *original)
{
   const auto *source = dynamic_cast<const CSFixed*>(original);
   if (!source || typeid(*source) != typeid(CSFixed) ||
       typeid(*this) != typeid(CSFixed))
      throw GmatBaseException("Cannot copy incompatible object into CSFixed");
   if (source == this)
      return;
   const auto previousContext = GetFunctionValueContext();
   operator=(*source);
   RebindFunctionValueContext(*source);
}
