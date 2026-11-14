//$Id$
//------------------------------------------------------------------------------
//                                   MathTree
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
// Created: 2006/04/17
//
/**
 * Defines the MathTree base class used for Math in Scripts.
 */
//------------------------------------------------------------------------------
#include "MathTree.hpp"
#include "ExpressionBinding.hpp"
#include "MathFunction.hpp"
#include "MathElement.hpp"
#include "FunctionRunner.hpp"
#include "StringUtil.hpp"            // for GetArrayIndex()
#include "InterpreterException.hpp"
#include "MessageInterface.hpp"

//#define DEBUG_MATH_TREE 1
//#define DEBUG_MATH_TREE_INIT 1
//#define DEBUG_MATH_TREE_EVAL 1
//#define DEBUG_MATH_WRAPPERS
//#define DEBUG_VALIDATE
//#define DEBUG_RENAME
//#define DEBUG_FUNCTION

//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif
#include <set>
#include <typeinfo>
#include <exception>

namespace
{
std::vector<MathNode*> StructuralMathNodes(MathNode *root, bool rejectCycles)
{
   std::vector<MathNode*> nodes;
   std::set<MathNode*> visited, visiting;
   std::function<void(MathNode*)> visit = [&](MathNode *node)
   {
      if (!node) return;
      if (rejectCycles && visiting.count(node))
         throw MathException("Cyclic expression-node ownership.\n");
      if (!visited.insert(node).second) return;
      nodes.push_back(node);
      visiting.insert(node);
      for (MathNode *child : node->GetStructuralChildren()) visit(child);
      visiting.erase(node);
   };
   visit(root);
   return nodes;
}

struct MathNodeCopies
{
   std::vector<std::unique_ptr<MathNode>> nodes;
   MathNode *root = NULL;
   MathNode *Release()
   {
      for (auto &node : nodes) node.release();
      return root;
   }
};

MathNodeCopies CopyMathNodes(MathNode *source, MathNode *previous = NULL)
{
   const auto originals = StructuralMathNodes(source, true);
   const auto oldNodes = StructuralMathNodes(previous, false);
   std::set<GmatBase*> forbidden;
   forbidden.insert(originals.begin(), originals.end());
   forbidden.insert(oldNodes.begin(), oldNodes.end());
   std::map<MathNode*, MathNode*> copies;
   MathNodeCopies result;
   GmatBase::FunctionValueCopyGuard copying;
   for (MathNode *original : originals)
   {
      std::unique_ptr<GmatBase> value(original->Clone());
      if (forbidden.count(value.get()))
      {
         value.release();
         throw MathException("MathNode Clone returned an existing source/peer.\n");
      }
      auto *node = dynamic_cast<MathNode*>(value.get());
      if (!node || typeid(*node) != typeid(*original))
         throw MathException("Cannot clone expression node with its native type.\n");
      forbidden.insert(node);
      copies.emplace(original, node);
      std::unique_ptr<MathNode> owned(static_cast<MathNode*>(value.release()));
      result.nodes.push_back(std::move(owned));
   }
   auto resolve = [&](MathNode *original)
   {
      if (!original) return static_cast<MathNode*>(NULL);
      const auto found = copies.find(original);
      if (found == copies.end()) throw MathException("Unexposed expression-node dependency.\n");
      return found->second;
   };
   for (MathNode *original : originals)
      copies.at(original)->RebindClonedChildren(*original, resolve);
   result.root = resolve(source);
   return result;
}

void VisitStructuralMathNodes(MathNode *root, const std::function<void(MathNode*)> &action)
{
   // Inventory first: reject malformed cycles before any runtime association.
   for (MathNode *node : StructuralMathNodes(root, true)) action(node);
}
}


//------------------------------------------------------------------------------
//  MathTree()
//------------------------------------------------------------------------------
/**
 * Constructs the MathTree object (default constructor).
 */
//------------------------------------------------------------------------------
MathTree::MathTree(const std::string &typeStr, const std::string &nomme) :
   GmatBase(Gmat::MATH_TREE, typeStr, nomme),
   theTopNode(NULL),
   theObjectMap(NULL),
   theGlobalObjectMap(NULL),
   theWrapperMap(NULL)
{
}


//------------------------------------------------------------------------------
//  ~MathTree(void)
//------------------------------------------------------------------------------
/**
 * Destroys the MathTree object (destructor).
 */
//------------------------------------------------------------------------------
MathTree::~MathTree()
{
   // Need to delete all math nodes
   if (theTopNode)
   {
      nodesToDelete.clear();
      DeleteNode(theTopNode);
      
      for (std::vector<MathNode*>::iterator i = nodesToDelete.begin();
           i != nodesToDelete.end(); ++i)
      {
         if ((*i) != NULL)
         {
            #ifdef DEBUG_MEMORY
            MemoryTracker::Instance()->Remove
               ((*i), (*i)->GetName(), "MathTree::~MathTree()", (*i)->GetTypeName() +
                " deleting math node");
            #endif
            
            delete (*i);
            (*i) = NULL;
         }
      }
      nodesToDelete.clear();
   }
}


