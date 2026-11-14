//------------------------------------------------------------------------------
//                           EquationWrapper
//------------------------------------------------------------------------------
// GMAT:  General Mission Analysis Tool
// CSALT: Collocation Stand Alone Library and Toolkit
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under Purchase
// Order NNG16LD52P
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: Jun 14, 2018
/**
 * Wrapper for (RHS) equations
 */
//------------------------------------------------------------------------------

#include "EquationWrapper.hpp"
#include "ExpressionBinding.hpp"
#include "MathTree.hpp"
#include "MessageInterface.hpp"
#include "StringUtil.hpp"
#include "MathException.hpp"
#include <memory>
#include <typeinfo>


//#define DEBUG_INITALIZE
//#define DEBUG_EVALUATE


EquationWrapper::EquationWrapper(const std::string &eq) :
   ElementWrapper(), theEquation(eq), dataType(Gmat::UNKNOWN_PARAMETER_TYPE),
   resultant(nullptr), configObjectMap(nullptr), hasEvaluated(false),
   isEquationInitialized(false)
{
   wrapperType = Gmat::EQUATION_WT;
   description = eq;
}


EquationWrapper::~EquationWrapper()
{
   ClearResult();
   // Newer definitions can borrow values from older ones. Retire dependents
   // before their owners rather than relying on vector destruction order.
   theTree.Reset();
   while (!retainedDefinitions.empty()) retainedDefinitions.pop_back();
}


EquationWrapper::EquationWrapper(const EquationWrapper& ew) :
   ElementWrapper(ew), theEquation(ew.theEquation), dataType(ew.dataType),
   theTree(ew.theTree), resultant(nullptr), refObjects(ew.refObjects),
   configObjectMap(ew.configObjectMap), equationObjectMap(ew.equationObjectMap),
   hasEvaluated(false), isEquationInitialized(false)
{
   ExpressionBindingScope definition(false, true);
   for (const auto &previous : ew.retainedDefinitions)
      retainedDefinitions.emplace_back(new RHSEquation(*previous));
   BindActualReferences(ew, [](GmatBase *object) { return object; });
   // Native copies retain cold definitions; explicit retained binding may
   // preserve already prepared source association without initialization.
}


EquationWrapper& EquationWrapper::operator=(const EquationWrapper& ew)
{
   if (this == &ew) return *this;
   EquationWrapper prepared(ew);
   ElementWrapper::operator=(ew);
   theTree.Swap(prepared.theTree);
   retainedDefinitions.swap(prepared.retainedDefinitions);
   theEquation.swap(prepared.theEquation);
   refObjects.swap(prepared.refObjects);
   equationObjectMap.swap(prepared.equationObjectMap);
   std::swap(configObjectMap, prepared.configObjectMap);
   dataType = ew.dataType;
   isEquationInitialized = false;
   ClearResult();
   return *this;
}


ElementWrapper* EquationWrapper::Clone() const
{
   if (typeid(*this) != typeid(EquationWrapper))
      throw MathException("Derived EquationWrapper must implement its native Clone.\n");
   return new EquationWrapper(*this);
}


void EquationWrapper::SetEquation(const std::string &eq)
{
   if (eq == theEquation) return;
   std::string expression = eq;
   std::unique_ptr<RHSEquation> previous(new RHSEquation);
   retainedDefinitions.reserve(retainedDefinitions.size() + 1);
   isEquationInitialized = false;
   ClearResult();
   theTree.Swap(*previous);
   retainedDefinitions.push_back(std::move(previous));
   theEquation.swap(expression);
   description = theEquation;
   dataType = Gmat::UNKNOWN_PARAMETER_TYPE;
   refObjects.clear();
   refObjectNames.clear();
}

//------------------------------------------------------------------------------
//  void SetObjectMap(ObjectMap *objMap)
//------------------------------------------------------------------------------
/**
 * Called by the Interpreter to set the local resource store used by the
 * GmatCommand for InterpretAction()
 *
 * @param map Pointer to the local object map
 */
//------------------------------------------------------------------------------
void EquationWrapper::SetConfiguredObjectMap(ObjectMap *map)
{
   configObjectMap = map;
}


