//$Id$
//------------------------------------------------------------------------------
//                                  ObjectWrapper
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. 
//
// Author: Linda Jun/GSFC
// Created: 2007/07/24
//
/**
 * Declares ObjectWrapper class.
 */
//------------------------------------------------------------------------------

#include "ObjectWrapper.hpp"
#include "ParameterException.hpp"
#include "StringUtil.hpp"         // for ToObject()
#include "MessageInterface.hpp"
#include "Subscriber.hpp"

//#define DEBUG_OBJECT_WRAPPER

//---------------------------------
// public methods
//---------------------------------

//------------------------------------------------------------------------------
// ObjectWrapper()
//------------------------------------------------------------------------------
/**
 * Constructs ObjectWrapper structures
 * (default constructor).
 *
 */
//------------------------------------------------------------------------------
ObjectWrapper::ObjectWrapper() :
   ElementWrapper()
{
   theObject   = NULL;
   wrapperType = Gmat::OBJECT_WT;
}


//------------------------------------------------------------------------------
// ObjectWrapper(const ObjectWrapper &copy);
//------------------------------------------------------------------------------
/**
 * Constructs base ObjectWrapper structures used in derived classes, by 
 * copying the input instance (copy constructor).
 *
 * @param <copy>  ObjectWrapper instance to copy to create "this" instance.
 */
//------------------------------------------------------------------------------
ObjectWrapper::ObjectWrapper(const ObjectWrapper &copy) :
   ElementWrapper(copy), theObject(nullptr)
{
   if (copy.ownedObject)
      ownedObject = CloneOwnedReference(copy.theObject);
   theObject = ownedObject ? ownedObject.get() : copy.theObject;
}


//------------------------------------------------------------------------------
// ObjectWrapper& operator=(const ObjectWrapper &right)
//------------------------------------------------------------------------------
/**
 * Assignment operator for ObjectWrapper structures.
 *
 * @param <right> The original that is being copied.
 *
 * @return Reference to this object
 */
//------------------------------------------------------------------------------
const ObjectWrapper& ObjectWrapper::operator=(const ObjectWrapper &right)
{
   if (&right == this) return *this;
   auto value = right.ownedObject ? CloneOwnedReference(right.theObject, theObject) : std::unique_ptr<GmatBase>();
   ElementWrapper::operator=(right);
   // The source can borrow this destination's owned result. Keep that owner
   // when assignment reconnects the same identity.
   if (!ownedObject || right.theObject != ownedObject.get() || right.ownedObject)
      ownedObject = std::move(value);
   theObject = ownedObject ? ownedObject.get() : right.theObject;
   return *this;
}


//------------------------------------------------------------------------------
// ~ObjectWrapper()
//------------------------------------------------------------------------------
/**
 * Destructor.
 */
//------------------------------------------------------------------------------
ObjectWrapper::~ObjectWrapper()
{
}


//------------------------------------------------------------------------------
// ElementWrapper* ObjectWrapper::Clone() const
//------------------------------------------------------------------------------
/**
 * Creates a copy of this wrapper
 *
 * @return Te new copy
 */
//------------------------------------------------------------------------------
ElementWrapper* ObjectWrapper::Clone() const
{
   return new ObjectWrapper(*this);
}

void ObjectWrapper::RebindClonedReferences(const ElementWrapper &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   CheckCloneType(source);
   const auto &original = static_cast<const ObjectWrapper&>(source);
   auto *value = BindClonedReference(original.theObject, resolve);
   if (value != theObject)
   {
      ownedObject.reset();
      theObject = value;
   }
}



//------------------------------------------------------------------------------
// std::string ToString()
//------------------------------------------------------------------------------
/**
 * @return ObjectWrapper value converted to std::string.
 *
 * @exception <GmatBaseException> thrown if this method is called.
 */
//------------------------------------------------------------------------------
std::string ObjectWrapper::ToString()
{
   return theObject->GetGeneratingString(Gmat::MATLAB_STRUCT);
}


//------------------------------------------------------------------------------
// Gmat::ParameterType GetDataType() const
//------------------------------------------------------------------------------
/**
 * This method returns the data type for the ObjectWrapper object.
 *
 * @return data type for the object.
 *
 */
//------------------------------------------------------------------------------
Gmat::ParameterType ObjectWrapper::GetDataType() const
{
   return Gmat::OBJECT_TYPE;
}


//---------------------------------------------------------------------------
// GmatBase* GetRefObject(const std::string &name = "")
//---------------------------------------------------------------------------
/*
 * @see ElementWrapper
 */
//---------------------------------------------------------------------------
GmatBase* ObjectWrapper::GetRefObject(const std::string &name)
{
   return theObject;
}


//---------------------------------------------------------------------------
//  bool SetRefObject(GmatBase *obj)
//---------------------------------------------------------------------------
/**
 * Method to set the reference object (Parameter) pointer on the wrapped 
 * object.
 *
 * @return true if successful; false otherwise.
 */
//---------------------------------------------------------------------------
bool ObjectWrapper::SetRefObject(GmatBase *obj)
{
   if (!obj || refObjectNames.empty() || obj->GetName() != refObjectNames[0]) return false;
   if (obj != theObject)
   {
      ownedObject.reset();
      theObject = obj;
   }
   return true;
}


