//------------------------------------------------------------------------------
//                           RealRHS
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under the FDSS II
// contract, Task Order 08
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: May 31, 2018
/**
 * Equation parser for Real number data
 */
//------------------------------------------------------------------------------

#include "RHSEquation.hpp"
#include "ExpressionBinding.hpp"

#include "MathParser.hpp"
#include "MathTree.hpp"
#include "StringUtil.hpp"
#include "CommandException.hpp"

// Wrappers and wrapped types
#include "ElementWrapper.hpp"
#include "NumberWrapper.hpp"
#include "ArrayWrapper.hpp"
#include "RvectorWrapper.hpp"
#include "ObjectWrapper.hpp"
#include "StringWrapper.hpp"
#include "Array.hpp"
#include "FunctionManager.hpp"
#include "EquationWrapper.hpp"
#include <set>
#include <memory>
#include <utility>
#include <exception>

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif

//#define DEBUG_RUN_MATH_TREE
//#define DEBUG_WRAPPER_CODE

#include "MessageInterface.hpp"

namespace
{
struct RHSWrapperCopies
{
   WrapperMap values;
   std::vector<std::unique_ptr<ElementWrapper>> owned;
   void Release() { for (auto &value : owned) value.release(); }
};

RHSWrapperCopies CopyRHSWrappers(const WrapperMap &source)
{
   RHSWrapperCopies result;
   std::map<const ElementWrapper*, ElementWrapper*> copies;
   std::set<const ElementWrapper*> forbidden;
   for (const auto &entry : source) if (entry.second) forbidden.insert(entry.second);
   GmatBase::FunctionValueCopyGuard copying;
   for (const auto &entry : source)
   {
      if (!entry.second) { result.values.emplace(entry.first, nullptr); continue; }
      auto found = copies.find(entry.second);
      if (found == copies.end())
      {
         auto copy = ElementWrapper::CloneValue(entry.second);
         if (forbidden.count(copy.get()))
         {
            copy.release();
            throw CommandException("Equation wrapper Clone returned a source/peer.\n");
         }
         ElementWrapper *value = copy.get();
         result.owned.push_back(std::move(copy));
         copies.emplace(entry.second, value);
         forbidden.insert(value);
         found = copies.find(entry.second);
      }
      result.values.emplace(entry.first, found->second);
   }
   // Register owned values before reconnecting borrowed aliases between wrappers.
   ObjectArray originals, values;
   for (const auto &entry : copies)
      entry.second->CollectClonedOwnedReferences(*entry.first, originals, values, true);
   for (const auto &entry : copies)
      entry.second->CollectClonedOwnedReferences(*entry.first, originals, values);
   auto resolve = [&](GmatBase *object) -> GmatBase*
   {
      const auto found = std::find(originals.begin(), originals.end(), object);
      return found == originals.end() ? object : values[found - originals.begin()];
   };
   for (const auto &entry : copies)
      entry.second->RebindClonedReferences(*entry.first, resolve);
   return result;
}
}

//------------------------------------------------------------------------------
// RealRHS()
//------------------------------------------------------------------------------
/**
 * Constructor
 */
//------------------------------------------------------------------------------
RHSEquation::RHSEquation() :
   rhsString      (""),
   resourceMap    (NULL),
   rhsWrapper     (NULL),
   mathTree       (NULL),
   isExpressionInitialized(false)
{
}

//------------------------------------------------------------------------------
// RealRHS()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
RHSEquation::~RHSEquation()
{
   Reset();
}

//------------------------------------------------------------------------------
// RealRHS(const RealRHS &rhs)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 *
 * @param rhs The RHC copied here.  Note that the tree is not rebuilt.
 */
