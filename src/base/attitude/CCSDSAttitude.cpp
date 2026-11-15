//$Id$
//------------------------------------------------------------------------------
//                               CCSDSAttitude
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
// Created: 2013.11.26
//
/**
 * Class implementation for the CCSDS-AEM (CCSDSAttitude) class.
 */
//------------------------------------------------------------------------------

#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>
#include "Attitude.hpp"
#include "AttitudeException.hpp"
#include "CCSDSAttitude.hpp"
#include "GmatBaseException.hpp"
#include <typeinfo>
#include "MessageInterface.hpp"
#include "AttitudeConversionUtility.hpp"

//#define DEBUG_CCSDS_ATTITUDE
//#define DEBUG_CCSDS_ATTITUDE_INIT
//#define DEBUG_CCSDS_ATTITUDE_GET_SET

//------------------------------------------------------------------------------
// static data
//------------------------------------------------------------------------------
// none

//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  CCSDSAttitude(const std::string &itsName)
//------------------------------------------------------------------------------
/**
 * This method creates an object of the CCSDSAttitude class
 * (default Constructor).
 */
namespace
{
std::unique_ptr<CCSDSAEMReader> CopyOwnedAemReader(const CCSDSAEMReader *source)
{
   std::unique_ptr<CCSDSEMReader> value(source ? source->Clone() : new CCSDSAEMReader());
   if (source && value.get() == source)
   {
      value.release();
      throw AttitudeException("AEM reader Clone returned its source.");
   }
   auto *typed = dynamic_cast<CCSDSAEMReader*>(value.get());
   if (!typed) throw AttitudeException("Incompatible copied AEM reader.");
   value.release();
   return std::unique_ptr<CCSDSAEMReader>(typed);
}
}

//------------------------------------------------------------------------------
CCSDSAttitude::CCSDSAttitude(const std::string &attName) :
   Attitude("CCSDS-AEM",attName)
{
   parameterCount            = CCSDSAttitudeParamCount;
   objectTypeNames.push_back("CCSDS-AEM");
   attitudeModelName         = "CCSDS-AEM";
   modifyCoordSysAllowed     = false;
   setInitialAttitudeAllowed = false;
   // No rates are computed for this model
   modelComputesRates        = false;

   reader = new CCSDSAEMReader();
 }

 //------------------------------------------------------------------------------
//  CCSDSAttitude(const CCSDSAttitude &att)
//------------------------------------------------------------------------------
/**
 * This method creates an object of the CCSDSAttitude class as a copy of the
 * specified CCSDSAttitude class.
 * (copy constructor)
 *
 * @param <att> CCSDSAttitude object to copy.
 */
//------------------------------------------------------------------------------
CCSDSAttitude::CCSDSAttitude(const CCSDSAttitude& att) :
   Attitude(att)
{
   reader = CopyOwnedAemReader(att.reader).release();
}

//------------------------------------------------------------------------------
//  CCSDSAttitude& operator= (const CCSDSAttitude& att)
//------------------------------------------------------------------------------
/**
 * Assignment operator for the CCSDSAttitude class.
 *
 * @param att the CCSDSAttitude object whose data to assign to "this"
 *            CCSDSAttitude.
 *
 * @return "this" CCSDSAttitude with data of input CCSDSAttitude att.
 */
//------------------------------------------------------------------------------
CCSDSAttitude& CCSDSAttitude::operator=(const CCSDSAttitude& att)
{
   if (&att == this)
      return *this;
   if (GmatBase::IsNativeRestorationCopy())
   {
      if (aemFileFullPath != att.aemFileFullPath || (reader == nullptr) != (att.reader == nullptr))
         throw AttitudeException("Prepare the AEM reader before changing an active restoration graph.");
      Attitude::operator=(att);
      return *this;
   }
   auto prepared = CopyOwnedAemReader(att.reader);
   Attitude::operator=(att);
   delete reader;
   reader = prepared.release();
   return *this;
}

//------------------------------------------------------------------------------
//  ~CCSDSAttitude()
//------------------------------------------------------------------------------
/**
 * Destroys the CCSDSAttitude class (destructor).
 */
//------------------------------------------------------------------------------
CCSDSAttitude::~CCSDSAttitude()
{
   if (reader) delete reader;
}


//---------------------------------------------------------------------------
//  bool Initialize()
//---------------------------------------------------------------------------
 /**
 * Initializes the CCSDSAttitude.
 *
 * @return Success flag.
 *
  */
//---------------------------------------------------------------------------
bool CCSDSAttitude::Initialize()
{
   #ifdef DEBUG_CCSDS_ATTITUDE_INIT
      MessageInterface::ShowMessage("Entering CCSDSAttitude::Initialize\n");
   #endif
   bool isOK = Attitude::Initialize();
   if (!isOK) return false;

   // Changed to use aemFileFullPath (LOJ: 2014.06.26)
   //if (aemFile == "")
   if (aemFileFullPath == "")
   {
      std::string errmsg = "Error - AEM file name not set on CCSDS-AEM object.\n";
      throw AttitudeException(errmsg);
   }
   
   #ifdef DEBUG_CCSDS_ATTITUDE_INIT
      MessageInterface::ShowMessage("In CCSDSAttitude::Initialize, about to call reader which is %sNULL\n",
            (reader? "NOT " : ""));
   #endif
   //reader->SetFile(aemFile);
   reader->SetFile(aemFileFullPath);
   reader->Initialize();
   #ifdef DEBUG_CCSDS_ATTITUDE_INIT
      MessageInterface::ShowMessage("EXITing CCSDSAttitude::Initialize\n");
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the CCSDSAttitude.
 *
 * @return clone of the CCSDSAttitude.
 *
 */
//------------------------------------------------------------------------------
GmatBase* CCSDSAttitude::Clone() const
{
   std::unique_ptr<CCSDSAttitude> value(new CCSDSAttitude(*this));
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
 */
//------------------------------------------------------------------------------
void CCSDSAttitude::ComputeCosineMatrixAndAngularVelocity(Real atTime)
{
   #ifdef DEBUG_CCSDS_ATTITUDE
      MessageInterface::ShowMessage("Entering CCSDSAttitude::Compute\n");
   #endif
   if (!isInitialized || needsReinit)  Initialize();

   dcm = reader->GetState(atTime);
   // Currently, no angular velocity is computed for this attitude
   #ifdef DEBUG_CCSDS_ATTITUDE
      MessageInterface::ShowMessage("EXITing CCSDSAttitude::Compute\n");
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
*/
//------------------------------------------------------------------------------
void CCSDSAttitude::ComputeCosineMatrixAndAngularVelocity(GmatTime &atTime)
{
   ///@todo: it needs to implement code for a high precision of time
   ComputeCosineMatrixAndAngularVelocity(atTime.GetMjd());
}


//------------------------------------------------------------------------------
//  private methods
//------------------------------------------------------------------------------
// none

// Reset/copy through the inherited polymorphic contract without slicing.
void CCSDSAttitude::Copy(const GmatBase *original)
{
   const auto *source = dynamic_cast<const CCSDSAttitude*>(original);
   if (!source || typeid(*source) != typeid(CCSDSAttitude) ||
       typeid(*this) != typeid(CCSDSAttitude))
      throw GmatBaseException("Cannot copy incompatible object into CCSDSAttitude");
   if (source == this)
      return;
   const auto previousContext = GetFunctionValueContext();
   operator=(*source);
   RebindFunctionValueContext(*source);
}