//---------------------------------------------------------------------------
//  bool RenameObject(const std::string &oldName, const std::string &newName)
//---------------------------------------------------------------------------
/**
 * Method to rename a reference object for the wrapper.
 *
 * @return true if successful; false otherwise.
 */
//---------------------------------------------------------------------------
bool ObjectWrapper::RenameObject(const std::string &oldName, 
                                 const std::string &newName)
{
   ElementWrapper::RenameObject(oldName, newName);
   description = refObjectNames[0];  
   return true;
}


//---------------------------------------------------------------------------
// Real EvaluateReal() const
//---------------------------------------------------------------------------
/**
 * Method to return the Real value of the wrapped object.
 *
 * @return Real value of the wrapped number object.
 * 
 * @note This method is not value for Strings.
 * 
 */
//---------------------------------------------------------------------------
Real ObjectWrapper::EvaluateReal() const
{
   throw ParameterException(
      "EvaluateReal() method not valid for wrapper of Object type.\n");
}


//---------------------------------------------------------------------------
// bool SetReal(const Real toValue)
//---------------------------------------------------------------------------
/**
 * Method to set the Real value of the wrapped object.
 *
 * @return true if successful; false otherwise.
 * 
 * @note This method is not valid for Object.
 */
//---------------------------------------------------------------------------
bool ObjectWrapper::SetReal(const Real toValue)
{
   throw ParameterException(
      "SetReal() method not valid for wrapper of Object type.\n");
}


//------------------------------------------------------------------------------
// GmatBase* EvaluateObject() const
//------------------------------------------------------------------------------
/**
 * Method to return the Object pointer of the ObjectWrapper object.
 *
 * @return value of the ObjectWrapper object.
 * 
 */
//------------------------------------------------------------------------------
GmatBase* ObjectWrapper::EvaluateObject() const
{
   return theObject;
}


//------------------------------------------------------------------------------
// bool SetObject(GmatBase *obj)
//------------------------------------------------------------------------------
/**
 * Method to set the object of the wrapped object.
 *
 * @param <obj> The object pointer to set
 * @return true if successful; false otherwise.
 */
//------------------------------------------------------------------------------
bool ObjectWrapper::SetObject(GmatBase *obj)
{
   if (obj == NULL)
   {
      if (theObject == NULL)
         throw ParameterException("Cannot set undefined object to undefined object");
      else
         throw ParameterException
            ("Cannot set undefined object to object of type \"" +
             theObject->GetTypeName() +  "\"");         
   }
   
   if (theObject != NULL)
   {
      // Let's check the object type
      if (theObject->GetTypeName() == obj->GetTypeName())
      {
         #ifdef DEBUG_OBJECT_WRAPPER
         MessageInterface::ShowMessage
            ("ObjectWrapper::SetObject() fromType=%s, toType=%s\n",
             obj->GetTypeName().c_str(), theObject->GetTypeName().c_str());
         #endif
         
         theObject->Copy(obj);
         if (theObject != obj)
            if (auto *subscriber = dynamic_cast<Subscriber*>(theObject))
               subscriber->CommitFunctionValueBindings();
         // A station value copy owns fresh attachments and detached local
         // frames. Make a copied output/direct assignment usable by subsequent
         // commands while leaving a shared global identity assignment alone.
         if (theObject != obj && theObject->IsOfType(Gmat::GROUND_STATION) &&
             !theObject->IsInitialized() && !theObject->Initialize())
            throw ParameterException("Cannot initialize copied GroundStation \"" +
                  theObject->GetName() + "\"");
      }
      else
      {
         ParameterException pe;
         pe.SetDetails("Cannot set object of type \"%s\" to object of type \"%s\"",
                       obj->GetTypeName().c_str(), theObject->GetTypeName().c_str());
         throw pe;
      }
   }
   else
   {
      throw ParameterException("Cannot set object \"" + obj->GetName() +
                               "\" to an undefined object");
   }
   
   return true;
}


//---------------------------------
// protected methods
//---------------------------------

//------------------------------------------------------------------------------
// void SetupWrapper()
//------------------------------------------------------------------------------
/**
 * Method to set up the Object Wrapper.
 *
 */
//------------------------------------------------------------------------------
void ObjectWrapper::SetupWrapper()
{
   refObjectNames.push_back(description);
}

// Explicitly take ownership of a newly generated expression result. Ordinary
// SetRefObject still borrows configured/runtime objects.
bool ObjectWrapper::OwnsRefObject() const { return ownedObject && ownedObject.get() == theObject; }

void ObjectWrapper::AdoptRefObject(std::unique_ptr<GmatBase> value)
{
   if (value && value.get() == ownedObject.get())
   {
      value.release();
      throw GmatBaseException("Expression result is already owned by this wrapper.\n");
   }
   if (!value || refObjectNames.empty() || value->GetName() != refObjectNames[0])
      throw GmatBaseException("Expression result does not match its wrapper.\n");
   ownedObject = std::move(value);
   theObject = ownedObject.get();
}