//------------------------------------------------------------------------------
RHSEquation::RHSEquation(const RHSEquation &rhs) :
   rhsString(rhs.rhsString), resourceMap(rhs.resourceMap),
   rhsWrapper(nullptr), mathTree(nullptr), isExpressionInitialized(false)
{
   // Native copies are cold and must not populate a surrounding retained
   // binding pass with interim identity-only scope mappings.
   ExpressionBindingScope cloning(false, true);
   auto wrappers = CopyRHSWrappers(rhs.mathWrapperMap);
   std::unique_ptr<MathTree> tree(rhs.mathTree ? new MathTree(*rhs.mathTree) : nullptr);
   mathWrapperMap.swap(wrappers.values);
   if (tree) tree->SetMathWrappers(&mathWrapperMap);
   mathTree = tree.get();
   {
      ExpressionBindingScope complete(false, true);
      RebindClonedReferences(rhs, [](GmatBase *object) { return object; });
   }
   mathTree = tree.release();
   wrappers.Release();
}

//------------------------------------------------------------------------------
// RealRHS& operator=(const RealRHS &rhs)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 *
 * @param The RHC copied here.  Note that the tree is not rebuilt.
 *
 * @return this RealRHS, set with the rhs string.
 */
//------------------------------------------------------------------------------
RHSEquation& RHSEquation::operator=(const RHSEquation &rhs)
{
   if (this == &rhs) return *this;
   RHSEquation prepared(rhs);
   Swap(prepared);
   return *this;
}


//------------------------------------------------------------------------------
// std::string GetEquationString()
//------------------------------------------------------------------------------
/**
 * Returns the string that built the MathTree
 *
 * @return The equation string
 */
//------------------------------------------------------------------------------
std::string RHSEquation::GetEquationString()
{
   return rhsString;
}

//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Sets up a (Real) RHS if the rhsString has been set
 *
 * @return true if there is a configured MathTree, false if not
 */
//------------------------------------------------------------------------------
bool RHSEquation::Initialize(ObjectMap *objectMap, ObjectMap *globalObjectMap)
{
   isExpressionInitialized = false;
   bool retval = true;
   if ((rhsString != "") && (resourceMap != NULL))
      retval = BuildExpression(rhsString, resourceMap);

   #ifdef DEBUG_RUN_MATH_TREE
      MessageInterface::ShowMessage("Initializing MathTree at address %p\n",
            mathTree);
   #endif

   if (!mathTree || !mathTree->GetTopNode()) return false;

   // Set references for the math element wrappers
   std::map<std::string, ElementWrapper*>::iterator ewi;
   std::set<ElementWrapper*> associated;
   for (ewi = mathWrapperMap.begin(); ewi != mathWrapperMap.end(); ++ewi)
   {
      if (!ewi->second)
         throw CommandException("Missing equation wrapper for \"" + ewi->first + "\".\n");
      if (associated.insert(ewi->second).second)
      {
         SetWrapperReferences(*(ewi->second), objectMap, globalObjectMap);
         if (auto *equation = dynamic_cast<EquationWrapper*>(ewi->second))
            if (!equation->Initialize(objectMap, globalObjectMap))
               throw CommandException("Cannot initialize nested equation wrapper.\n");
      }
   }

   if (mathTree && retval)
   {
      retval = mathTree->Initialize(objectMap, globalObjectMap);
      mathTree->SetMathWrappers(&mathWrapperMap);

      if (retval)
      {
         try
         {
            #ifdef DEBUG_RUN_MATH_TREE
               MessageInterface::ShowMessage("   Validating\n");
            #endif

            MathNode *topNode = mathTree->GetTopNode();
            if (!topNode->ValidateInputs())
               throw CommandException("Failed to validate math equation or "
                     "function inputs");
         }
         catch (BaseException &be)
         {
            throw CommandException("Failed to validate math equation or "
                  "function inputs: " + be.GetFullMessage());
         }
      }
   }

   isExpressionInitialized = mathTree && retval;
   return isExpressionInitialized;
}