void EquationWrapper::ClearRefObjectNames()
{
   refObjects.clear();
   refObjectNames.clear();
   equationObjectMap.clear();
   isEquationInitialized = false;
   ClearResult();
}

const StringArray& EquationWrapper::GetRefObjectNames()
{
   ValidateOwnedWrapperGraph();
   refObjects.clear();
   if (theEquation.empty()) return refObjects;
   if (!theTree.GetMathTree(false) && !ConstructTree()) return refObjects;
   theTree.GetWrapperObjectNames();
   for (const auto &entry : theTree.GetMathWrapperMap())
   {
      const StringArray refs = entry.second ? entry.second->GetRefObjectNames() : StringArray{entry.first};
      for (const std::string &name : refs)
         if (std::find(refObjects.begin(), refObjects.end(), name) == refObjects.end())
            refObjects.push_back(name);
   }
   return refObjects;
}

bool EquationWrapper::SetRefObjectName(const std::string &name, Integer index)
{
   const StringArray refs = GetRefObjectNames();
   if (name.empty() || index < 0 || static_cast<size_t>(index) >= refs.size()) return false;
   return refs[index] == name || RenameObject(refs[index], name);
}

GmatBase* EquationWrapper::GetRefObject(const std::string &name)
{
   if (name.empty())
      return equationObjectMap.size() == 1 ? equationObjectMap.begin()->second : nullptr;
   const auto found = equationObjectMap.find(name);
   if (found != equationObjectMap.end()) return found->second;
   for (const auto &entry : theTree.GetMathWrapperMap())
      if (entry.second)
         for (const std::string &ref : entry.second->GetRefObjectNames())
            if (ref == name) return entry.second->GetRefObject(name);
   return nullptr;
}

bool EquationWrapper::SetRefObject(GmatBase *obj)
{
   if (!obj) return false;
   const auto found = equationObjectMap.find(obj->GetName());
   if (found == equationObjectMap.end() || found->second != obj)
   {
      equationObjectMap[obj->GetName()] = obj;
      isEquationInitialized = false;
      ClearResult();
   }
   return true;
}

bool EquationWrapper::RenameObject(const std::string &oldName, const std::string &newName)
{
   if (oldName.empty() || newName.empty()) return false;
   if (oldName == newName) return true;
   ObjectMap objects;
   for (const auto &entry : equationObjectMap)
      objects[GmatStringUtil::ReplaceName(entry.first, oldName, newName)] = entry.second;
   const bool renamed = theTree.RenameRefObject(Gmat::UNKNOWN_OBJECT, oldName, newName);
   theEquation = GmatStringUtil::ReplaceName(theEquation, oldName, newName);
   description = GmatStringUtil::ReplaceName(description, oldName, newName);
   equationObjectMap.swap(objects);
   refObjects.clear();
   refObjectNames.clear();
   isEquationInitialized = false;
   ClearResult();
   return renamed;
}



bool EquationWrapper::Initialize(ObjectMap *objectMap, ObjectMap *globalObjectMap)
{
   ValidateOwnedWrapperGraph();
   isEquationInitialized = false;
   ClearResult();
   if (theEquation.empty()) return false;
   if (!theTree.GetMathTree(false) &&
         !theTree.BuildExpression(theEquation, configObjectMap ? configObjectMap : objectMap, true))
      return false;
   theTree.GetWrapperObjectNames();
   if (!theTree.Initialize(objectMap, globalObjectMap)) return false;
   Integer type, rows, columns;
   theTree.GetMathTree(false)->GetOutputInfo(type, rows, columns);
   ObjectMap objects = equationObjectMap;
   for (const auto &entry : theTree.GetMathWrapperMap())
      if (entry.second)
         for (const std::string &name : entry.second->GetRefObjectNames())
            if (GmatBase *object = entry.second->GetRefObject(name)) objects[name] = object;
   equationObjectMap.swap(objects);
   dataType = static_cast<Gmat::ParameterType>(type);
   isEquationInitialized = true;
   return true;
}

bool EquationWrapper::Initialize()
{
   return Initialize(&equationObjectMap, nullptr);
}

//------------------------------------------------------------------------------
// bool ConstructTree()
//------------------------------------------------------------------------------
/**
 * Parse the equation string and build the tree
 *
 * @return true is an equation was set up
 */
