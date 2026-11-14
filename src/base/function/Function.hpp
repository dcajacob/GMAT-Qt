//$Id$
//------------------------------------------------------------------------------
//                                   Function
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
// Author: Allison Greene
// Created: 2004/09/22
//
/**
 * Defines the Function base class used for Matlab and Gmat functions.
 */
//------------------------------------------------------------------------------
#ifndef Function_hpp
#define Function_hpp

#include "GmatBase.hpp"
#include "SolarSystem.hpp"
#include "CoordinateSystem.hpp"
#include "PhysicalModel.hpp"
#include "ObjectInitializer.hpp"
#include "Rvector6.hpp"
#include <memory>
#include <tuple>
#include <utility>

struct ExpressionScopeCopies;

// An invocation owns its saved fields; restoration never evaluates the model.
// A derived layer restores before its enclosing layer releases dependencies.
class GMAT_API FunctionInvocationState
{
public:
   FunctionInvocationState(GmatBase &owner, std::function<void()> restore,
         std::unique_ptr<FunctionInvocationState> enclosing = {},
         bool restoreEnclosingFirst = false);
   ~FunctionInvocationState() noexcept;
   bool Restore() noexcept;
   FunctionInvocationState(const FunctionInvocationState&) = delete;
   FunctionInvocationState& operator=(const FunctionInvocationState&) = delete;
private:
   friend class SharedSubscriberBindings;
   // Binding scopes can commit an explicit resource value instead of restoring
   // old bindings. Keep captured owners until ordinary scope retirement.
   void Dismiss() noexcept;
   GmatBase *owner;
   std::weak_ptr<const void> identity;
   std::function<void()> restore;
   std::unique_ptr<FunctionInvocationState> enclosing;
   bool active = true;
   bool restoredSuccessfully = true;
   bool restoreEnclosingFirst;
};

namespace FunctionInvocationDetail
{
template<class T> void Swap(T &value, T &saved) noexcept(noexcept(std::swap(value, saved)))
{
   std::swap(value, saved);
}
inline void Swap(Rvector6 &value, Rvector6 &saved) noexcept
{
   for (Integer i = 0; i < 6; ++i) std::swap(value(i), saved(i));
}
template<class Locations, class Values, size_t... I>
void Restore(Locations &locations, Values &saved, std::index_sequence<I...>) noexcept
{
   static_assert((noexcept(Swap(*std::get<I>(locations), std::get<I>(saved))) && ...),
         "Invocation restoration must not allocate or throw");
   (Swap(*std::get<I>(locations), std::get<I>(saved)), ...);
}
}

/**
 * All function classes are derived from this base class.
 */
class GMAT_API Function : public GmatBase
{
public:
   static constexpr unsigned int NativeValueProviderDepth = GmatBase::NativeValueProviderDepth + 1;

   Function(const std::string &typeStr, const std::string &nomme);
   virtual ~Function();
   Function(const Function &f);
   Function&            operator=(const Function &f);
   
   virtual void         SetCallDescription(const std::string &desc);
   
   virtual WrapperTypeArray
                        GetOutputTypes(IntegerArray &rowCounts,
                                       IntegerArray &colCounts) const;
   virtual void         SetOutputTypes(WrapperTypeArray &outputTypes,
                                       IntegerArray &rowCounts,
                                       IntegerArray &colCounts);
   
   virtual void         SetErrorFound(bool errFlag);
   virtual bool         ErrorFound();
   
   virtual std::string  GetFunctionPathAndName();
   void                 ValidateSourceFilePolicy(const std::string &path,
                                                  bool includeFile = false) const;
   
   virtual bool         IsNewFunction();
   virtual void         SetNewFunction(bool flag);
   
   virtual bool         Initialize(ObjectInitializer *objInit, bool reinitialize = false);
   virtual bool         Execute(ObjectInitializer *objInit, bool reinitialize = false);
   virtual void         Finalize(bool cleanUp = false);
   
   virtual void         SetSolarSystem(SolarSystem *ss);
   const SolarSystem*   GetSolarSystem() const;
   virtual void         SetInternalCoordSystem(CoordinateSystem *cs);
   virtual void         SetTransientForces(std::vector<PhysicalModel*> *tf);
   