//------------------------------------------------------------------------------
// bool SetWrapperReferences(ElementWrapper *ew, ObjectMap *objectMap,
//       ObjectMap *globalObjectMap)
//------------------------------------------------------------------------------
/**
 * Updates wrappers with resource pointers to objects used in current run
 *
 * @param ew The wrapper that is updated
 * @param objectMap The local object map of resources
 * @param globalObjectMap The global object map of resources
 *
 * @return true
 */
//------------------------------------------------------------------------------
void RHSEquation::SetWrapperReferences(ElementWrapper &ew, ObjectMap *objectMap,
      ObjectMap *globalObjectMap)
{
   const StringArray refs = ew.GetRefObjectNames();
   for (const std::string &name : refs)
   {
      GmatBase *object = nullptr;
      if (objectMap)
      {
         auto found = objectMap->find(name);
         if (found != objectMap->end()) object = found->second;
      }
      if (!object && globalObjectMap)
      {
         auto found = globalObjectMap->find(name);
         if (found != globalObjectMap->end()) object = found->second;
      }
      if (!object)
         throw CommandException("Cannot resolve equation reference \"" + name + "\".\n");
      if (!ew.SetRefObject(object))
         throw CommandException("Cannot bind equation reference \"" + name + "\".\n");
   }
}


//------------------------------------------------------------------------------
// bool SetElementWrapper(ElementWrapper *toWrapper,
//                        const std::string &withName)
//------------------------------------------------------------------------------
/**
 * Sets the wrapper objects used in the equation
 *
 * @param toWrapper The element wrapper that the equation needs
 * @param withName The name of the wrapper, so it can be matched to the needed
 *                 member of the wrapper map.
 *
 * @return true if the wrapper was set
 */
//------------------------------------------------------------------------------
bool RHSEquation::SetElementWrapper(ElementWrapper *toWrapper,
                                   const std::string &withName)
{
   #ifdef DEBUG_WRAPPER_CODE
      MessageInterface::ShowMessage("Setting RHS equation element wrapper %s\n",
            withName.c_str());
   #endif

   bool retval = false;
   if (!toWrapper) return false;

   // If name found in the math wrapper map
   if (mathWrapperMap.find(withName) != mathWrapperMap.end())
   {
      #ifdef DEBUG_WRAPPER_CODE
         MessageInterface::ShowMessage("   name '%s' found in mathWrapperMap\n",
               withName.c_str());
      #endif
      // rhs should always be parameter wrapper, so check first
      // Now string wrapper is allowed such as strcat(a, ' new string') (LOJ: 2016.02.25)
      if ((withName.find(".") == withName.npos) ||
          (withName.find(".") != withName.npos &&
           (toWrapper->GetWrapperType() == Gmat::PARAMETER_WT ||
            toWrapper->GetWrapperType() == Gmat::NUMBER_WT)) ||
          (toWrapper->GetWrapperType() == Gmat::STRING_WT))
      {
         if (mathWrapperMap[withName] != toWrapper)
         {
            #ifdef DEBUG_WRAPPER_CODE
               MessageInterface::ShowMessage("   now setting equation wrapper "
                     "%s to <%p> on RHS %p\n", withName.c_str(), toWrapper,
                     this);
            #endif
            mathWrapperMap[withName] = toWrapper;
            isExpressionInitialized = false;
         }
         retval = true;
      }
   }
   return retval;
}


//------------------------------------------------------------------------------
// void ClearWrappers()
//------------------------------------------------------------------------------
/**
 * Clears the wrappers used in the equation
 */
//------------------------------------------------------------------------------
void RHSEquation::ClearWrappers()
{
   isExpressionInitialized = false;
   std::set<ElementWrapper*> wrappers;
   for (const auto &entry : mathWrapperMap)
      if (entry.second) wrappers.insert(entry.second);
   if (rhsWrapper) wrappers.insert(rhsWrapper);
   mathWrapperMap.clear();
   rhsWrapper = nullptr;
   for (ElementWrapper *wrapper : wrappers) delete wrapper;
}


