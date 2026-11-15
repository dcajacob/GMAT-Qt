//$Id$
//------------------------------------------------------------------------------
//                                  ElementWrapper
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
// number NNG04CC06P
//
// Author: Wendy C. Shoan/GSFC/MAB
// Created: 2007.01.17
//
/**
 * Definition of the ElementWrapper class.  This is the base class for wrappers 
 * for various element types.
 *
 * @note    As of 2007.01.17, derived classes handle elements of various 
 * parameter types - therefore, the derived class code is located in the
 * parameter subsystem.
 *
 */
//------------------------------------------------------------------------------

#ifndef ElementWrapper_hpp
#define ElementWrapper_hpp

#include "gmatdefs.hpp"
#include "GmatBase.hpp"
#include "Rmatrix.hpp"
#include <typeinfo>

// forward references
class SolarSystem;
class Parameter;
struct ElementWrapperInvocationLease;

class GMAT_API ElementWrapper
{
public:

   // default constructor
   ElementWrapper();
   // copy constructor
   ElementWrapper(const ElementWrapper &er);
   // operator = 
   const ElementWrapper& operator=(const ElementWrapper &er);
   // destructor
   virtual ~ElementWrapper();

   using InvocationLease = std::shared_ptr<ElementWrapperInvocationLease>;
   static InvocationLease RetainInvocationWrapper(ElementWrapper *wrapper);
   static ElementWrapper *GetInvocationWrapper(const InvocationLease &lease) noexcept;
   static bool MatchesInvocationWrapper(const InvocationLease &lease,
         const ElementWrapper *wrapper) noexcept;
   static void ReleaseOwnedWrapper(ElementWrapper *wrapper) noexcept;
   
   virtual std::string        ToString();
   virtual const StringArray& GetPropertyNames();
   virtual Integer            GetPropertyId();
   virtual void               AllowOneDimArraySetting(bool allow);
   virtual bool               IsOneDimArraySettingAllowed();
   
   // Abstract to !!force!! implementation in derived classes
   /// todo: Remove implementation in .cpp file, since all it does is throw;
   ///       this is the better approach because it FORCES the developers to
   ///       implement the method.
   virtual ElementWrapper*    Clone() const = 0;
   virtual void               SetDescription(const std::string &str);
   virtual std::string        GetDescription() const;
   Gmat::WrapperDataType      GetWrapperType() const;
   
   virtual void               ClearRefObjectNames();
   virtual const StringArray& GetRefObjectNames();
   virtual bool               SetRefObjectName(const std::string &name,
                                               Integer index = 0);
   virtual GmatBase*          GetRefObject(const std::string &name = "");
   virtual bool               SetRefObject(GmatBase *obj);
   
   virtual bool               RenameObject(const std::string &oldName, 
                                           const std::string &newName);
   
   // This method tells a wrapper's object to take whatever action it needs to take
   // before the value for the specified parameter is set (e.g. clearing arrays).
   // This must be implemented in any wrapper type that handles a parameter
   // for which pre-action(s) must be taken.
   virtual bool               TakeRequiredAction() const;



   // static functions
   static bool SetValue(ElementWrapper *lhsWrapper, ElementWrapper *rhsWrapper,
                        SolarSystem *solarSys, ObjectMap *objMap,
                        ObjectMap *globalObjMap, bool setRefObj = true);
   
   static GmatBase* FindObject(const std::string &name, SolarSystem *solarSys,
                               ObjectMap *objMap, ObjectMap *globalObjMap);
   
   static void ShowObjectMaps(ObjectMap *objMap, ObjectMap *globalObjMap);
   
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
   virtual Gmat::ParameterType GetDataType() const = 0;
   
//---------------------------------------------------------------------------
//  Real EvaluateReal() const
//---------------------------------------------------------------------------
/**
 * Method to return the Real value of the wrapped object.
 *
 * @return Real value of the wrapped object.
 * 
 * @note  This is a pure virtual (abstract) method and must be implemented in
 * all leaf classes derived from this one.
 */
//---------------------------------------------------------------------------
   virtual Real            EvaluateReal() const = 0;