//------------------------------------------------------------------------------
//  MathTree(const MathTree &mt)
//------------------------------------------------------------------------------
/**
 * Constructs the MathTree object (copy constructor).
 * 
 * @param <mn> Object that is copied
 */
//------------------------------------------------------------------------------
MathTree::MathTree(const MathTree &mt) :
   GmatBase(mt),
   theTopNode(NULL),
   theObjectMap(NULL),
   theGlobalObjectMap(NULL),
   theWrapperMap(NULL),
   theParamArray(mt.theParamArray),
   theAllParamArray(mt.theAllParamArray),
   theGmatFunctionNames(mt.theGmatFunctionNames),
   theWrapperNames(mt.theWrapperNames),
   theFunctions(mt.theFunctions)
{
   auto nodes = CopyMathNodes(mt.theTopNode);
   theTopNode = nodes.Release();
   isInitialized = false;
}


//------------------------------------------------------------------------------
//  MathTree& operator=(const MathTree &mt)
//------------------------------------------------------------------------------
/**
 * Sets one MathTree object to match another (assignment operator).
 * 
 * @param <mt> The object that is copied.
 * 
 * @return this object, with the parameters set as needed.
 */
//------------------------------------------------------------------------------
MathTree& MathTree::operator=(const MathTree &mt)
{
   if (this == &mt) return *this;
   const auto previousContext = GetFunctionValueContext();
   const auto previousBindings = expressionBindings;
   auto nodes = CopyMathNodes(mt.theTopNode, theTopNode);
   auto parameters = mt.theParamArray;
   auto allParameters = mt.theAllParamArray;
   auto functionNames = mt.theGmatFunctionNames;
   auto wrapperNames = mt.theWrapperNames;
   auto functions = mt.theFunctions;
   const auto previous = StructuralMathNodes(theTopNode, false);
   GmatBase::operator=(mt);
   theParamArray.swap(parameters);
   theAllParamArray.swap(allParameters);
   theGmatFunctionNames.swap(functionNames);
   theWrapperNames.swap(wrapperNames);
   theFunctions.swap(functions);
   theTopNode = nodes.Release();
   theObjectMap = NULL;
   theGlobalObjectMap = NULL;
   theWrapperMap = NULL;
   nodesToDelete.clear();
   isInitialized = false;
   for (MathNode *node : previous) delete node;
   expressionBindings.reset();
   return *this;
}


//------------------------------------------------------------------------------
// const StringArray& GetGmatFunctionNames()
//------------------------------------------------------------------------------
const StringArray& MathTree::GetGmatFunctionNames()
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::GetGmatFunctionNames() theTopNode type=%s, desc='%s'returning "
       "%d GmatFunctions\n", theTopNode->GetTypeName().c_str(),
       theTopNode->GetName().c_str(), theGmatFunctionNames.size());
   for (UnsignedInt i=0; i<theGmatFunctionNames.size(); i++)
      MessageInterface::ShowMessage("   '%s'\n", theGmatFunctionNames[i].c_str());
   
   #endif
   
   return theGmatFunctionNames;
}


//------------------------------------------------------------------------------
// void SetGmatFunctionNames(StringArray funcList)
//------------------------------------------------------------------------------
void MathTree::SetGmatFunctionNames(StringArray funcList)
{
   theGmatFunctionNames = funcList;
}


//------------------------------------------------------------------------------
// std::vector<Function*> GetFunctions() const
//------------------------------------------------------------------------------
std::vector<Function*> MathTree::GetFunctions() const
{
   return theFunctions;
}


//------------------------------------------------------------------------------
// void SetFunction(Function *function)
//------------------------------------------------------------------------------
void MathTree::SetFunction(Function *function)
{
   if (!function)
      throw MathException("Cannot bind a null function to a MathTree.\n");
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetFunction() function=<%p>, name='%s'\n", function,
       function->GetName().c_str());
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetFunctionToRunner() theTopNode type=%s, desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetFunctionToRunner(theTopNode, function);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetFunction() returning\n");
   #endif
}

//------------------------------------------------------------------------------
// void SetCallingFunction();
//------------------------------------------------------------------------------
void MathTree::SetCallingFunction(FunctionManager *fm)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetCallingFunction() fm=<%p>, name='%s'\n", fm,
            fm ? fm->GetFunctionName().c_str() : "NULL");
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetCallingFunction() theTopNode type=%s, desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetCallingFunctionToRunner(theTopNode, fm);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetCallingFunction() returning\n");
   #endif
}


//------------------------------------------------------------------------------
// MathNode* GetTopNode()
//------------------------------------------------------------------------------
MathNode* MathTree::GetTopNode()
{
   return theTopNode;
}