//------------------------------------------------------------------------------
// void SetCallingFunction(FunctionManager *fm)
//------------------------------------------------------------------------------
/**
 * Passes Function objects to the MathTree
 *
 * @param fm The manager for the Function
 */
//------------------------------------------------------------------------------
void RHSEquation::SetCallingFunction(FunctionManager *fm)
{
   if (mathTree)
      mathTree->SetCallingFunction(fm);
}


//------------------------------------------------------------------------------
// void Assignment::SetMathWrappers()
//------------------------------------------------------------------------------
/**
 * Passes the wrappers to the math tree for use
 */
//------------------------------------------------------------------------------
void RHSEquation::SetMathWrappers()
{
   if (mathTree)
      mathTree->SetMathWrappers(&mathWrapperMap);
}


//------------------------------------------------------------------------------
// void Finalize()
//------------------------------------------------------------------------------
/**
 * Tells the MathTree that we are done using it
 */
//------------------------------------------------------------------------------
void RHSEquation::Finalize()
{
   isExpressionInitialized = false;
   if (mathTree)
      mathTree->Finalize();
}

void RHSEquation::RetireBindings()
{
   if (isRetiringBindings) return;
   isRetiringBindings = true;
   struct RetirementGuard
   {
      bool &retiring;
      ~RetirementGuard() { retiring = false; }
   } guard{isRetiringBindings};
   isExpressionInitialized = false;
   resourceMap = nullptr;
   std::exception_ptr firstError;
   try { if (mathTree) mathTree->RetireBindings(); }
   catch (...) { firstError = std::current_exception(); }
   // Keep copied scope storage alive through wrapper destruction. Nested
   // EquationWrappers own their trees/managers and retire before the FOS dies.
   ClearWrappers();
   expressionBindings.reset();
   if (firstError) std::rethrow_exception(firstError);
}


//------------------------------------------------------------------------------
// bool BuildExpression(const std::string &rhside, ObjectMap *theMap)
//------------------------------------------------------------------------------
/**
 * Code that breaks the string into a MathTree.
 *
 * This method builds a MathTree from an input string.  The string is first
 * tested to see if it defines an equation.  If so, the MathTree construction
 * is attempted.
 *
 * @param rhside  The string describing the equation
 * @param theMap  An ObjectArray containing the objects that can be used in
 *                evaluating the tree.
 * @param allowSingleElement Flag to identify a single element as an equation if
 *                the parsing is clean
 *
 * @return true if a MathTree was built, false if not (and no exception was
 *         thrown).
 */
//------------------------------------------------------------------------------
bool RHSEquation::BuildExpression(const std::string &rhside, ObjectMap *theMap,
      bool allowSingleElement)
{
   if (mathTree && rhside == rhsString)
   {
      resourceMap = theMap;
      return true;
   }
   MathParser parser(theMap);
   if (!parser.IsEquation(rhside, false, allowSingleElement)) return false;

   // The new owner is installed immediately after parsing so subsequent syntax
   // checks and metadata allocation cannot leak a successfully parsed graph.
   std::unique_ptr<MathTree> tree(new MathTree("MathTree", rhside));
   tree->SetTopNode(parser.Parse(rhside, true));
   std::string expression = rhside;
   if (GmatStringUtil::EndsWith(expression, "'"))
   {
      std::string candidate = GmatStringUtil::RemoveLastString(expression, "'");
      candidate = GmatStringUtil::RemoveAll(candidate, ' ');
      for (const std::string &name : parser.GetGmatFunctionNames())
         if (GmatStringUtil::ToUpper(candidate) == GmatStringUtil::ToUpper(name))
            throw CommandException("Found missing start quote on the right-"
                  "hand side of an Assignment command or equation");
   }
   tree->SetGmatFunctionNames(parser.GetGmatFunctionNames());
   delete mathTree;
   mathTree = nullptr;
   ClearWrappers();
   rhsString.swap(expression);
   resourceMap = theMap;
   mathTree = tree.release();
   return true;
}


