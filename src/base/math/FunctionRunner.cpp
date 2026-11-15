//$Id$
//------------------------------------------------------------------------------
//                                  FunctionRunner
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
// number NNG06CCA54C
//
// Author: Wendy Shoan
// Created: 2008.04.21
//
/**
 * Implements FunctionRunner class.
 */
//------------------------------------------------------------------------------

#include <memory>
#include "FunctionRunner.hpp"
#include "ExpressionBinding.hpp"
#include "MessageInterface.hpp"
#include "NumberWrapper.hpp"
#include "Variable.hpp"

//#define DEBUG_FUNCTION
//#define DEBUG_INPUT_OUTPUT
//#define DEBUG_EVALUATE
//#define DEBUG_FINALIZE

//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif
//#ifndef DEBUG_PERFORMANCE
//#define DEBUG_PERFORMANCE
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif
#ifdef DEBUG_PERFORMANCE
#include <ctime>                 // for clock()
#endif

//---------------------------------
// public methods
//---------------------------------

//------------------------------------------------------------------------------
// FunctionRunner()
//------------------------------------------------------------------------------
/**
 * Constructor.
 */
//------------------------------------------------------------------------------
FunctionRunner::FunctionRunner(const std::string &nomme)
   : MathFunction("FunctionRunner", nomme)
{
   objectTypeNames.push_back("FunctionRunner");
   theObjectMap = NULL;
   theGlobalObjectMap = NULL;
   theFunction = NULL;
   callingFunction = NULL;
   internalCS = NULL;
}


//------------------------------------------------------------------------------
// ~FunctionRunner()
//------------------------------------------------------------------------------
/**
 * Destructor.
 */
//------------------------------------------------------------------------------
FunctionRunner::~FunctionRunner()
{
}


//------------------------------------------------------------------------------
//  FunctionRunner(const FunctionRunner &copy)
//------------------------------------------------------------------------------
/**
 * Constructs the FunctionRunner object (copy constructor).
 * 
 * @param <copy> Object that is copied
 */
//------------------------------------------------------------------------------
FunctionRunner::FunctionRunner(const FunctionRunner &copy) :
   MathFunction(copy),
   theFunctionManager(copy.theFunctionManager),
   theObjectMap(NULL),
   theGlobalObjectMap(NULL),
   theFunctionName(copy.theFunctionName),
   theFunction(copy.theFunction),
   theInputNames(copy.theInputNames),
   theOutputNames(copy.theOutputNames),
   theInputNodes(copy.theInputNodes),
   callingFunction(NULL),
   internalCS(copy.internalCS)
{
   // Copy a call definition, never an active invocation or its scope maps.
   theFunctionManager.SetObjectMap(NULL);
   theFunctionManager.SetGlobalObjectMap(NULL);
   theFunctionManager.SetInputs(theInputNames);
   theFunctionManager.SetOutputs(theOutputNames);
}


//------------------------------------------------------------------------------
// void SetFunctionName(const std::string &fname)
//------------------------------------------------------------------------------
/*
 * Sets function name to the FunctionManager.
 *
 * @param  fname  The function name to set
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetFunctionName(const std::string &fname)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetFunctionName() passingArgs='%s', fname='%s'\n",
       GetName().c_str(), fname.c_str());
   #endif
   
   std::string replacement(fname);
   theFunctionManager.SetExpressionFunctionName(fname);
   if (theFunctionName != fname) theFunction = nullptr;
   theFunctionName.swap(replacement);
}


//------------------------------------------------------------------------------
// void SetFunction(Function *function)
//------------------------------------------------------------------------------
/*
 * Sets function pointer to the FunctionManager.
 *
 * @param  function  The function pointer to set
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetFunction(Function *function)
{
   if (!function)
      throw MathException("Cannot bind a null function to an expression.\n");
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetFunction() passingArgs='%s', function=%p, name='%s'\n",
       GetName().c_str(), function, function->GetName().c_str());
   #endif
   
   if (theFunctionManager.GetFunctionName() == function->GetName())
   {
      theFunctionManager.SetExpressionEnvironment(function, theObjectMap, theGlobalObjectMap);
      theFunction = function;
   }
}


//------------------------------------------------------------------------------
// void AddFunctionInput(const std::string &name)
//------------------------------------------------------------------------------
/*
 * Adds function input argument name to the input names.
 *
 * @param  name  The name to add the list
 */