//------------------------------------------------------------------------------
// void SetTopNode(MathNode *node)
//------------------------------------------------------------------------------
void MathTree::SetTopNode(MathNode *node)
{
   const auto previousBindings = expressionBindings;
   if (node == theTopNode) return;
   const auto next = StructuralMathNodes(node, true);
   const auto previous = StructuralMathNodes(theTopNode, false);
   std::set<MathNode*> retained(next.begin(), next.end());
   theTopNode = node;
   theObjectMap = NULL;
   theGlobalObjectMap = NULL;
   theWrapperMap = NULL;
   theParamArray.clear();
   theAllParamArray.clear();
   theGmatFunctionNames.clear();
   theWrapperNames.clear();
   theFunctions.clear();
   nodesToDelete.clear();
   isInitialized = false;
   expressionBindings.reset();
   for (MathNode *old : previous)
      if (!retained.count(old)) delete old;
}


//------------------------------------------------------------------------------
// void SetMathWrappers(std::map<std::string, ElementWrapper*> *wrapperMap)
//------------------------------------------------------------------------------
void MathTree::SetMathWrappers(std::map<std::string, ElementWrapper*> *wrapperMap)
{
   if (theTopNode == NULL)
      return;
   
   theWrapperMap = wrapperMap;
   
   #ifdef DEBUG_MATH_WRAPPERS
   MessageInterface::ShowMessage
      ("MathTree::SetMathWrappers() entered, theWrapperMap=%p, theTopNode=%s, %s\n",
       theWrapperMap, theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetMathElementWrappers(theTopNode);
   
   #ifdef DEBUG_MATH_WRAPPERS
   MessageInterface::ShowMessage
      ("MathTree::SetMathWrappers() leaving, theWrapperMap=%p, theTopNode=%s, %s\n",
       theWrapperMap, theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
}

//------------------------------------------------------------------------------
// bool Validate(std::string &msg)
//------------------------------------------------------------------------------
/**
 * Validates math tree nodes by going through theWrapperMap.
 *
 * @param  msg  Output error message if any
 *
 * @return true if math tree is valid, false otherwise
 */
//------------------------------------------------------------------------------
bool MathTree::Validate(std::string &msg)
{
   #ifdef DEBUG_VALIDATE
   MessageInterface::ShowMessage
      ("MathTree::Validate() entered, theWrapperMap=<%p>\n", theWrapperMap);
   #endif
   
   bool retval = true;
   msg = "";
   std::string errmsg = "Cannot use object ";
   
   if (theWrapperMap)
   {
      WrapperMap::iterator iter = theWrapperMap->begin();
      while (iter != theWrapperMap->end())
      {
         ElementWrapper *ew = iter->second;
         #ifdef DEBUG_VALIDATE
         MessageInterface::ShowMessage
            ("   name = '%s', wrapper = <%p>\n", (iter->first).c_str(), ew);
         #endif
         if (ew)
         {
            Gmat::WrapperDataType wrapperType = ew->GetWrapperType();
            #ifdef DEBUG_VALIDATE
            MessageInterface::ShowMessage("   wrapper type = %d\n", wrapperType);
            #endif
            
            // Check for invalid wrapper type (Allow Object type since GmatFunction can take
            // Object and return some value. LOJ: 2015.06.05)
            // if (wrapperType == Gmat::STRING_WT || wrapperType == Gmat::STRING_OBJECT_WT ||
            //     //wrapperType == Gmat::OBJECT_WT ||
            //     wrapperType == Gmat::BOOLEAN_WT ||
            //     wrapperType == Gmat::ON_OFF_WT || wrapperType == Gmat::UNKNOWN_WRAPPER_TYPE)
            // Just check for UNKNOWN_WRAPPER_TYPE (LOJ: 2015.09.21)
            if (wrapperType == Gmat::UNKNOWN_WRAPPER_TYPE)
            {
               #ifdef DEBUG_VALIDATE
               MessageInterface::ShowMessage
                  ("   '%s' has invalid wrapper type = %d\n", (iter->first).c_str(), wrapperType);
               #endif
               errmsg = errmsg + "\"" + iter->first + "\", ";
               retval = false;
            }
         }
         
         ++iter;
      }
   }
   
   if (!retval)
   {
      // Remove last , from the errmsg
      errmsg = GmatStringUtil::RemoveLastString(errmsg, ", ");
      msg = errmsg + " in a math equation.";
      // [] is no longer required for single output, so commented out (LOJ: 2015.09.21)
      // msg = errmsg + " in a math equation. If it is a GMAT function call, currently "
      //    "[] is required on the left hand side of equal sign.";
   }
   
   #ifdef DEBUG_VALIDATE
   MessageInterface::ShowMessage
      ("MathTree::Validate() returning %d, msg = '%s'\n", retval, msg.c_str());
   #endif
   
   return retval;
}

//------------------------------------------------------------------------------
// void Evaluate() const
//------------------------------------------------------------------------------
Real MathTree::Evaluate()
{
   #ifdef DEBUG_MATH_TREE_EVAL
   MessageInterface::ShowMessage
      ("MathTree::Evaluate() theTopNode=%s, %s\n", theTopNode->GetTypeName().c_str(),
       theTopNode->GetName().c_str());
   #endif
   return theTopNode->Evaluate();
}


//------------------------------------------------------------------------------
// Rmatrix *MatrixEvaluate()
//------------------------------------------------------------------------------
Rmatrix MathTree::MatrixEvaluate()
{
   #ifdef DEBUG_MATH_TREE
   MessageInterface::ShowMessage
      ("MathTree::MatrixEvaluate() theTopNode=%s, %s\n", theTopNode->GetTypeName().c_str(),
       theTopNode->GetName().c_str());
   #endif
   return theTopNode->MatrixEvaluate();
}


//------------------------------------------------------------------------------
// bool Initialize(ObjectMap *objectMap, ObjectMap *globalObjectMap)
//------------------------------------------------------------------------------
bool MathTree::Initialize(ObjectMap *objectMap, ObjectMap *globalObjectMap)
{
   isInitialized = false;
   // Null is an explicit absence of that scope, not an instruction to keep
   // pointers from an earlier caller. Keep storage when supplied maps are ours.
   const auto previousBindings = expressionBindings;
   if (!expressionBindings || (!expressionBindings->OwnsMap(objectMap) &&
         !expressionBindings->OwnsMap(globalObjectMap))) expressionBindings.reset();
   theObjectMap = objectMap;
   theGlobalObjectMap = globalObjectMap;
   for (MathNode *node : GetStructuralNodes())
      if (auto *runner = dynamic_cast<FunctionRunner*>(node))
         runner->SetExpressionObjectMaps(objectMap, globalObjectMap);
   if (!theTopNode) return true;
   isInitialized = InitializeParameter(theTopNode);
   return isInitialized;
}


//------------------------------------------------------------------------------
// void Finalize()
//------------------------------------------------------------------------------
void MathTree::Finalize()
{   
   if (theTopNode == NULL)
   {
      #ifdef DEBUG_MATH_TREE_FINALIZE
      MessageInterface::ShowMessage
         ("MathTree::Finalize() theTopNode is NULL, so just returning true\n");
      #endif
      return;
   }
   
   #ifdef DEBUG_MATH_TREE_FINALIZE
   MessageInterface::ShowMessage
      ("MathTree::Finalize() theTopNode=%s, %s\n", theTopNode->GetTypeName().c_str(),
       theTopNode->GetName().c_str());
   #endif
   
   FinalizeFunctionRunner(theTopNode);
}

void MathTree::RetireBindings()
{
   if (isRetiringBindings) return;
   isRetiringBindings = true;
   struct RetirementGuard
   {
      bool &retiring;
      ~RetirementGuard() { retiring = false; }
   } guard{isRetiringBindings};
   isInitialized = false;
   const auto nodes = GetStructuralNodes();
   std::exception_ptr firstError;
   // A shared argument node is visited once. Attempt every runner even if an
   // earlier one fails; all caller objects must still exist at this point.
   for (MathNode *node : nodes)
      if (auto *runner = dynamic_cast<FunctionRunner*>(node))
         try { runner->Finalize(); }
         catch (...) { if (!firstError) firstError = std::current_exception(); }
   for (MathNode *node : nodes)
      try { node->RetireExpressionReferences(); }
      catch (...) { if (!firstError) firstError = std::current_exception(); }
   theObjectMap = nullptr;
   theGlobalObjectMap = nullptr;
   theWrapperMap = nullptr;
   theFunctions.clear();
   expressionBindings.reset();
   if (firstError) std::rethrow_exception(firstError);
}


//------------------------------------------------------------------------------
// void GetOutputInfo(Integer &type, Integer &rowCount, Integer &colCount) const
//------------------------------------------------------------------------------
void MathTree::GetOutputInfo(Integer &type, Integer &rowCount, Integer &colCount) 
{
    theTopNode->GetOutputInfo(type, rowCount, colCount);
}


//------------------------------------------------------------------------------
//  void SetObjectMap(ObjectMap *map)
//------------------------------------------------------------------------------
/**
 * Called by the Assignment to set the local asset store used by the GmatCommand
 * 
 * @param <map> Pointer to the local object map
 */
//------------------------------------------------------------------------------
void MathTree::SetObjectMap(ObjectMap *map)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetObjectMap() map=%p\n", map);
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetObjectMapToRunner() theTopNode type='%s', desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetObjectMapToRunner(theTopNode, map);
   theObjectMap = map;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetObjectMap() returning\n");
   #endif
}


//------------------------------------------------------------------------------
//  void SetGlobalObjectMap(ObjectMap *map)
//------------------------------------------------------------------------------
/**
 * Called by the Assignment to set the global asset store used by the GmatCommand
 * 
 * @param <map> Pointer to the global object map
 */
//------------------------------------------------------------------------------
void MathTree::SetGlobalObjectMap(ObjectMap *map)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetGlobalObjectMap() map=%p\n", map);
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetGlobalObjectMapToRunner() theTopNode type='%s', desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetGlobalObjectMapToRunner(theTopNode, map);
   theGlobalObjectMap = map;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetGlobalObjectMap() returning\n");
   #endif
}