//------------------------------------------------------------------------------
// bool Validate(std::string &msg)
//------------------------------------------------------------------------------
/**
 * Validator for the RHS expression
 *
 * @param msg String used for error messages
 *
 * @return true if validated, false if not
 */
//------------------------------------------------------------------------------
bool RHSEquation::Validate(std::string &msg)
{
   bool retval = false;

   if (mathTree)
      retval = mathTree->Validate(msg);
   else
      msg = "The MathTree for the right hand side has not been set";

   return retval;
}


//------------------------------------------------------------------------------
// bool ValidateStringExpression(std::string &msg)
//------------------------------------------------------------------------------
/**
 * Validator for RHS string expressions
 *
 * @param msg String used for error messages
 *
 * @return true if validated, false if not
 */
//------------------------------------------------------------------------------
bool RHSEquation::ValidateStringExpression(std::string &msg)
{
   bool retval = false;

   // Now allow string function such as strcat() (LOJ: 2016.02.18)
   MathNode *topNode = mathTree->GetTopNode();
   if (topNode && topNode->IsOfType("StringFunctionNode"))
   {
      #ifdef DEBUG_VALIDATION
      MessageInterface::ShowMessage
         ("   We now allow string function, so it is valid\n");
      #endif
      retval = true;
   }
   else
   {
      msg = "Right of the equal sign is not a valid string.";
   }

   return retval;
}

//------------------------------------------------------------------------------
// WrapperMap GetMathWrapperMap()
//------------------------------------------------------------------------------
/**
 * Returns the math wrapper map used for RHS math elements
 *
 * @return The math wrapper map
 */
 //------------------------------------------------------------------------------
WrapperMap RHSEquation::GetMathWrapperMap()
{
   return mathWrapperMap;
}

//------------------------------------------------------------------------------
// MathTree *RealRHS::GetMathTree(bool releaseOwnership)
//------------------------------------------------------------------------------
/**
 * Returns a pointer to the MathTree (so the Assignment command can access it).
 *
 * @param releaseOwnership Flag indicating if the calling function acquires
 *                         ownership of the tree.  If so, this object no longer
 *                         holds the MathTree.  (Default is true, relinquishing
 *                         the pointer.)
 *
 * @return The MathTree
 */
//------------------------------------------------------------------------------
MathTree* RHSEquation::GetMathTree(bool releaseOwnership)
{
   MathTree *retval = mathTree;

   if (releaseOwnership)
   {
      mathTree = NULL;
      isExpressionInitialized = false;
   }

   return retval;
}


//------------------------------------------------------------------------------
// ElementWrapper* RunMathTree()
//------------------------------------------------------------------------------
/**
 * Evaluates a RHS math tree, returning a wrapper containing the resultant.
 *
 * @return The MathTree evaluation, in an ElementWrapper.
 */