//------------------------------------------------------------------------------
void FunctionRunner::AddFunctionInput(const std::string &name)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::AddFunctionInput() passingArgs='%s', adding name='%s'\n",
       GetName().c_str(), name.c_str());
   #endif
   
   theInputNames.push_back(name);
}


//------------------------------------------------------------------------------
// void SetFunctionOutputs()
//------------------------------------------------------------------------------
/*
 * Sets function output list to the FunctionManager.
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetFunctionOutputs()
{
   theFunctionManager.SetOutputs(theOutputNames);
}


//------------------------------------------------------------------------------
// const StringArray& GetInputs()
//------------------------------------------------------------------------------
/*
 * @return the input list
 */
//------------------------------------------------------------------------------
const StringArray& FunctionRunner::GetInputs()
{
   return theInputNames;
}


//------------------------------------------------------------------------------
// void AddInputNode(MathNode *node)
//------------------------------------------------------------------------------
/*
 * Adds input MathNode to the list
 *
 * @param  node  input MathNode to add to the list
 */
//------------------------------------------------------------------------------
void FunctionRunner::AddInputNode(MathNode *node)
{
   if (node != NULL)
      theInputNodes.push_back(node);
}


//------------------------------------------------------------------------------
// void SetFunctionInputs()
//------------------------------------------------------------------------------
/*
 * Sets function input list to the FunctionManager.
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetFunctionInputs()
{
   theFunctionManager.SetInputs(theInputNames);
}


//------------------------------------------------------------------------------
// void AddFunctionOutput(const std::string &name)
//------------------------------------------------------------------------------
/*
 * Adds function output argument name to the output names.
 *
 * @param  name  The name to add the list
 */
//------------------------------------------------------------------------------
void FunctionRunner::AddFunctionOutput(const std::string &name)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::AddFunctionOutput() passingArgs='%s', adding name='%s'\n",
       GetName().c_str(), name.c_str());
   #endif
   
   theOutputNames.push_back(name);
}


//------------------------------------------------------------------------------
// void SetCallingFunction();
//------------------------------------------------------------------------------
void FunctionRunner::SetCallingFunction(FunctionManager *fm)
{
   callingFunction = fm;
}


//------------------------------------------------------------------------------
//  void SetObjectMap(ObjectMap *map)
//------------------------------------------------------------------------------
/**
 * Called by the MathTree to set the local asset store used by the GmatCommand
 * 
 * @param <map> Pointer to the local object map
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetObjectMap(ObjectMap *map)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetObjectMap() entered, theFunctionName='%s', "
       "map=<%p>\n", theFunctionName.c_str(), map);
   #endif

   theObjectMap = map;
   theFunctionManager.SetObjectMap(map);
}


//------------------------------------------------------------------------------
//  void SetGlobalObjectMap(ObjectMap *map)
//------------------------------------------------------------------------------
/**
 * Called by the MathTree to set the global asset store used by the GmatCommand
 * 
 * @param <map> Pointer to the global object map
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetGlobalObjectMap(ObjectMap *map)
{
   SetExpressionObjectMaps(theObjectMap, map);
}

//------------------------------------------------------------------------------
//  void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
/**
 * Called by the MathTree to set the SolarSystem used by the GmatCommand
 * 
 * @param <ss> Pointer to the SolarSystem
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetSolarSystem(SolarSystem *ss)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetSolarSystem() entered, theFunctionName='%s', "
       "ss=<%p>\n", theFunctionName.c_str(), ss);
   #endif
   
   theFunctionManager.SetSolarSystem(ss);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("FunctionRunner::SetSolarSystem() exiting\n");
   #endif
}


//------------------------------------------------------------------------------
//  void SetInternalCoordSystem(CoordinateSystem *cs)
//------------------------------------------------------------------------------
/**
 * Called by the MathTree to set the CoordinateSystem used by the GmatCommand and
 * function objects.
 * 
 * @param <cs> Pointer to the CoordinateSystem
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetInternalCoordSystem(CoordinateSystem *cs)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetInternalCoordSystem() entered, theFunctionName='%s', "
       "cs=<%p>\n", theFunctionName.c_str(), cs);
   #endif
   
   internalCS = cs;
   theFunctionManager.SetInternalCoordinateSystem(cs);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("FunctionRunner::SetInternalCoordSystem() leaving\n");
   #endif
}


//------------------------------------------------------------------------------
//  void SetTransientForces(std::vector<PhysicalModel*> *tf)
//------------------------------------------------------------------------------
/**
 * Called by the MathTree to set the transient forces used by the GmatCommand
 * 
 * @param <map> Pointer to the vector of transient forces
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetTransientForces(std::vector<PhysicalModel*> *tf)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetTransientForces() entered, theFunctionName='%s', "
       "tf=<%p>\n", theFunctionName.c_str(), tf);
   #endif
   
   theFunctionManager.SetTransientForces(tf);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("FunctionRunner::SetTransientForces() exiting\n");
   #endif
}


//------------------------------------------------------------------------------
//  void SetPublisher(Publisher *pub)
//------------------------------------------------------------------------------
/**
 * Called by the MathTree to set the Publisher used by the GmatCommand
 * 
 * @param <pub> Pointer to the Publisher
 */
