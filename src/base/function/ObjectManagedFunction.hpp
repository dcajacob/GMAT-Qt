//$Id$
//------------------------------------------------------------------------------
//                                ObjectManagedFunction
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
// number NNG04CC06P.
//
// Author: Linda Jun
// Created: 2016.10.20
//
/**
 * Defines the ObjectManagedFunction base class for BuiltinGmatFunction and
 * UserDefinedFunction
 */
//------------------------------------------------------------------------------
#ifndef ObjectManagedFunction_hpp
#define ObjectManagedFunction_hpp

#include "Function.hpp"
#include "ElementWrapper.hpp"
#include "ObjectInitializer.hpp"
#include "Validator.hpp"
#include <map>

/**
 * All function classes using function object store are derived from this base class.
 */
class GMAT_API ObjectManagedFunction : public Function
{
public:
   ObjectManagedFunction(const std::string &typeStr, const std::string &nomme);
   virtual ~ObjectManagedFunction();
   ObjectManagedFunction(const ObjectManagedFunction &f);
   ObjectManagedFunction& operator=(const ObjectManagedFunction &f);
   
   virtual bool         Initialize(ObjectInitializer *objInit,
                                   bool reinitialize = false,
                                   bool resetLocalObjects = true);
   virtual bool         Execute(ObjectInitializer *objInit, bool reinitialize = false);
   virtual void         Finalize(bool cleanUp = false);
   std::unique_ptr<FunctionInvocationState> SuspendInvocationState() override;
   void RetireInvocationInterfaces() noexcept override;
   
   virtual void         SetFunctionObjectMap(ObjectMap *objMap);
   virtual ObjectMap*   GetFunctionObjectStore();
   void RetireFunctionObjectStore(ObjectMap *store) noexcept;
   void RetireArgumentWrappers(const WrapperMap &inputs,
         const WrapperArray &outputs) noexcept;
   virtual void         SetGlobalObjectMap(ObjectMap *objMap);
   // Active binding, or the last bound global view while its actual registered
   // owner is alive. A retained view does not keep invocation resources alive.
   const ObjectMap*     GetGlobalObjectStore() const;
   // Completed-call observation only; both the actual scope owner and solar
   // system must still be alive. This never reinstalls an invocation service.
   const SolarSystem*   GetObservedSolarSystem() const;
   void                 ClearObservedSolarSystem();
   
   virtual bool         SetInputElementWrapper(const std::string &forName,
                                               ElementWrapper *wrapper);
   virtual ElementWrapper* GetOutputArgument(Integer argNumber);
   virtual ElementWrapper* GetOutputArgument(const std::string &byName);
   virtual WrapperArray&   GetWrappersToDelete();
   // Release owned outputs once, retaining formal names and borrowed inputs.
   void                 ClearOutputWrappers() noexcept;
   virtual void            ClearInOutArgMaps(bool deleteInputs, bool deleteOutputs);

   virtual void            SetChangingInputType(bool hasChangingInput);
   virtual bool            HasChangingInputType();
   
   // Inherited methods from GmatBase
   virtual bool         TakeAction(const std::string &action,
                                   const std::string &actionData = "");
   
   virtual bool         SetStringParameter(const Integer id, 
                                           const char *value);
   virtual bool         SetStringParameter(const Integer id, 
                                           const std::string &value);
   virtual bool         SetStringParameter(const std::string &label,
                                           const char *value);
   virtual bool         SetStringParameter(const std::string &label,
                                           const std::string &value);
   
   DEFAULT_TO_NO_CLONES
   DEFAULT_TO_NO_REFOBJECTS

protected:
   // Sandbox and manager-registered maps have actual weak owners. An empty predicate
   // denotes an unregistered map whose lifetime remains the caller's contract.
   static std::function<bool()> CaptureObjectStoreLifetime(ObjectMap *store);

   void RebindObjectManagedFunctionValue(const ObjectManagedFunction &source,
         const std::function<GmatBase*(GmatBase*)> &resolve);
   
   /// Function input name and element wrapper map
   WrapperMap           inputArgMap;
   /// Function output name element wrapper map
   WrapperMap           outputArgMap;
   /// Object store for the Function 
   ObjectMap            *objectStore;
   /// Object store obtained from the Sandbox
   ObjectMap            *globalObjectStore;
   /// Old wrappers to delete
   WrapperArray         wrappersToDelete;
   /// Whether object store has changing input types
   bool                 changingInputType;

   std::shared_ptr<ExpressionScopeCopies> objectBindings;
   std::vector<ElementWrapper::InvocationLease> invocationWrapperLeases;
   void PruneInvocationWrapperLeases() noexcept;
  
   // for debug
   void ShowObjectMap(ObjectMap *objMap, const std::string &title = "",
                      const std::string &mapName = "");
};


#endif // ObjectManagedFunction_hpp