//------------------------------------------------------------------------------
// void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
void MathTree::SetSolarSystem(SolarSystem *ss)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetSolarSystem() ss=%p\n", ss);
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetSolarSystemToRunner() theTopNode type='%s', desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetSolarSystemToRunner(theTopNode, ss);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetSolarSystem() returning\n");
   #endif
}


//------------------------------------------------------------------------------
// void SetInternalCoordSystem(CoordinateSystem *cs)
//------------------------------------------------------------------------------
void MathTree::SetInternalCoordSystem(CoordinateSystem *cs)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetInternalCoordSystem() cs=%p\n", cs);
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetInternalCoordSystemToRunner() theTopNode type='%s', desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetInternalCoordSystemToRunner(theTopNode, cs);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetInternalCoordSystem() returning\n");
   #endif
}


//------------------------------------------------------------------------------
// void SetTransientForces(std::vector<PhysicalModel*> *tf)
//------------------------------------------------------------------------------
void MathTree::SetTransientForces(std::vector<PhysicalModel*> *tf)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetTransientForces() tf=%p\n", tf);
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetTransientForcesToRunner() theTopNode type=%s, desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetTransientForcesToRunner(theTopNode, tf);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetTransientForces() returning\n");
   #endif
}


//------------------------------------------------------------------------------
// void SetPublisher(Publisher *pub)
//------------------------------------------------------------------------------
void MathTree::SetPublisher(Publisher *pub)
{
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetPublisher() pub=<%p>\n", pub);
   #endif
   
   if (theTopNode == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   Calling SetPublisherToRunner() theTopNode type='%s', desc='%s'\n",
       theTopNode->GetTypeName().c_str(), theTopNode->GetName().c_str());
   #endif
   
   SetPublisherToRunner(theTopNode, pub);
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage("MathTree::SetPublisher() returning\n");
   #endif
}