//------------------------------------------------------------------------------
void FunctionRunner::SetPublisher(Publisher *pub)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::SetPublisher() entered, theFunctionName='%s', "
       "pub=<%p>\n", theFunctionName.c_str(), pub);
   #endif
   
   theFunctionManager.SetPublisher(pub);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("FunctionRunner::SetPublisher() exiting\n");
   #endif
}


//------------------------------------------------------------------------------
// void GetOutputInfo(Integer &type, Integer &rowCount, Integer &colCount)
//------------------------------------------------------------------------------
void FunctionRunner::GetOutputInfo(Integer &type,
                                   Integer &rowCount, Integer &colCount)
{
   Function *function = theFunctionManager.GetFunction();
   if (function == NULL)
      throw MathException("FunctionRunner::GetOutputInfo() function is NULL");
   
   #ifdef DEBUG_INPUT_OUTPUT
   MessageInterface::ShowMessage
      ("FunctionRunner::GetOutputInfo() entered, this=<%p><%s>, function=<%s><%p>\n",
       this, GetName().c_str(), function->GetName().c_str(), function);
   #endif
   
   // check for function output count
   IntegerArray rowCounts, colCounts;
   WrapperTypeArray outputTypes = function->GetOutputTypes(rowCounts, colCounts);
   std::string errMsg;
   
   if (outputTypes.size() == 0)
   {
      errMsg = "The function \"" + function->GetName() + "\" does not return"
         " any value";
   }
   else if (outputTypes.size() > 1)
   {
      errMsg = "The function \"" + function->GetName() + "\" returns more than "
         " one value";
   }
   else
   {
      if (outputTypes[0] == Gmat::VARIABLE_WT)
      {
         type = Gmat::REAL_TYPE;
         rowCount = 1;
         colCount = 1;
      }
      else if (outputTypes[0] == Gmat::ARRAY_WT)
      {
         if (rowCounts.size() != 1 || colCounts.size() != 1)
            throw MathException("Function Array output is missing its dimensions.\n");
         type = Gmat::RMATRIX_TYPE;
         rowCount = rowCounts[0];
         colCount = colCounts[0];
         matrix.SetSize(rowCount, colCount);
      }
      else if (outputTypes[0] == Gmat::VECTOR_WT)
      {
         if (rowCounts.size() != 1 || colCounts.size() != 1 ||
               rowCounts[0] <= 0 || colCounts[0] <= 0 ||
               (rowCounts[0] != 1 && colCounts[0] != 1))
            throw MathException("Function vector output must declare one row or column.\n");
         type = Gmat::RVECTOR_TYPE;
         rowCount = rowCounts[0] == 1 ? colCounts[0] : rowCounts[0];
         colCount = 1;
      }
      else if (outputTypes[0] == Gmat::OBJECT_WT)
      {
         type = Gmat::OBJECT_TYPE;
         rowCount = 1;
         colCount = 1;
      }
      else
         errMsg = "Unsupported function expression output type";
   }
   
   if (errMsg != "")
      throw MathException("FunctionRunner::GetOutputInfo() " + errMsg);

   elementType = type;
   
   //======================================================================
   #ifdef __ALLOW_MATH_EXP_NODE__
   //======================================================================
   
   if (leftNode == NULL)
      throw MathException("FunctionRunner::GetOutputInfo() The left node is NULL");
   
   Integer type1, row1, col1; // Left node
   
   // Get the type(Real or Matrix), # rows and # columns of the left node
   leftNode->GetOutputInfo(type1, row1, col1);
   
   // Check output type and assign
   
   if (outputTypes[0] == Gmat::VARIABLE_WT)
   {
      if (type1 != Gmat::REAL_TYPE)
         throw MathException
            ("FunctionRunner::GetOutputInfo() The GmatFunction \"%s\" expecting "
             "output type of Real");
      
      type = Gmat::REAL_TYPE;
      rowCount = 1;
      colCount = 1;
   }
   else if (outputTypes[0] == Gmat::ARRAY_WT)
   {
      if (type1 != Gmat::RMATRIX_TYPE)
         throw MathException
            ("FunctionRunner::GetOutputInfo() The GmatFunction \"%s\" expecting "
             "output type of Rmatrix");
      
      type = Gmat::RMATRIX_TYPE;
      rowCount = rowCounts[0];
      colCount = colCounts[0];
      matrix.SetSize(rowCount, colCount);
   }
   
   //======================================================================
   #endif
   //======================================================================
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::GetOutputInfo() returning type=%d, rowCount=%d, colCount=%d\n",
       type, rowCount, colCount);
   #endif
}


