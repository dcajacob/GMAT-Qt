//$Id$
//------------------------------------------------------------------------------
//                           FormationInterface
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
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
// number NNG04CI63P
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: 2013/01/09
/**
 * Proxy code for Formation objects, used to define interfaces implemented in 
 * the Formation plugin. 
 */
//------------------------------------------------------------------------------

#ifndef FormationInterface_hpp
#define FormationInterface_hpp

#include "SpaceObject.hpp"
#include <functional>

/**
 * Interface definition for Formations, implemented in libFormation.
 */
// A prepared layout owns all allocations and retained owners before Apply.
// Applying it swaps native storage only and does not initialize propagation.
class GMAT_API FormationRestorationPlan : public NativeRestorationPlan
{
public:
   ~FormationRestorationPlan() override = default;
};

class GMAT_API FormationInterface : public SpaceObject
{
public:
   FormationInterface(UnsignedInt typeId, const std::string &typeStr,
             const std::string &instName);
   virtual ~FormationInterface();
   FormationInterface(const FormationInterface& fi);
   FormationInterface& operator=(const FormationInterface& fi);

   virtual void         BuildState() = 0;
   virtual void         UpdateElements() = 0;
   virtual void         UpdateState() = 0;
   // Reconnect a completed native value copy without rebuilding its packed
   // state or applying member values. Implemented across the plugin boundary.
   virtual void RebindClonedReferences(const FormationInterface &source,
         const std::function<GmatBase*(GmatBase*)> &resolve, bool bindMembers = true);
   virtual std::unique_ptr<FormationRestorationPlan> PrepareRestorationLayout(
         const FormationInterface &snapshot, const ObjectArray &members);
   virtual GmatBase *CloneRestorationProbe() const;

};

#endif // FormationInterface_hpp