//---------------------------------------------------------------------------
// virtual bool RenameRefObject(const UnsignedInt type,
//                 const std::string &oldName, const std::string &newName)
//---------------------------------------------------------------------------
/*
 * Renames referenced objects
 *
 * @param <type> type of the reference object.
 * @param <oldName> old name of the reference object.
 * @param <newName> new name of the reference object.
 *
 * @return always true to indicate RenameRefObject() was implemented.
 */
//---------------------------------------------------------------------------
bool MathTree::RenameRefObject(const UnsignedInt type,
                               const std::string &oldName,
                               const std::string &newName)
{
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage
      ("MathTree::RenameRefObject() oldName=<%s>, newName=<%s>\n", oldName.c_str(),
       newName.c_str());
   #endif
   
   if (theTopNode)
      RenameParameter(theTopNode, type, oldName, newName);
   
   return true;
}


//------------------------------------------------------------------------------
// virtual const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
const StringArray& MathTree::GetRefObjectNameArray(const UnsignedInt type)
{
   theAllParamArray.clear();
   
   if (theTopNode)
      CreateParameterNameArray(theTopNode);
   
   return theAllParamArray;
}

//------------------------------------------------------------------------------
// const StringArray& GetWrapperObjectNameArray(bool completeSet = false)
//------------------------------------------------------------------------------
const StringArray& MathTree::GetWrapperObjectNameArray(bool completeSet)
{
   theWrapperNames.clear();
   if (theTopNode)
      return theTopNode->GetWrapperObjectNameArray(completeSet);
   else
      return theWrapperNames;
}

//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the MathTree.
 *
 * @return clone of the MathTree.
 *
 */
//------------------------------------------------------------------------------
GmatBase* MathTree::Clone() const
{
   if (typeid(*this) != typeid(MathTree))
      throw MathException("Derived MathTree must implement its native Clone.\n");
   return (new MathTree(*this));
}