//------------------------------------------------------------------------------
// bool ValidateInputs()
//------------------------------------------------------------------------------
/**
 * This method calls its subnodes and checks to be sure that the subnodes return
 * compatible data for the function.
 */
//------------------------------------------------------------------------------
bool FunctionRunner::ValidateInputs()
{
   Function *function = theFunctionManager.GetFunction();
   if (function == NULL)
      throw MathException("FunctionRunner::ValidateInputs() function is NULL");
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::ValidateInputs() entered, this=<%p><%s>, function=<%s><%p>\n",
       this, GetName().c_str(), function->GetName().c_str(), function);
   #endif
   
   //@todo
   //How can we validate input here? Just return true for now.
   return true;
}


//------------------------------------------------------------------------------
// Real Evaluate()
//------------------------------------------------------------------------------
/**
 * @return the FunctionRunner of left node
 *
 */
//------------------------------------------------------------------------------
Real FunctionRunner::Evaluate()
{
   Function *function = theFunctionManager.GetFunction();
   if (function == NULL)
      throw MathException("FunctionRunner::Evaluate() function is NULL");
   
   #ifdef DEBUG_PERFORMANCE
   static Integer callCount = 0;
   callCount++;      
   clock_t t1 = clock();
   MessageInterface::ShowMessage
      ("=== FunctionRunner::Evaluate() entered, '%s' Count = %d\n",
       GetName().c_str(), callCount);
   #endif
   
   #ifdef DEBUG_EVALUATE
   MessageInterface::ShowMessage
      ("FunctionRunner::Evaluate() entered, this=<%p><%s>, function=<%s><%p>, "
       "internalCS=<%p>\n", this, GetName().c_str(), function->GetName().c_str(),
       function, internalCS);
   #endif
   
   if (elementType == Gmat::RMATRIX_TYPE || elementType == Gmat::RVECTOR_TYPE)
      throw MathException
         ("The function \"" + function->GetName() + "\" returns a matrix or vector value");
   
   //=======================================================
   #ifdef __ALLOW_MATH_EXP_NODE__
   HandlePassingMathExp(function);
   #endif
   //=======================================================
   
   #ifdef DEBUG_EVALUATE
   MessageInterface::ShowMessage
      ("Calling FunctionManager::Evaluate()\n"
       "===================================================================\n");
   #endif
   
   Real result = -9999.9999;
   
   /// as temp fix, try setting InternalCoordinateSystem on FunctionManager
   theFunctionManager.SetInternalCoordinateSystem(internalCS);
   result = theFunctionManager.Evaluate(callingFunction);
   
   // FunctionManager releases output wrappers after copying the result.

   #ifdef DEBUG_EVALUATE
   MessageInterface::ShowMessage
      ("FunctionRunner::Evaluate() returning %f\n", result);
   #endif
   
   #ifdef DEBUG_PERFORMANCE
   clock_t t2 = clock();
   MessageInterface::ShowMessage
      ("=== FunctionRunner::Evaluate() exiting, '%s' Count = %d, Run Time: %f seconds\n",
       GetName().c_str(), callCount, (Real)(t2-t1)/CLOCKS_PER_SEC);
   #endif
   return result;
}