//------------------------------------------------------------------------------
ElementWrapper* RHSEquation::RunMathTree(ElementWrapper *lhsWrapper)
{
   if (!mathTree) return nullptr;
   if (!isExpressionInitialized)
      throw CommandException("Equation must be initialized before evaluation.\n");
   MathNode *topNode = mathTree->GetTopNode();
   if (!topNode) throw CommandException("RHS is an equation, but top node is NULL\n");
   Integer returnType, rows, columns;
   topNode->GetOutputInfo(returnType, rows, columns);
   if (returnType < 0 || returnType >= Gmat::TypeCount)
      throw CommandException("Equation has an undefined result type.\n");
   const auto lhsType = lhsWrapper ? lhsWrapper->GetDataType() :
         static_cast<Gmat::ParameterType>(returnType);
   if (lhsType < 0 || lhsType >= Gmat::TypeCount)
      throw CommandException("Equation destination has an undefined type.\n");
   if (lhsType != returnType)
   {
      bool compatible = lhsType == Gmat::REAL_TYPE &&
            returnType == Gmat::RMATRIX_TYPE && rows == 1 && columns == 1;
      if (lhsType == Gmat::RVECTOR_TYPE && returnType == Gmat::RMATRIX_TYPE &&
            (rows == 1 || columns == 1)) compatible = true;
      if (lhsType == Gmat::RMATRIX_TYPE && returnType == Gmat::RVECTOR_TYPE)
         compatible = true;
      if (lhsType == Gmat::RMATRIX_TYPE && returnType == Gmat::REAL_TYPE && lhsWrapper)
      {
         auto *array = dynamic_cast<Array*>(lhsWrapper->GetRefObject());
         compatible = array && array->GetRowCount() == 1 && array->GetColCount() == 1;
      }
      if (!compatible)
      {
         CommandException error;
         error.SetDetails("Cannot set type \"%s\" to type \"%s\"",
               GmatBase::PARAM_TYPE_STRING[returnType].c_str(),
               GmatBase::PARAM_TYPE_STRING[lhsType].c_str());
         throw error;
      }
   }
   std::unique_ptr<ElementWrapper> result;
   switch (returnType)
   {
      case Gmat::REAL_TYPE:
      {
         const Real value = topNode->Evaluate();
         std::unique_ptr<NumberWrapper> wrapper(new NumberWrapper);
         wrapper->SetDescription(GmatStringUtil::ToString(value));
         wrapper->SetReal(value);
         result = std::move(wrapper);
         break;
      }
      case Gmat::RVECTOR_TYPE:
      {
         const Rvector value = topNode->VectorEvaluate();
         if (value.GetSize() != rows || columns != 1)
            throw CommandException("Equation vector size does not match its declared output.\n");
         std::unique_ptr<RvectorWrapper> wrapper(new RvectorWrapper(value));
         wrapper->SetDescription("VectorOutput");
         result = std::move(wrapper);
         break;
      }
      case Gmat::RMATRIX_TYPE:
      {
         const Rmatrix value = topNode->MatrixEvaluate();
         if (value.GetNumRows() != rows || value.GetNumColumns() != columns)
            throw CommandException("Equation matrix dimensions do not match its declared output.\n");
         std::unique_ptr<Array> array(new Array("ArrayOutput"));
         array->SetSize(value.GetNumRows(), value.GetNumColumns());
         array->SetRmatrix(value);
         std::unique_ptr<ArrayWrapper> wrapper(new ArrayWrapper);
         wrapper->SetDescription("ArrayOutput");
         wrapper->AdoptRefObject(std::move(array));
         if (topNode->GetTypeName() == "Cross3") wrapper->AllowOneDimArraySetting(true);
         result = std::move(wrapper);
         break;
      }
      case Gmat::OBJECT_TYPE:
      {
         GmatBase *source = topNode->EvaluateObject();
         if (!source) throw CommandException("Equation returned an undefined object.\n");
         std::unique_ptr<GmatBase> object(source->Clone());
         if (object.get() == source)
         {
            object.release();
            throw CommandException("Equation result Clone returned its source.\n");
         }
         if (!object || typeid(*object) != typeid(*source))
            throw CommandException("Equation result Clone changed its native object type.\n");
         std::unique_ptr<ObjectWrapper> wrapper(new ObjectWrapper);
         wrapper->SetDescription(source->GetName());
         wrapper->AdoptRefObject(std::move(object));
         result = std::move(wrapper);
         break;
      }
      case Gmat::STRING_TYPE:
      {
         const std::string value = topNode->EvaluateString();
         std::unique_ptr<StringWrapper> wrapper(new StringWrapper);
         wrapper->SetDescription(value);
         wrapper->SetString(value);
         result = std::move(wrapper);
         break;
      }
      default:
         throw CommandException("The return type of the equation or function is unknown.\n");
   }
   // The caller owns the wrapper; it owns any newly generated result object.
   return result.release();
}