//------------------------------------------------------------------------------
// bool InitializeParameter(MathNode *node)
//------------------------------------------------------------------------------
bool MathTree::InitializeParameter(MathNode *root)
{
   bool result = true;
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
      auto associate = [&]() -> bool
      {
   if (node == NULL)
   {
      #ifdef DEBUG_MATH_TREE_INIT
      MessageInterface::ShowMessage
         ("MathTree::InitializeParameter() theTopNode is NULL, so just returning true\n");
      #endif
      return true;
   }
   
   #ifdef DEBUG_MATH_TREE_INIT
   MessageInterface::ShowMessage
      ("MathTree::InitializeParameter() node=%s, %s\n   IsFunction=%d, IsNumber=%d, "
       "IsFunctionInput=%d\n", node->GetTypeName().c_str(), node->GetName().c_str(),
       node->IsFunction(), node->IsNumber(), node->IsFunctionInput());
   #endif
   
   if (!node->IsFunction())
   {
      if (node->IsNumber())
         return true;
      
      if (node->IsFunctionInput())
         return true;
      
      // Now MathElement can have more than one ref objects due to GmatFunction
      // input arguments.
      StringArray refNames = node->GetRefObjectNameArray(Gmat::PARAMETER);
      std::string undefNames;
      
      for (UnsignedInt i=0; i<refNames.size(); i++)
      {
         #ifdef DEBUG_MATH_TREE_INIT
         MessageInterface::ShowMessage
            ("MathTree::InitializeParameter() refNames[%d]=%s\n", i, refNames[i].c_str());
         #endif
         
         // Handle array index
         Integer row, col;
         std::string newName;
         GmatStringUtil::GetArrayIndex(refNames[i], row, col, newName);
         
         if (theObjectMap && theObjectMap->find(newName) != theObjectMap->end())
         {
            node->SetRefObject((*theObjectMap)[newName], Gmat::PARAMETER,
                               newName);
            
            #ifdef DEBUG_MATH_TREE_INIT
            MessageInterface::ShowMessage
               ("MathTree::InitializeParameter() Found %s from theObjectMap\n",
                refNames[i].c_str());
            #endif
         }
         else if (theGlobalObjectMap && theGlobalObjectMap->find(newName) != theGlobalObjectMap->end())
         {
            node->SetRefObject((*theGlobalObjectMap)[newName], Gmat::PARAMETER,
                               newName);
            
            #ifdef DEBUG_MATH_TREE_INIT
            MessageInterface::ShowMessage
               ("MathTree::InitializeParameter() Found %s from theGlobalObjectMap\n",
                refNames[i].c_str());
            #endif
         }
         else
         {
            #ifdef DEBUG_MATH_TREE_INIT
            MessageInterface::ShowMessage
               ("MathTree::InitializeParameter() Unable to find " + newName +
                " from theObjectMap or theGlobalObjectMap\n");
            #endif

            if (undefNames == "")
               undefNames = newName;
            else
               undefNames = undefNames + ", " + newName;
         }
      }
      
      if (undefNames == "")
         return true;
      else
         throw InterpreterException("Undefined variable(s) \"" + undefNames + "\" used in MathTree");
   }
   else
   {
      return true;
   }

      };
      result = associate() && result;
   });
   return result;
}


//------------------------------------------------------------------------------
// void FinalizeFunctionRunner(MathNode *node)
//------------------------------------------------------------------------------
void MathTree::FinalizeFunctionRunner(MathNode *root)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::FinalizeFunctionRunner() node=%p\n", node);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', name='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->Finalize();
   
   

   });
}


//------------------------------------------------------------------------------
// void SetMathElementWrappers(MathNode *node)
//------------------------------------------------------------------------------
void MathTree::SetMathElementWrappers(MathNode *root)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_MATH_WRAPPERS
   MessageInterface::ShowMessage
      ("MathTree::SetMathElementWrappers() entered, node=<%s>\n",
       node ? node->GetName().c_str() : "NULL");
   #endif
   
   if (node == NULL)
      return;
   
   if (!node->IsFunction())
   {
      if (node->IsNumber())
         return;
      
      #ifdef DEBUG_MATH_WRAPPERS
      MessageInterface::ShowMessage
         ("   Setting wrappers to '%s'\n", node->GetName().c_str());
      #endif
      
      node->SetMathWrappers(theWrapperMap);
   }
   else
   {
      if (node->IsOfType("BuiltinFunctionNode"))
      {
         #ifdef DEBUG_MATH_WRAPPERS
         MessageInterface::ShowMessage
            ("   Setting wrappers to '%s'\n", node->GetName().c_str());
         #endif
         
         node->SetMathWrappers(theWrapperMap);
      }
      else
      {
         
      }
   }

   });
}


//------------------------------------------------------------------------------
// void SetFunctionToRunner(MathNode *node, Function *function)
//------------------------------------------------------------------------------
void MathTree::SetFunctionToRunner(MathNode *root, Function *function)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetFunctionToRunner() node=%p, function=%p\n", node, function);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', name='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetFunction(function);
   
   

   });
}

//------------------------------------------------------------------------------
// void SetCallingFunctionToRunner(MathNode *node, Function *fm)
//------------------------------------------------------------------------------
void MathTree::SetCallingFunctionToRunner(MathNode *root, FunctionManager *fm)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetCallingFunctionToRunner() node=%p, function=%p\n", node, fm);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', name='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetCallingFunction(fm);
   
   

   });
}


//------------------------------------------------------------------------------
// void SetObjectMapToRunner(MathNode *node, ObjectMap *map)
//------------------------------------------------------------------------------
void MathTree::SetObjectMapToRunner(MathNode *root, ObjectMap *map)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetObjectMapToRunner() node=%p, map=%p\n", node, map);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', desc='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetObjectMap(map);
   
   

   });
}


