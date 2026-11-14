//$Id$
//------------------------------------------------------------------------------
//                                  Barycenter
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number S-67573-G
//
// Author: Wendy C. Shoan
// Created: 2005.04.05
//
/**
 * Definition of the Barycenter class.
 *
 */
//------------------------------------------------------------------------------

#ifndef Barycenter_hpp
#define Barycenter_hpp

#include "gmatdefs.hpp"
#include "GmatBase.hpp"
#include "CalculatedPoint.hpp"
#include "A1Mjd.hpp"
#include "Rmatrix.hpp"
#include "Rvector6.hpp"
#include "TimeTypes.hpp"

/**
 * Barycenter class, derived from CalculatedPoint class.
 *
 * @note Bodies are sent to an object of this class via the SetRefObject
 *       method.
 */
class GMAT_API Barycenter : public CalculatedPoint
{
public:
   // default constructor, specifying calculated point type and name
   Barycenter(const std::string &itsName = "");
   // copy constructor
   Barycenter(const Barycenter &bary);
   // operator=
   Barycenter& operator=(const Barycenter &bary);
   // destructor
   virtual ~Barycenter();
   
   virtual GmatBase*       Clone() const;
   virtual void            Copy(const GmatBase* orig);
   
   // methods inherited from SpacePoint, that must be implemented here
   virtual const Rvector6  GetMJ2000State(const A1Mjd &atTime);
   virtual const Rvector3  GetMJ2000Position(const A1Mjd &atTime);
   virtual const Rvector3  GetMJ2000Velocity(const A1Mjd &atTime);
   const Rvector3 GetMJ2000Acceleration(const A1Mjd &atTime) override;
   const Rvector3 GetMJ2000Acceleration(const Real atTime) override { return GetMJ2000Acceleration(GmatTime(atTime)); }
   const Rvector3 GetMJ2000Acceleration(const GmatTime &atTime) override;

   virtual const Rvector6 GetMJ2000State(const Real atTime) { return GetMJ2000State(A1Mjd(atTime)); };
   virtual const Rvector3 GetMJ2000Position(const Real atTime) { return GetMJ2000Position(A1Mjd(atTime)); };
   virtual const Rvector3 GetMJ2000Velocity(const Real atTime) { return GetMJ2000Velocity(A1Mjd(atTime)); };
   
   virtual const Rvector6  GetMJ2000State(const GmatTime &atTime);
   virtual const Rvector3  GetMJ2000Position(const GmatTime &atTime);
   virtual const Rvector3  GetMJ2000Velocity(const GmatTime &atTime);

   virtual bool            SetRefObject(GmatBase *obj,
                                        const UnsignedInt type,
                                        const std::string &name = "");

   virtual Real            GetMass();
   virtual bool            Initialize();

   virtual StringArray     GetBuiltInNames();

   // The calculation source is independent of the built-in resource's
   // protection flag, so an editable copy can retain the native SSB provider.
   bool                    UsesSolarSystemEphemeris() const;
   void                    MakeUserDefinedCopy();

   using CalculatedPoint::IsParameterReadOnly;
   using CalculatedPoint::GetStringParameter;
   using CalculatedPoint::SetStringParameter;
   using CalculatedPoint::GetStringArrayParameter;
   using CalculatedPoint::GetPropertyEnumStrings;
   std::string             GetParameterText(const Integer id) const override;
   Integer                 GetParameterID(const std::string &str) const override;
   Gmat::ParameterType     GetParameterType(const Integer id) const override;
   bool                    IsParameterReadOnly(const Integer id) const override;
   bool                    IsParameterCommandModeSettable(const Integer id) const override;
   std::string             GetStringParameter(const Integer id) const override;
   std::string             GetStringParameter(const std::string &label) const override;
   bool                    SetStringParameter(const Integer id, const std::string &value) override;
   bool                    SetStringParameter(const Integer id, const std::string &value,
                                               const Integer index) override;
   const StringArray&      GetStringArrayParameter(const Integer id) const override;
   const StringArray&      GetPropertyEnumStrings(const Integer id) const override;
   const StringArray&      GetRefObjectNameArray(const UnsignedInt type) override;
   bool                    TakeRequiredAction(const Integer id) override;

protected:

   enum
   {
      EPHEMERIS_SOURCE = CalculatedPointParamCount,
      BarycenterParamCount
   };

   /// If this is a built-in point, this is the SpacePoint for it
   SpacePoint                  *builtInSP;
    
private:
   SpacePoint* GetNativePoint();
   // check that all bodies in the list are CelestialBody objects
   virtual void CheckBodies();

};
#endif // Barycenter_hpp