   virtual bool         IsParameterReadOnly(const Integer id) const;
   virtual bool         IsParameterCloaked(const Integer id) const;
   virtual bool         IsParameterCommandModeSettable(const Integer id) const;
   virtual const StringArray& GetPropertyEnumStrings(const Integer id) const;
   virtual bool         GetBooleanParameter(const Integer id) const;
   virtual bool         GetBooleanParameter(const std::string &label) const;
   virtual bool         SetBooleanParameter(const Integer id, const bool value);
   virtual bool         SetBooleanParameter(const std::string &label, const bool value);
   virtual std::string  GetParameterText(const Integer id) const;
   virtual Integer      GetParameterID(const std::string &str) const;
   virtual Gmat::ParameterType
                        GetParameterType(const Integer id) const;
   virtual std::string  GetParameterTypeString(const Integer id) const;
   virtual std::string  GetStringParameter(const Integer id) const;
   virtual std::string  GetStringParameter(const std::string &label) const;
   virtual std::string  GetStringParameter(const Integer id,
                                           const Integer index) const;
   virtual std::string  GetStringParameter(const std::string &label,
                                           const Integer index) const;
   virtual bool         SetStringParameter(const Integer id, 
                                           const char *value);
   virtual bool         SetStringParameter(const Integer id, 
                                           const std::string &value);
   virtual bool         SetStringParameter(const std::string &label,
                                           const char *value);
   virtual bool         SetStringParameter(const std::string &label,
                                           const std::string &value);
   virtual const StringArray&
                        GetStringArrayParameter(const Integer id) const;
   virtual const StringArray&
                        GetStringArrayParameter(const std::string &label) const;
   
   virtual void         SetOutputName(const std::string &outputName);

   // Definition-owned components must be registered before passive references
   // bind. Native invocation stores/wrappers are rebuilt by FunctionManager.
   virtual bool HasFunctionValueReferenceProvider() const;
   using OutputObjectInitializer = std::function<void()>;
   OutputObjectInitializer ExchangeOutputObjectInitializer(OutputObjectInitializer initializer);
   std::unique_ptr<FunctionInvocationState> EnterInvocation();
   bool HasActiveInvocation() const noexcept { return activeInvocationDepth != 0; }
   virtual std::unique_ptr<FunctionInvocationState> SuspendInvocationState();
   virtual void RetireInvocationInterfaces() noexcept;
   virtual void PrepareClonedFunctionValue(const Function &source);
   ObjectArray GetFunctionValueOwnedObjectsImpl() const;
   virtual void GetClonedOwnedObjects(const Function &target,
         ObjectArray &sources, ObjectArray &copies) const;

   DEFAULT_TO_NO_CLONES
   DEFAULT_TO_NO_REFOBJECTS

protected:
   template<class... Fields>
   std::unique_ptr<FunctionInvocationState> CaptureInvocationFields(
         std::unique_ptr<FunctionInvocationState> enclosing, Fields &...fields)
   {
      auto saved = std::make_shared<std::tuple<Fields...>>(fields...);
      auto restore = [locations = std::make_tuple(&fields...), saved]() mutable noexcept
      {
         FunctionInvocationDetail::Restore(locations, *saved,
               std::index_sequence_for<Fields...>{});
      };
      return std::make_unique<FunctionInvocationState>(*this, std::move(restore),
            std::move(enclosing));
   }

   /// Call description to be written out in error message
   std::string          callDescription;
   /// Fully-qualified path for function script
   std::string          functionPath;
   /// Function name
   std::string          functionName;
   /// Native parser behavior or strict seven-bit ASCII function sources
   std::string          sourceFormat;
   /// Require brackets on standalone calls that assign outputs
   bool                 requireOutputBrackets;
   /// Function input names
   StringArray          inputNames;
   /// Function output names
   StringArray          outputNames;
   /// Output wrapper type array
   WrapperTypeArray     outputWrapperTypes;
   
   /// Output row count used for returning one Array type
   IntegerArray         outputRowCounts;
   /// Output column count used for returning one Array type
   IntegerArray         outputColCounts;
   // Invocation services are never copied with a function definition/value.
   OutputObjectInitializer outputObjectInitializer;
   bool notifyingOutputObjects = false;
   unsigned int activeInvocationDepth = 0;
   
   /// Solar System, set by the local Sandbox, to pass to the function
   SolarSystem          *solarSys;
   /// Internal CS, set by the local Sandbox, to pass to the function
   CoordinateSystem     *internalCoordSys;
   /// transient forces to pass to the function
   std::vector<PhysicalModel *> *forces;
   
   /// the flag indicating error found in function, this flag is set by Interpreter
   bool                 errorFound;

   std::shared_ptr<ExpressionScopeCopies> functionBindings;
   void RebindFunctionEnvironment(const Function &source,
         const std::function<GmatBase*(GmatBase*)> &resolve);
   
   enum
   {
      FUNCTION_PATH = GmatBaseParamCount,
      FUNCTION_NAME,
      FUNCTION_INPUT,
      FUNCTION_OUTPUT,
      FUNCTION_SOURCE_FORMAT,
      REQUIRE_OUTPUT_BRACKETS,
      FunctionParamCount  ///< Count of the parameters for this class
   };
   
   static const std::string
      PARAMETER_TEXT[FunctionParamCount - GmatBaseParamCount];
   static const Gmat::ParameterType
      PARAMETER_TYPE[FunctionParamCount - GmatBaseParamCount];
   
};


#endif // Function_hpp