//------------------------------------------------------------------------------
// void SetGlobalObjectMapToRunner(MathNode *node, ObjectMap *map)
//------------------------------------------------------------------------------
void MathTree::SetGlobalObjectMapToRunner(MathNode *root, ObjectMap *map)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetGlobalObjectMapToRunner() node=%p, map=%p\n", node, map);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', desc='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetGlobalObjectMap(map);
   
   

   });
}


//------------------------------------------------------------------------------
// void SetSolarSystemToRunner(MathNode *node, SolarSystem *ss)
//------------------------------------------------------------------------------
void MathTree::SetSolarSystemToRunner(MathNode *root, SolarSystem *ss)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetSolarSystemToRunner() node=%p, ss=%p\n", node, ss);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', desc='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetSolarSystem(ss);
   
   

   });
}


//------------------------------------------------------------------------------
// void SetInternalCoordSystemToRunner(MathNode *node, CoordinateSystem *cs)
//------------------------------------------------------------------------------
void MathTree::SetInternalCoordSystemToRunner(MathNode *root, CoordinateSystem *cs)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetInternalCoordSystemToRunner() node=%p, cs=%p\n", node, cs);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', desc='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetInternalCoordSystem(cs);
   
   

   });
}


//------------------------------------------------------------------------------
// void SetTransientForcesToRunner(MathNode *node, std::vector<PhysicalModel*> *tf)
//------------------------------------------------------------------------------
void MathTree::SetTransientForcesToRunner(MathNode *root, std::vector<PhysicalModel*> *tf)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetTransientForcesToRunner() node=%p, tf=%p\n", node, tf);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', desc='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetTransientForces(tf);
   
   

   });
}


//------------------------------------------------------------------------------
// void SetPublisherToRunner(MathNode *node, Publisher *pub)
//------------------------------------------------------------------------------
void MathTree::SetPublisherToRunner(MathNode *root, Publisher *pub)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("MathTree::SetPublisherToRunner() node=<%p>, pub=<%p>\n", node, pub);
   #endif
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_FUNCTION
   MessageInterface::ShowMessage
      ("   node type='%s', desc='%s'\n", node->GetTypeName().c_str(),
       node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
      return;
   
   if (node->IsOfType("FunctionRunner"))
      ((FunctionRunner*)(node))->SetPublisher(pub);
   
   

   });
}


//------------------------------------------------------------------------------
// bool RenameParameter(MathNode *node, const UnsignedInt type,
//                      const std::string &oldName, const std::string &newName)
//------------------------------------------------------------------------------
bool MathTree::RenameParameter(MathNode *root, const UnsignedInt type,
                               const std::string &oldName,
                               const std::string &newName)
{
   bool result = true;
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
      auto associate = [&]() -> bool
      {
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage
      ("MathTree::RenameParameter() oldName=<%s>, newName=<%s>, node=<%s>\n",
       oldName.c_str(), newName.c_str(), node ? node->GetName().c_str() : "NULL");
   #endif
   
   if (node == NULL)
      return true;
   
   std::string nodeName = node->GetName();
   
   if (nodeName.find(oldName) != nodeName.npos)
   {
      #ifdef DEBUG_RENAME
      MessageInterface::ShowMessage("   old nodeName=<%s>\n", nodeName.c_str());
      #endif
      
      nodeName = GmatStringUtil::ReplaceName(nodeName, oldName, newName);
      node->SetName(nodeName);
      
      #ifdef DEBUG_RENAME
      MessageInterface::ShowMessage("   new nodeName=<%s>\n", node->GetName().c_str());
      #endif
   }
   
   if (!node->IsFunction())
   {
      if (!node->IsNumber())
         node->RenameRefObject(type, oldName, newName);
   }
   else
   {
      //loj: ================ try getting node name here
      
   }
   
   return true;

      };
      result = associate() && result;
   });
   return result;
}


//------------------------------------------------------------------------------
// void CreateParameterNameArray(MathNode *node)
//------------------------------------------------------------------------------
void MathTree::CreateParameterNameArray(MathNode *root)
{
   VisitStructuralMathNodes(root, [&](MathNode *node)
   {
   theParamArray.clear();
   
   if (node == NULL)
      return;
   
   #ifdef DEBUG_MATH_WRAPPERS
   MessageInterface::ShowMessage
      ("MathTree::CreateParameterNameArray() node=%s\n", node->GetName().c_str());
   #endif
   
   if (!node->IsFunction())
   {
      // if math element is not a number
      if (!node->IsNumber())
      {
         StringArray refs = node->GetRefObjectNameArray(Gmat::PARAMETER);
         for (UnsignedInt i=0; i<refs.size(); i++)
         {
            #ifdef DEBUG_MATH_WRAPPERS
            MessageInterface::ShowMessage("   [%d] %s\n", i, refs[i].c_str());
            #endif
            
            // add if not found in the all ref array
            if (find(theAllParamArray.begin(), theAllParamArray.end(), refs[i]) ==
                theAllParamArray.end())
               theAllParamArray.push_back(refs[i]);
         }
      }
   }
   else
   {
      #ifdef __USE_ARGLIST_FOR_REFNAMES__
      // add function input arguments if node is a FunctionRunner
      if (node->IsOfType("FunctionRunner"))
      {
         #ifdef DEBUG_MATH_WRAPPERS
         MessageInterface::ShowMessage
            ("   It is a FunctionRunner of '%s'\n", node->GetName().c_str());
         #endif
         
         StringArray inputs = ((FunctionRunner*)node)->GetInputs();
         for (UnsignedInt i=0; i<inputs.size(); i++)
         {
            #ifdef DEBUG_MATH_WRAPPERS
            MessageInterface::ShowMessage("   inputs[%d]='%s'\n", i, inputs[i].c_str());
            #endif
            
            if (inputs[i] == "")
               continue;
            
            if (find(theAllParamArray.begin(), theAllParamArray.end(), inputs[i]) ==
                theAllParamArray.end())
               theAllParamArray.push_back(inputs[i]);
         }
      }
      #endif
      
      
   }

   });
}