//------------------------------------------------------------------------------
// bool RenameRefObject(const UnsignedInt type, const std::string &oldName,
//          const std::string &newName)
//------------------------------------------------------------------------------
/**
 * Object rename method (matching the GmatBase version)
 *
 * @param type The type of object getting renamed
 * @param oldName Name of the object before it get changed
 * @param newName The new object name
 *
 * @return true if an object name was changed
 */
//------------------------------------------------------------------------------
bool RHSEquation::RenameRefObject(const UnsignedInt type, const std::string &oldName,
         const std::string &newName)
{
   bool retval = false;
   isExpressionInitialized = false;
   rhsString = GmatStringUtil::ReplaceName(rhsString, oldName, newName);

   if (mathTree)
      retval = mathTree->RenameRefObject(type, oldName, newName);

   // Handle the wrapper map
   std::map<std::string, ElementWrapper *>::iterator ewi;
   for (ewi = mathWrapperMap.begin(); ewi != mathWrapperMap.end(); ++ewi)
   {
      std::string wrapperName = ewi->first;
      ElementWrapper *wrapper = ewi->second;
      if (wrapperName.find(oldName) != wrapperName.npos)
      {
         #ifdef DEBUG_RENAME
         MessageInterface::ShowMessage
            ("   Found '%s' in the mathWrapperMap\n", wrapperName.c_str());
         #endif
         std::string oldWrapperName = wrapperName;
         wrapperName = GmatStringUtil::ReplaceName(wrapperName, oldName, newName);

         // If name changed then replace and remove old one
         if (oldWrapperName != wrapperName)
         {
            #ifdef DEBUG_RENAME
            MessageInterface::ShowMessage
               ("   New name is '%s' and setting <%p> to wrapper\n", wrapperName.c_str(),
                wrapper);
            #endif

            mathWrapperMap[wrapperName] = wrapper;

            // Do not remove the iterator - it is used to iterate the for loop!
            // mathWrapperMap.erase(ewi);
         }
      }
   }

   return retval;
}


//------------------------------------------------------------------------------
// StringArray GetWrapperObjectNames()
//------------------------------------------------------------------------------
/**
 * Accessor for the names of objects used in the RHS expression
 *
 * @return A list of object names
 */
//------------------------------------------------------------------------------
StringArray RHSEquation::GetWrapperObjectNames()
{
   StringArray wons;

   if (mathTree == NULL)
      throw CommandException("The equation \"" + rhsString +
            "\" has not yet been constructed");

   MathNode *topNode = mathTree->GetTopNode();
   if (topNode && topNode->IsOfType("BuiltinFunctionNode"))
   {
      #ifdef DEBUG_WRAPPER_CODE
      MessageInterface::ShowMessage("   It is a builtin function call\n");
      #endif
      // Add math node elements to wrapper object names for string function
      StringArray tmpArray = mathTree->GetWrapperObjectNameArray();
      if (tmpArray.size() > 0)
         wons.insert(wons.end(), tmpArray.begin(), tmpArray.end());
   }
   else
   {
      // Add math node elements to ref object names
      StringArray tmpArray = mathTree->GetRefObjectNameArray(Gmat::PARAMETER);
      if (tmpArray.size() > 0)
         wons.insert(wons.end(), tmpArray.begin(), tmpArray.end());
   }

   #ifdef DEBUG_WRAPPER_CODE
      MessageInterface::ShowMessage("   Got the following from the MathTree:\n");
   #endif

   // Initialize the wrapper map
   if (mathWrapperMap.size() == 0)
   {
      for (UnsignedInt i = 0; i < wons.size(); ++i)
         mathWrapperMap[wons[i]] = NULL;
   }

   return wons;
}