//------------------------------------------------------------------------------
// Rvector VectorEvaluate()
//------------------------------------------------------------------------------
/**
 * @return the FunctionRunner of left node
 *
 */
//------------------------------------------------------------------------------
Rvector FunctionRunner::VectorEvaluate()
{
   if (!theFunctionManager.GetFunction())
      throw MathException("FunctionRunner vector function is NULL.\n");
   if (elementType != Gmat::RVECTOR_TYPE)
      throw MathException("FunctionRunner does not declare a vector result.\n");
   return theFunctionManager.VectorEvaluate(callingFunction);
}

// Matrix evaluation also accepts a vector represented as a column Array.
Rmatrix FunctionRunner::MatrixEvaluate()
{
   Function *function = theFunctionManager.GetFunction();
   if (function == NULL)
      throw MathException("FunctionRunner::Evaluate() function is NULL");
   
   #ifdef DEBUG_PERFORMANCE
   static Integer callCount = 0;
   callCount++;      
   clock_t t1 = clock();
   MessageInterface::ShowMessage
      ("=== FunctionRunner::MatrixEvaluate() entered, '%s' Count = %d\n",
       GetName().c_str(), callCount);
   #endif
   
   #ifdef DEBUG_EVALUATE
   MessageInterface::ShowMessage
      ("FunctionRunner::MatrixEvaluate() entered, this=<%p><%s>, function=<%s><%p>\n",
       this, GetName().c_str(), function->GetName().c_str(), function);
   #endif
   
   if (elementType == Gmat::REAL_TYPE)
      throw MathException
         ("The function \"" + function->GetName() + "\" returns Real value");
   
   Rmatrix rmatResult = theFunctionManager.MatrixEvaluate(callingFunction);
   
   // FunctionManager releases output wrappers after copying the result.

   #ifdef DEBUG_PERFORMANCE
   clock_t t2 = clock();
   MessageInterface::ShowMessage
      ("=== FunctionRunner::MatrixEvaluate() exiting, '%s' Count = %d, Run Time: %f seconds\n",
       GetName().c_str(), callCount, (Real)(t2-t1)/CLOCKS_PER_SEC);
   #endif
   
   return rmatResult;
}

//------------------------------------------------------------------------------
// GmatBase EvaluateObject()
//------------------------------------------------------------------------------
/**
 * @return the FunctionRunner of left node
 *
 */
//------------------------------------------------------------------------------
GmatBase* FunctionRunner::EvaluateObject()
{
   Function *function = theFunctionManager.GetFunction();
   if (function == NULL)
      throw MathException("FunctionRunner::Evaluate() function is NULL");
   
   #ifdef DEBUG_PERFORMANCE
   static Integer callCount = 0;
   callCount++;      
   clock_t t1 = clock();
   MessageInterface::ShowMessage
      ("=== FunctionRunner::EvaluateObject() entered, '%s' Count = %d\n",
       GetName().c_str(), callCount);
   #endif
   
   #ifdef DEBUG_EVALUATE
   MessageInterface::ShowMessage
      ("FunctionRunner::EvaluateObject() entered, this=<%p><%s>, function=<%s><%p>\n",
       this, GetName().c_str(), function->GetName().c_str(), function);
   #endif
   
   if (elementType != Gmat::OBJECT_TYPE)
      throw MathException
         ("The function \"" + function->GetName() + "\" does not return OBJECT_TYPE");
   
   GmatBase *retObj = theFunctionManager.EvaluateObject(callingFunction);
   
   // FunctionManager releases output wrappers after copying the result.

   #ifdef DEBUG_PERFORMANCE
   clock_t t2 = clock();
   MessageInterface::ShowMessage
      ("=== FunctionRunner::EvaluateObject() exiting, '%s' Count = %d, Run Time: %f seconds\n",
       GetName().c_str(), callCount, (Real)(t2-t1)/CLOCKS_PER_SEC);
   #endif
   
   return retObj;
}

//------------------------------------------------------------------------------
// void Finalize()
//------------------------------------------------------------------------------
void FunctionRunner::Finalize()
{
   #ifdef DEBUG_FINALIZE
   MessageInterface::ShowMessage
      ("FunctionRunner::RunComplete() calling FunctionManager::Finalize()\n");
   #endif
   theFunctionManager.Finalize();
}