//------------------------------------------------------------------------------
bool EquationWrapper::ConstructTree()
{
   isEquationInitialized = false;
   ClearResult();
   const bool built = theTree.BuildExpression(theEquation, configObjectMap, true);
   if (built) theTree.GetWrapperObjectNames();
   return built;
}

//------------------------------------------------------------------------------
// RHSEquation *GetTree()
//------------------------------------------------------------------------------
/**
 * Access the equation tree container
 *
 * @return The RHS pointer
 */
//------------------------------------------------------------------------------
RHSEquation *EquationWrapper::GetTree()
{
   return &theTree;
}

//------------------------------------------------------------------------------
//  Gmat::ParameterType  GetDataType() const
//------------------------------------------------------------------------------
/**
 * This method returns the data type for the ElementWrapper object.
 *
 * @return value type for the object.
 *
 */
//------------------------------------------------------------------------------
Gmat::ParameterType EquationWrapper::GetDataType() const
{
   return dataType;
}

//------------------------------------------------------------------------------
// bool EvaluateEquation()
//------------------------------------------------------------------------------
/**
 * Evaluate the equation to generate the data for teh wrapper to return
 *
 * @return true if the equation fires successfully
 */
//------------------------------------------------------------------------------
bool EquationWrapper::EvaluateEquation()
{
   EvaluateCurrent();
   return true;
}


//---------------------------------------------------------------------------
//  Real EvaluateReal() const
//---------------------------------------------------------------------------
/**
 * Method to return the Real value of the wrapped object.
 *
 * @return Real value of the wrapped object.
 *
 * @note  This is a pure (abstract) method and must be implemented in
 * all leaf classes derived from this one.
 */
//---------------------------------------------------------------------------
Real EquationWrapper::EvaluateReal() const
{
   if (dataType != Gmat::REAL_TYPE)
      throw MathException("Equation does not return a real number.\n");
   return EvaluateCurrent().EvaluateRealForCalculation();
}

//---------------------------------------------------------------------------
//  bool SetReal(const Real value)
//---------------------------------------------------------------------------
/**
 * Method to set the Real value of the wrapped object.
 *
 * @return true if successful; false otherwise.
 *
 * @note  This is a pure (abstract) method and must be implemented in
 * all leaf classes derived from this one.
 */
//---------------------------------------------------------------------------
bool EquationWrapper::SetReal(const Real toValue)
{
   bool retval = false;
   return retval;
}

const Rmatrix& EquationWrapper::EvaluateArray() const
{
   if (dataType != Gmat::RMATRIX_TYPE)
      throw MathException("Equation does not return an array.\n");
   const Rmatrix &value = EvaluateCurrent().EvaluateArray();
   retMat.SetSize(value.GetNumRows(), value.GetNumColumns());
   retMat = value;
   return retMat;
}

bool EquationWrapper::SetArray(const Rmatrix &toValue)
{
   bool retval = false;
   return retval;
}


const Rvector& EquationWrapper::EvaluateRvector() const
{
   if (dataType != Gmat::RVECTOR_TYPE && dataType != Gmat::RMATRIX_TYPE)
      throw MathException("Equation does not return a vector.\n");
   ElementWrapper &result = EvaluateCurrent();
   Rvector value;
   if (result.GetDataType() == Gmat::RVECTOR_TYPE)
      value = result.EvaluateRvector();
   else
   {
      const Rmatrix &matrix = result.EvaluateArray();
      if (matrix.GetNumRows() != 1 && matrix.GetNumColumns() != 1)
         throw MathException("Equation array is not a row or column vector.\n");
      value = matrix.GetRowOrColumn();
   }
   retVec.SetSize(value.GetSize());
   retVec = value;
   return retVec;
}


bool EquationWrapper::SetRvector(const Rvector &toValue)
{
   bool retval = false;
   return retval;
}





//---------------------------------------------------------------------------
//  void SetupWrapper()
//---------------------------------------------------------------------------
/**
 * Method that parses the description string so that the reference
 * objects and related properties are identified.
 *
 * @return true if successful; false otherwise.
 *
 * @note  This is a pure virtual (abstract) method and must be implemented in
 * all leaf classes derived from this one.
 */