//------------------------------------------------------------------------------
// void DeleteNode(MathNode *node)
//------------------------------------------------------------------------------
void MathTree::DeleteNode(MathNode *node)
{
   for (MathNode *item : StructuralMathNodes(node, false))
      if (std::find(nodesToDelete.begin(), nodesToDelete.end(), item) == nodesToDelete.end())
         nodesToDelete.push_back(item);
}



std::vector<MathNode*> MathTree::GetStructuralNodes() const
{
   return StructuralMathNodes(theTopNode, true);
}

void MathTree::Copy(const GmatBase *source)
{
   const auto *tree = dynamic_cast<const MathTree*>(source);
   if (!tree || typeid(*tree) != typeid(MathTree) || typeid(*this) != typeid(MathTree))
      throw MathException("Cannot copy incompatible MathTree.\n");
   operator=(*tree);
}

void MathTree::GetClonedOwnedObjects(const MathTree &target,
      ObjectArray &originals, ObjectArray &copies) const
{
   const auto from = GetStructuralNodes();
   const auto to = target.GetStructuralNodes();
   if (this == &target || typeid(*this) != typeid(target) || from.size() != to.size())
      throw MathException("Incompatible expression ownership graph.\n");
   std::map<MathNode*, MathNode*> nodes;
   for (size_t i = 0; i < from.size(); ++i)
   {
      if (from[i] == to[i] || typeid(*from[i]) != typeid(*to[i]))
         throw MathException("Missing independently copied expression node.\n");
      nodes.emplace(from[i], to[i]);
   }
   for (size_t i = 0; i < from.size(); ++i)
   {
      const auto children = from[i]->GetStructuralChildren();
      const auto values = to[i]->GetStructuralChildren();
      if (children.size() != values.size()) throw MathException("Changed expression child graph.\n");
      for (size_t j = 0; j < children.size(); ++j)
         if ((children[j] ? nodes.at(children[j]) : nullptr) != values[j])
            throw MathException("Changed expression child alias.\n");
   }
   auto append = [&](GmatBase *source, GmatBase *value)
   {
      const auto found = std::find(originals.begin(), originals.end(), source);
      if (found == originals.end()) { originals.push_back(source); copies.push_back(value); }
      else if (copies[found - originals.begin()] != value)
         throw MathException("Conflicting owned expression copies.\n");
   };
   append(const_cast<MathTree*>(this), const_cast<MathTree*>(&target));
   for (size_t i = 0; i < from.size(); ++i) append(from[i], to[i]);
}

void MathTree::RebindClonedReferences(const MathTree &source,
      const std::function<GmatBase*(GmatBase*)> &resolve, WrapperMap *wrappers)
{
   isInitialized = false;
   ExpressionBindingScope binding;
   ObjectArray originals, copies;
   source.GetClonedOwnedObjects(*this, originals, copies);
   auto *local = binding.scopes->CopyMap(source.theObjectMap, resolve);
   const bool valueGlobal = source.theGlobalObjectMap == source.theObjectMap ||
         (source.expressionBindings && source.expressionBindings->OwnsMap(source.theGlobalObjectMap));
   auto *global = valueGlobal ? binding.scopes->CopyMap(source.theGlobalObjectMap, resolve) :
         source.theGlobalObjectMap;
   std::vector<Function*> functions;
   for (Function *function : source.theFunctions)
      functions.push_back(BindExpressionReference(function, resolve));
   const auto from = source.GetStructuralNodes();
   const auto to = GetStructuralNodes();
   for (size_t i = 0; i < from.size(); ++i)
      to[i]->RebindExpressionReferences(*from[i], resolve, wrappers);
   expressionBindings = binding.scopes;
   theObjectMap = local;
   theGlobalObjectMap = global;
   theWrapperMap = wrappers;
   theFunctions.swap(functions);
   isInitialized = ExpressionBindingScope::preserveReadiness && source.isInitialized;
}