void FunctionRunner::RetireExpressionReferences()
{
   // Finalize is a separate step, attempted for every runner while its caller
   // store still lives. Detachment must also work if that cleanup reported an
   // error or is reentering the same manager.
   theFunctionManager.RetireExpressionContext();
   theObjectMap = nullptr;
   theGlobalObjectMap = nullptr;
   internalCS = nullptr;
   callingFunction = nullptr;
   theFunction = theFunctionManager.GetFunction();
   isInitialized = false;
}


//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * Clone of the FunctionRunner operation.
 *
 * @return clone of the FunctionRunner operation.
 *
 */
//------------------------------------------------------------------------------
GmatBase* FunctionRunner::Clone() const
{
   return (new FunctionRunner(*this));
}


//------------------------------------------------------------------------------
// GmatBase* FunctionRunner::FindObject(const std::string &name)
//------------------------------------------------------------------------------
GmatBase* FunctionRunner::FindObject(const std::string &name)
{
   std::string newName = name;
   
   // Ignore array indexing of Array
   std::string::size_type index = name.find('(');
   if (index != name.npos)
      newName = name.substr(0, index);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("FunctionRunner::FindObject() theObjectMap=<%p>, theGlobalObjectMap=<%p>, "
       "newName='%s'\n", theObjectMap, theGlobalObjectMap, newName.c_str());
   if (theObjectMap)
   {
      MessageInterface::ShowMessage("Here is the local object map:\n");
      for (std::map<std::string, GmatBase *>::iterator i = theObjectMap->begin();
           i != theObjectMap->end(); ++i)
         MessageInterface::ShowMessage("   %s\n", i->first.c_str());
   }
   if (theGlobalObjectMap)
   {
      MessageInterface::ShowMessage("Here is the global object map:\n");
      for (std::map<std::string, GmatBase *>::iterator i = theGlobalObjectMap->begin();
           i != theGlobalObjectMap->end(); ++i)
         MessageInterface::ShowMessage("   %s\n", i->first.c_str());
   }
   #endif
   
   // Check for the object in the Local Object Store (LOS) first
   if (theObjectMap && theObjectMap->find(newName) != theObjectMap->end())
      return (*theObjectMap)[newName];
   
   // If not found in the LOS, check the Global Object Store (GOS)
   if (theGlobalObjectMap &&
       theGlobalObjectMap->find(newName) != theGlobalObjectMap->end())
      return (*theGlobalObjectMap)[newName];
   
   return NULL;
}

//------------------------------------------------------------------------------
// void HandlePassingMathExp(Function *function)
//------------------------------------------------------------------------------
/*
 * @todo This method is not complete and will be implemented fully later.
 */
//------------------------------------------------------------------------------
void FunctionRunner::HandlePassingMathExp(Function *function)
{
   if (leftNode == NULL)
      throw MathException
         ("The left node of \"" + function->GetName() + "\" is NULL");
   
   #ifdef DEBUG_EVALUATE
   MessageInterface::ShowMessage
      ("   leftNode is%s Function input and %sa MathElement\n",
       leftNode->IsFunctionInput() ? "" : " not",
       leftNode->IsOfType("MathElement") ? "" : " not");
   MessageInterface::ShowMessage
      ("Capturing function input nodes\n"
       "===================================================================\n");
   #endif
   
   // Evaluate nodes before the common invocation entry creates or refreshes
   // formals. Initialize here used to mutate a recursive caller's live store
   // before PrepareExecution had a chance to save that caller frame.
   theFunctionManager.PrepareObjectMap();
   for (Integer i = 0; i < (Integer)theInputNodes.size(); ++i)
   {
      std::unique_ptr<Variable> passingInput(new Variable(theInputNodes[i]->GetName()));
      passingInput->SetReal(theInputNodes[i]->Evaluate());
      bool inputAdded = false;
      const bool accepted = theFunctionManager.SetPassedInput(i, passingInput.get(), inputAdded);
      if (inputAdded) passingInput.release();
      if (!accepted)
         throw MathException("Unable to replace input argument for function \"" +
                             function->GetName() + "\"\n");
   }
}