//---------------------------------------------------------------------------
void EquationWrapper::SetupWrapper()
{
   SetEquation(description);
}

void EquationWrapper::ClearResult() const
{
   delete resultant;
   resultant = nullptr;
   hasEvaluated = false;
}

ElementWrapper& EquationWrapper::EvaluateCurrent() const
{
   if (!isEquationInitialized)
      throw MathException("Equation must be initialized before evaluation.\n");
   ValidateOwnedWrapperGraph();
   hasEvaluated = false;
   std::unique_ptr<ElementWrapper> value(theTree.RunMathTree(nullptr));
   if (!value || value->GetDataType() != dataType)
      throw MathException("Equation result does not match its declared type.\n");
   ClearResult();
   resultant = value.release();
   hasEvaluated = true;
   return *resultant;
}

std::string EquationWrapper::EvaluateString() const
{
   if (dataType != Gmat::STRING_TYPE)
      throw MathException("Equation does not return a string.\n");
   return EvaluateCurrent().EvaluateString();
}

GmatBase* EquationWrapper::EvaluateObject() const
{
   if (dataType != Gmat::OBJECT_TYPE)
      throw MathException("Equation does not return an object.\n");
   // Valid until the next evaluation, definition/binding change or destruction.
   return EvaluateCurrent().EvaluateObject();
}

void EquationWrapper::RebindClonedReferences(const ElementWrapper &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   CheckCloneType(source);
   BindActualReferences(static_cast<const EquationWrapper&>(source), resolve);
}

void EquationWrapper::BindActualReferences(const EquationWrapper &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   isEquationInitialized = false;
   ClearResult();
   ValidateOwnedWrapperGraph();
   ExpressionBindingScope binding;
   ObjectArray originals, copies;
   source.GetClonedOwnedObjects(*this, originals, copies);
   auto actual = [&](GmatBase *object) -> GmatBase*
   {
      // The enclosing native owner/scene may have already chosen a canonical
      // copy shared by several expressions. Preserve that choice before using
      // this definition's own freshly cloned values.
      GmatBase *mapped = resolve(object);
      if (mapped && mapped != object) return mapped;
      const auto found = std::find(originals.begin(), originals.end(), object);
      return found == originals.end() ? mapped : copies[found - originals.begin()];
   };
   auto *configured = binding.scopes->CopyMap(source.configObjectMap, actual);
   ObjectMap objects;
   for (const auto &entry : source.equationObjectMap)
      objects.emplace(entry.first, entry.second ? actual(entry.second) : nullptr);
   for (size_t i = 0; i < retainedDefinitions.size(); ++i)
      retainedDefinitions[i]->RebindClonedReferences(*source.retainedDefinitions[i], actual);
   theTree.RebindClonedReferences(source.theTree, actual);
   configObjectMap = configured;
   equationObjectMap.swap(objects);
   dataType = source.dataType;
   isEquationInitialized = ExpressionBindingScope::preserveReadiness && source.isEquationInitialized;
}

std::vector<RHSEquation*> EquationWrapper::GetOwnedDefinitions() const
{
   std::vector<RHSEquation*> definitions{&theTree};
   for (const auto &definition : retainedDefinitions) definitions.push_back(definition.get());
   return definitions;
}

void EquationWrapper::GetClonedOwnedObjects(const EquationWrapper &target,
      ObjectArray &originals, ObjectArray &copies) const
{
   GetClonedOwnedObjects(target, originals, copies, true);
   GetClonedOwnedObjects(target, originals, copies, false);
}

void EquationWrapper::GetClonedOwnedObjects(const EquationWrapper &target,
      ObjectArray &originals, ObjectArray &copies, bool sourceOwnedOnly) const
{
   ValidateOwnedWrapperGraph();
   target.ValidateOwnedWrapperGraph();
   const auto from = GetOwnedDefinitions();
   const auto to = target.GetOwnedDefinitions();
   if (from.size() != to.size()) throw MathException("Missing copied equation definition owners.\n");
   for (size_t i = 0; i < from.size(); ++i)
      from[i]->GetClonedOwnedObjects(*to[i], originals, copies, sourceOwnedOnly);
}