   // Calculation paths must distinguish a failed system-parameter evaluation
   // from a valid finite value. Legacy evaluation/formatting remains available
   // for metadata registration and reporting.
   Real EvaluateRealForCalculation();
   // Range endpoints and unlimited step sizes may use infinity, but never NaN
   // or a failed native parameter evaluation.
   Real EvaluateRealForRange();
   static Real EvaluateParameterRealForCalculation(Parameter *parameter);
   
//---------------------------------------------------------------------------
//  bool SetReal(const Real value)
//---------------------------------------------------------------------------
/**
 * Method to set the Real value of the wrapped object.
 *
 * @return true if successful; false otherwise.
 * 
 * @note  This is a pure virtual (abstract) method and must be implemented in
 * all leaf classes derived from this one.
 */
//---------------------------------------------------------------------------
   virtual bool            SetReal(const Real toValue) = 0;
   
   virtual const Rmatrix&  EvaluateArray() const;
   virtual bool            SetArray(const Rmatrix &toValue); 
   virtual const Rvector&  EvaluateRvector() const;
   virtual bool            SetRvector(const Rvector &toValue); 
   virtual std::string     EvaluateString() const;
   virtual bool            SetString(const std::string &toValue); 
   virtual std::string     EvaluateOnOff() const;
   virtual bool            SetOnOff(const std::string &toValue); 
   virtual bool            EvaluateBoolean() const;
   virtual bool            SetBoolean(const bool toValue); 
   virtual Integer         EvaluateInteger() const;
   virtual bool            SetInteger(const Integer toValue); 
   virtual GmatBase*       EvaluateObject() const;
   virtual bool            SetObject(GmatBase *obj); 

   // Passive retained-value binding. The declaration/description remains a
   // lexical role; actual objects are resolved by identity without evaluation.
   virtual void RebindClonedReferences(const ElementWrapper &source,
         const std::function<GmatBase*(GmatBase*)> &resolve);
   virtual bool OwnsRefObject() const;
   void CollectOwnedReferences(ObjectArray &objects) const;
   void ValidateOwnedWrapperGraph() const;
   void CollectClonedOwnedReferences(const ElementWrapper &source,
         ObjectArray &originals, ObjectArray &copies) const;
   void CollectClonedOwnedReferences(const ElementWrapper &source,
         ObjectArray &originals, ObjectArray &copies, bool sourceOwnedOnly) const;
   static std::unique_ptr<ElementWrapper> CloneValue(const ElementWrapper *source,
         const ElementWrapper *previous = NULL);
   
protected:  

   void CheckCloneType(const ElementWrapper &source) const;
   template <class T> static std::unique_ptr<T> CloneOwnedReference(const T *source,
         const T *previous = NULL)
   {
      if (!source) return {};
      std::unique_ptr<GmatBase> copy(source->Clone());
      if (copy.get() == source || (previous && copy.get() == previous))
      {
         copy.release();
         throw GmatBaseException("Expression value Clone returned an existing object.\n");
      }
      T *value = dynamic_cast<T*>(copy.get());
      if (!value || typeid(*value) != typeid(*source))
         throw GmatBaseException("Cannot copy expression value with its native type.\n");
      copy.release();
      return std::unique_ptr<T>(value);
   }
   template <class T> static T *BindClonedReference(T *source,
         const std::function<GmatBase*(GmatBase*)> &resolve)
   {
      if (!source) return NULL;
      T *value = dynamic_cast<T*>(resolve(source));
      if (!value) throw GmatBaseException("Invalid retained expression reference.\n");
      return value;
   }

   static const Real UNDEFINED_REAL;
   /// Description of the wrapper
   std::string           description;
   StringArray           refObjectNames;
   
   Gmat::WrapperDataType wrapperType;
   
   /// flag indicating if one dimentional row or column vector can be set each other
   bool allowOneDimArraySetting;

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
   virtual void            SetupWrapper() = 0;
   
   
};

// Required for Visual Studio dll exports
#ifdef EXPORT_TEMPLATES
   EXPIMP_TEMPLATE template class DECLSPECIFIER std::allocator<ElementWrapper*>;
   EXPIMP_TEMPLATE template class DECLSPECIFIER std::vector<ElementWrapper*>;
#endif

#endif // ElementWrapper_hpp
