//$Id$
//------------------------------------------------------------------------------
//                                UserDefinedFunction
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
 * Defines UserDefinedFunction class. UserDefinedFunction is created from
 * the associated function file and function resources and commands are created.
 * All user defined function classes such as GmatFunction are derived from this class.
 */
//------------------------------------------------------------------------------
#ifndef UserDefinedFunction_hpp
#define UserDefinedFunction_hpp

#include "ObjectManagedFunction.hpp"
#include <memory>

struct FunctionDefinitionObjects;

class GMAT_API UserDefinedFunction : public ObjectManagedFunction
{
public:
   static constexpr unsigned int NativeValueProviderDepth = ObjectManagedFunction::NativeValueProviderDepth + 1;

   UserDefinedFunction(const std::string &typeStr, const std::string &nomme);
   virtual ~UserDefinedFunction();
   UserDefinedFunction(const UserDefinedFunction &f);
   UserDefinedFunction& operator=(const UserDefinedFunction &f);
   
   virtual bool         Initialize(ObjectInitializer *objInit,
                                   bool reinitialize = false,
                                   bool resetLocalObjects = true);
   virtual bool         Execute(ObjectInitializer *objInit, bool reinitialize = false);
   virtual void         Finalize(bool cleanUp = false);
   std::unique_ptr<FunctionInvocationState> SuspendInvocationState() override;
   
   virtual bool         IsFcsFinalized();
   
   virtual bool         IsFunctionControlSequenceSet();
   // Accept initial ownership or the same sequence again. Replacing an already
   // interpreted sequence requires a fresh rebuilt resource, as source setters do.
   virtual bool         SetFunctionControlSequence(GmatCommand *cmd);
   virtual GmatCommand* GetFunctionControlSequence();
   // Retire a native-owned sequence. Cold definition copies need no RunComplete.
   void ClearFunctionControlSequence(bool callRunComplete = true);
   void RetireFunctionBindings(ObjectMap *store);
   void RetireAutomaticReferences(ObjectMap *store);
   void RetireSandboxReferences(const ObjectMap &objects);
   void PruneExpiredAutomaticReferences();
   ObjectArray GetFunctionValueOwnedObjectsImpl() const;

   // Methods for objects created in the function via Create
   virtual void         ClearFunctionObjects();
   virtual void         AddFunctionObject(GmatBase *obj);
   virtual GmatBase*    FindFunctionObject(const std::string &name);
   virtual ObjectMap*   GetFunctionObjectMap();
   
   // methods to set/get the automatic objects
   virtual void         ClearAutomaticObjects();
   virtual void         AddAutomaticObject(const std::string &withName, GmatBase *obj,
                                           bool alreadyManaged);
   virtual GmatBase*    FindAutomaticObject(const std::string &name);
   virtual ObjectMap*   GetAutomaticObjectMap();
   
   DEFAULT_TO_NO_CLONES
   DEFAULT_TO_NO_REFOBJECTS

protected:
   
   /// the function control sequence
   GmatCommand          *fcs;
   /// have the commands in the FCS been initialized?
   bool                 fcsInitialized;
   /// have the commands in the FCS been finalized?
   bool                 fcsFinalized;

   /// Object store obtained from the Sandbox
   ////ObjectMap            *globalObjectStore;
   /// Map to hold objects created in function
   ObjectMap            functionObjectMap;
   
   /// objects automatically created on parsing (but for whom a references object cannot be
   /// set at that time)
   ObjectMap            automaticObjectMap;
   // Validator used to create the ElementWrappers
   Validator            *validator;
   /// Object store needed by the validator
   ObjectMap            validatorStore;
   
   /// Objects already in the Sandbox object map
   ObjectArray          sandboxObjects;
   /// Legacy storage retained for binary layout compatibility. Owned output
   /// wrappers use ObjectManagedFunction::wrappersToDelete and its cleanup.
   WrapperArray         wrappersToDelete;
   /// Object array to delete
   ObjectArray          objectsToDelete;
   
   /// the flag indicating local objects are initialized
   bool                 objectsInitialized;

   // Prototypes cloned with a command definition have one owner even when
   // several declaration/automatic keys alias the same actual object.
   std::unique_ptr<FunctionDefinitionObjects> definitionObjects;
   void CopyDefinitionGraph(const UserDefinedFunction &source,
         UserDefinedFunction *definitionOwner = nullptr);
   void SwapDefinitionState(UserDefinedFunction &other) noexcept;
   bool OwnsDefinitionObject(const GmatBase *object) const;
   bool IsBorrowedAutomaticObject(const GmatBase *object) const;
   bool IsLiveAutomaticObject(const std::string &name, const GmatBase *object) const;
   void RebuildBorrowedAutomaticObjects();

   // Ordinary assignment to an already interpreted live resource retains the
   // existing source-cache lifecycle. Cold copies also retain compiled syntax
   // and prototype ownership, but rebuild invocation wrappers and runtime stores.
   bool HasSameDefinition(const UserDefinedFunction &other) const;
   
   GmatBase* FindObject(const std::string &name);
   bool      IsAutomaticObjectGlobal(const std::string &autoObjName, GmatBase **owner);
   
   // for debug
   void ShowObjects(const std::string &title);

};

#endif // UserDefinedFunction_hpp