//------------------------------------------------------------------------------
// StringArray GetFunctionNames()
//------------------------------------------------------------------------------
/**
 * Returns the list of functions used in the equation
 *
 * @return A string array containing the function names
 */
//------------------------------------------------------------------------------
StringArray RHSEquation::GetFunctionNames()
{
   StringArray funs;
   if (mathTree)
      funs = mathTree->GetGmatFunctionNames();
   return funs;
}

// Relocate the trees' borrowed map addresses after transferring their payloads.
void RHSEquation::Swap(RHSEquation &rhs)
{
   if (this == &rhs) return;
   rhsString.swap(rhs.rhsString);
   std::swap(resourceMap, rhs.resourceMap);
   std::swap(rhsWrapper, rhs.rhsWrapper);
   std::swap(mathTree, rhs.mathTree);
   mathWrapperMap.swap(rhs.mathWrapperMap);
   std::swap(isExpressionInitialized, rhs.isExpressionInitialized);
   expressionBindings.swap(rhs.expressionBindings);
   if (mathTree) mathTree->SetMathWrappers(&mathWrapperMap);
   if (rhs.mathTree) rhs.mathTree->SetMathWrappers(&rhs.mathWrapperMap);
}

void RHSEquation::GetClonedOwnedObjects(const RHSEquation &target,
      ObjectArray &originals, ObjectArray &copies) const
{
   GetClonedOwnedObjects(target, originals, copies, true);
   GetClonedOwnedObjects(target, originals, copies, false);
}

void RHSEquation::GetClonedOwnedObjects(const RHSEquation &target,
      ObjectArray &originals, ObjectArray &copies, bool sourceOwnedOnly) const
{
   if (mathWrapperMap.size() != target.mathWrapperMap.size() ||
         static_cast<bool>(mathTree) != static_cast<bool>(target.mathTree))
      throw CommandException("Changed copied equation ownership graph.\n");
   if (mathTree) mathTree->GetClonedOwnedObjects(*target.mathTree, originals, copies);
   std::map<ElementWrapper*, ElementWrapper*> seen;
   for (const auto &entry : mathWrapperMap)
   {
      const auto found = target.mathWrapperMap.find(entry.first);
      if (found == target.mathWrapperMap.end()) throw CommandException("Missing copied equation role.\n");
      if (!entry.second && !found->second) continue;
      if (!entry.second || !found->second) throw CommandException("Missing copied equation wrapper.\n");
      auto inserted = seen.emplace(entry.second, found->second);
      if (!inserted.second && inserted.first->second != found->second)
         throw CommandException("Changed copied equation wrapper alias.\n");
   }
   for (const auto &entry : seen)
      entry.second->CollectClonedOwnedReferences(*entry.first, originals, copies, sourceOwnedOnly);
}

void RHSEquation::RebindClonedReferences(const RHSEquation &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   isExpressionInitialized = false;
   if (rhsString != source.rhsString)
      throw CommandException("Cannot bind a different expression definition.\n");
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
   auto *resources = binding.scopes->CopyMap(source.resourceMap, actual);
   std::set<ElementWrapper*> seen;
   for (const auto &entry : source.mathWrapperMap)
      if (entry.second && seen.insert(entry.second).second)
         mathWrapperMap.at(entry.first)->RebindClonedReferences(*entry.second, actual);
   if (mathTree) mathTree->RebindClonedReferences(*source.mathTree, actual, &mathWrapperMap);
   expressionBindings = binding.scopes;
   resourceMap = resources;
   isExpressionInitialized = ExpressionBindingScope::preserveReadiness && source.isExpressionInitialized;
}

void RHSEquation::Reset()
{
   isExpressionInitialized = false;
   delete mathTree;
   mathTree = nullptr;
   ClearWrappers();
   rhsString.clear();
   resourceMap = nullptr;
   expressionBindings.reset();
}