FunctionRunner& FunctionRunner::operator=(const FunctionRunner &copy)
{
   if (&copy == this) return *this;
   const auto previousContext = GetFunctionValueContext();
   // FunctionManager enforces its existing non-running assignment guard and
   // disposes invocation-owned state before installing the copied definition.
   theFunctionManager = copy.theFunctionManager;
   MathFunction::operator=(copy);
   theObjectMap = NULL;
   theGlobalObjectMap = NULL;
   theFunctionName = copy.theFunctionName;
   theFunction = copy.theFunction;
   theInputNames = copy.theInputNames;
   theOutputNames = copy.theOutputNames;
   theInputNodes = copy.theInputNodes;
   callingFunction = NULL;
   internalCS = copy.internalCS;
   theFunctionManager.SetObjectMap(NULL);
   theFunctionManager.SetGlobalObjectMap(NULL);
   theFunctionManager.SetInputs(theInputNames);
   theFunctionManager.SetOutputs(theOutputNames);
   return *this;
}

std::vector<MathNode*> FunctionRunner::GetStructuralChildren() const
{
   auto children = MathNode::GetStructuralChildren();
   children.insert(children.end(), theInputNodes.begin(), theInputNodes.end());
   return children;
}

void FunctionRunner::RebindClonedChildren(const MathNode &source,
      const std::function<MathNode*(MathNode*)> &resolve)
{
   const auto *runner = dynamic_cast<const FunctionRunner*>(&source);
   if (!runner) throw MathException("Incompatible copied function expression.\n");
   MathNodeArray inputs;
   inputs.reserve(runner->theInputNodes.size());
   for (MathNode *node : runner->theInputNodes) inputs.push_back(resolve(node));
   MathNode::RebindClonedChildren(source, resolve);
   theInputNodes.swap(inputs);
}

void FunctionRunner::SetExpressionObjectMaps(ObjectMap *local, ObjectMap *global)
{
   // Resolve in the incoming environment, including a local null declaration
   // shadowing a global name. Native definition copies deliberately detach
   // their old scopes; initialization must bind their callee again as well.
   GmatBase *object = nullptr;
   bool found = false;
   for (ObjectMap *scope : {local, global})
   {
      if (!scope) continue;
      const auto entry = scope->find(theFunctionName);
      if (entry == scope->end()) continue;
      object = entry->second;
      found = true;
      break;
   }
   auto *function = dynamic_cast<ObjectManagedFunction*>(object);
   // Clear a previous callee on a missing or invalid binding. An initialization
   // failure must not leave an expression able to call its previous function.
   theFunctionManager.SetExpressionEnvironment(function, local, global);
   theObjectMap = local;
   theGlobalObjectMap = global;
   theFunction = function;
   if (!function)
   {
      if (!found || !object)
         throw MathException("Cannot find function \"" + theFunctionName + "\" in the expression environment.\n");
      throw MathException("Expression resource \"" + theFunctionName + "\" is not an ObjectManagedFunction.\n");
   }
}

void FunctionRunner::RebindExpressionReferences(const MathNode &source,
      const std::function<GmatBase*(GmatBase*)> &resolve, WrapperMap *)
{
   const auto *runner = dynamic_cast<const FunctionRunner*>(&source);
   if (!runner || typeid(*this) != typeid(source))
      throw MathException("Incompatible native function expression copy.\n");
   ExpressionBindingScope binding;
   auto *local = binding.scopes->CopyMap(runner->theObjectMap, resolve);
   // Global commands publish into the native shared store. A value-owned
   // scope (including a local/global alias) keeps its copied-container policy.
   const bool valueGlobal = runner->theGlobalObjectMap == runner->theObjectMap ||
         runner->theFunctionManager.OwnsExpressionMap(runner->theGlobalObjectMap);
   auto *global = valueGlobal ? binding.scopes->CopyMap(runner->theGlobalObjectMap, resolve) :
         runner->theGlobalObjectMap;
   Function *callee = runner->theFunctionManager.GetFunction();
   if (callee && runner->theFunction != callee)
      throw MathException("Inconsistent cached expression callee.\n");
   // A call definition can be rebound by name on its next native Initialize.
   // Do not inspect an expired cached object while copying that definition.
   auto *function = BindExpressionReference(callee, resolve);
   auto *coordinateSystem = BindExpressionReference(runner->internalCS, resolve);
   theFunctionManager.RebindExpressionContext(runner->theFunctionManager, resolve, local, global);
   theObjectMap = local;
   theGlobalObjectMap = global;
   theFunction = function;
   internalCS = coordinateSystem;
   callingFunction = nullptr;
}
