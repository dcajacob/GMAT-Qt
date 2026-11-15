//$Id: MatlabWorkspace.cpp 9460 2011-04-21 22:03:28Z lindajun $
//------------------------------------------------------------------------------
//                                  MatlabWorkspace
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
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
// number S-67573-G
//
// Author: Linda Jun
// Created: 2004/09/08
//
/**
 * Implements MatlabWorkspace class which evaluates parameters to be sent to MATLAB
 * workspace.
 */
//------------------------------------------------------------------------------

#include "MatlabWorkspace.hpp"
#include "SubscriberBindingState.hpp"
#include "SubscriberException.hpp"
#include "Parameter.hpp"
#include "StringUtil.hpp"          // for ToString()
#include "MatlabInterface.hpp"
#include "MessageInterface.hpp"

//#define DEBUG_MATLABWS_PARAM
//#define DEBUG_MATLABWS_INIT
//#define DEBUG_MATLABWS_SET
//#define DBGLVL_MATLABWS_RUN 2

//---------------------------------
// static data
//---------------------------------
const std::string
MatlabWorkspace::PARAMETER_TEXT[MatlabWorkspaceParamCount - SubscriberParamCount] =
{
   "Add",
   "UpdateFrequency"
};

const Gmat::ParameterType
MatlabWorkspace::PARAMETER_TYPE[MatlabWorkspaceParamCount - SubscriberParamCount] =
{
   Gmat::OBJECTARRAY_TYPE,    //"Add"
   Gmat::INTEGER_TYPE,        //"UpdateFrequency"
};


//------------------------------------------------------------------------------
// MatlabWorkspace(const std::string &name, Parameter *firstParam)
//------------------------------------------------------------------------------
MatlabWorkspace::MatlabWorkspace(const std::string &name, Parameter *firstParam) :
   Subscriber("MatlabWorkspace", name)
{
   mNumParams = 0;
   mDataCount = 0;
   mSendCount = 0;
   
   if (firstParam != NULL)
      AddParameter(firstParam->GetName(), 0);
   
   mUpdateFrequency = 1;
   parameterCount = MatlabWorkspaceParamCount;
   
   matlabIf = NULL;
}


//------------------------------------------------------------------------------
// ~MatlabWorkspace(void)
//------------------------------------------------------------------------------
MatlabWorkspace::~MatlabWorkspace(void)
{
   //if (matlabIf != NULL)
   //   delete matlabIf;
}


//------------------------------------------------------------------------------
// MatlabWorkspace(const MatlabWorkspace &copy)
//------------------------------------------------------------------------------
MatlabWorkspace::MatlabWorkspace(const MatlabWorkspace &copy) :
   Subscriber(copy)
{
   mParams = copy.mParams; 
   mNumParams = copy.mNumParams;
   mParamNames = copy.mParamNames;
   mUpdateFrequency = copy.mUpdateFrequency;
   mAllRefObjectNames = copy.mAllRefObjectNames;
   mDataCount = 0;
   mSendCount = 0;
   
   matlabIf = nullptr;
   //matlabIf = new MatlabInterface("MatlabWorkspace");
   if (typeid(copy) == typeid(MatlabWorkspace)) ReconnectSubscriberCopy(copy);

}


//------------------------------------------------------------------------------
// MatlabWorkspace& MatlabWorkspace::operator=(const MatlabWorkspace& right)
//------------------------------------------------------------------------------
/**
 * The assignment operator
 */
//------------------------------------------------------------------------------
MatlabWorkspace& MatlabWorkspace::operator=(const MatlabWorkspace& right)
{
   if (this == &right) return *this;
   const auto previousContext = GetFunctionValueContext();
   auto transaction = typeid(*this) == typeid(MatlabWorkspace) ? BeginSubscriberValueCopy(right) : SubscriberValueCopyToken{};
   {
      FunctionValueCopyGuard nativeCopy;
      Subscriber::operator=(right);

      mParams = right.mParams;
      mNumParams = right.mNumParams;
      mParamNames = right.mParamNames;
      mUpdateFrequency = right.mUpdateFrequency;
      mAllRefObjectNames = right.mAllRefObjectNames;
      mDataCount = 0;
      mSendCount = 0;

      matlabIf = nullptr;

      //if (matlabIf != NULL)
      //{
      //   delete matlabIf;
      //   matlabIf = new MatlabInterface("MatlabWorkspace");
      //}
   }
   if (typeid(*this) == typeid(MatlabWorkspace)) CompleteSubscriberValueCopy(right, transaction);
   return *this;
}

//----------------------------------
// methods inherited from Subscriber
//----------------------------------

//------------------------------------------------------------------------------
// virtual bool Initialize()
//------------------------------------------------------------------------------
bool MatlabWorkspace::Initialize()
{
   #ifdef DEBUG_MATLABWS_INIT
   MessageInterface::ShowMessage
      ("MatlabWorkspace::Initialize() entered. mUpdateFrequency=%d, mDataCount=%d, "
       "mSendCount=%d\n", mUpdateFrequency, mDataCount, mSendCount);
   #endif
   
   Subscriber::Initialize();
   
   if (active)
   {
      if (mNumParams == 0)
      {
         MessageInterface::ShowMessage
            ("*** WARNING *** The MatlabWorkspace named \"%s\" will not be created.\n"
             "No parameters were added to MatlabWorkspace.\n", GetName().c_str());
         
         active = false;
         return false;
      }
   }
   
   //-----------------------------------------------------------------
   // Open Matlab engine, clear parameters in the workspace
   //-----------------------------------------------------------------
   MatlabInterface *matlabIf = MatlabInterface::Instance();
   //MatlabInterface *matlabIf = new MatlabInterface("MatlabWorkspace");
   if (!matlabIf->Open())
      throw SubscriberException("Cannot open the GMAT MATLAB engine session for " + instanceName + ".");
   
   for (int i=0; i < mNumParams; i++)
   {
      std::string matlabStr = "clear " + mParamNames[i];
      
      #ifdef DEBUG_MATLABWS_INIT
      MessageInterface::ShowMessage
         ("   Sending \"%s\" to MATLAB workspace\n", matlabStr.c_str());
      #endif
      
      matlabIf->RunMatlabString(matlabStr);
      
      // if it has dot, clear matlab struct
      std::string type, owner, dep;
      GmatStringUtil::ParseParameter(mParamNames[i], type, owner, dep);
      if (owner != "")
      {
         matlabStr = "clear " + owner;
         
         #ifdef DEBUG_MATLABWS_INIT
         MessageInterface::ShowMessage
            ("   Sending \"%s\" to MATLAB workspace\n", matlabStr.c_str());
         #endif
         
         matlabIf->RunMatlabString(matlabStr);
      }
   }
   //-----------------------------------------------------------------
   
   mDataCount = 0;
   mSendCount = 0;
   
   #ifdef DEBUG_MATLABWS_INIT
   MessageInterface::ShowMessage("MatlabWorkspace::Initialize() exiting\n");
   #endif
   
   return true;
}


//------------------------------------------------------------------------------
//  GmatBase* Clone(void) const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the MatlabWorkspace.
 *
 * @return clone of the MatlabWorkspace.
 *
 */
//------------------------------------------------------------------------------
GmatBase* MatlabWorkspace::Clone() const
{
   if (typeid(*this) != typeid(MatlabWorkspace))
      throw GmatBaseException("A derived MatlabWorkspace must provide its complete Clone.\n");
   std::unique_ptr<MatlabWorkspace> value;
   {
      FunctionValueCopyGuard nativeCopy;
      value.reset(new MatlabWorkspace(*this));
   }
   value->ReconnectSubscriberCopy(*this);
   return value.release();
}


//------------------------------------------------------------------------------
// virtual bool TakeAction(const std::string &action,
//                         const std::string &actionData = "");
//------------------------------------------------------------------------------
/**
 * This method performs action.
 *
 * @param <action> action to perform
 * @param <actionData> action data associated with action
 * @return true if action successfully performed
 *
 */
//------------------------------------------------------------------------------
bool MatlabWorkspace::TakeAction(const std::string &action,
                          const std::string &actionData)
{
   if (action == "Clear")
   {
      ClearParameters();
      return true;
   }

   return false;
}


//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
std::string MatlabWorkspace::GetParameterText(const Integer id) const
{
    if (id >= SubscriberParamCount && id < MatlabWorkspaceParamCount)
        return PARAMETER_TEXT[id - SubscriberParamCount];
    else
        return Subscriber::GetParameterText(id);
}


//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
Integer MatlabWorkspace::GetParameterID(const std::string &str) const
{
    for (Integer i = SubscriberParamCount; i < MatlabWorkspaceParamCount; i++)
    {
        if (str == PARAMETER_TEXT[i - SubscriberParamCount])
            return i;
    }

    return Subscriber::GetParameterID(str);
}


//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
Gmat::ParameterType MatlabWorkspace::GetParameterType(const Integer id) const
{
    if (id >= SubscriberParamCount && id < MatlabWorkspaceParamCount)
        return PARAMETER_TYPE[id - SubscriberParamCount];
    else
        return Subscriber::GetParameterType(id);

}


//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
std::string MatlabWorkspace::GetParameterTypeString(const Integer id) const
{
   if (id >= SubscriberParamCount && id < MatlabWorkspaceParamCount)
      return Subscriber::PARAM_TYPE_STRING[GetParameterType(id)];
   else
      return Subscriber::GetParameterTypeString(id);

}


//------------------------------------------------------------------------------
// virtual Integer GetIntegerParameter(const Integer id) const
//------------------------------------------------------------------------------
Integer MatlabWorkspace::GetIntegerParameter(const Integer id) const
{
   switch (id)
   {
   case UPDATE_FREQUENCY:
      return mUpdateFrequency;
   default:
      return Subscriber::GetIntegerParameter(id);
   }
}


//------------------------------------------------------------------------------
// virtual Integer GetIntegerParameter(const std::string &label) const
//------------------------------------------------------------------------------
Integer MatlabWorkspace::GetIntegerParameter(const std::string &label) const
{
   return GetIntegerParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
// virtual Integer SetIntegerParameter(const Integer id, const Integer value)
//------------------------------------------------------------------------------
Integer MatlabWorkspace::SetIntegerParameter(const Integer id, const Integer value)
{
   switch (id)
   {
   case UPDATE_FREQUENCY:
      mUpdateFrequency = value;
      if (mUpdateFrequency <= 0)
         mUpdateFrequency = 1;
      return mUpdateFrequency;
   default:
      return Subscriber::SetIntegerParameter(id, value);
   }
}


//------------------------------------------------------------------------------
// virtual Integer SetIntegerParameter(const std::string &label,
//                                     const Integer value)
//------------------------------------------------------------------------------
Integer MatlabWorkspace::SetIntegerParameter(const std::string &label,
                                        const Integer value)
{
   return SetIntegerParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// virtual bool SetStringParameter(const Integer id, const std::string &value)
//------------------------------------------------------------------------------
bool MatlabWorkspace::SetStringParameter(const Integer id, const std::string &value)
{
   switch (id)
   {
   case ADD:
      return AddParameter(value, mNumParams);
   default:
      return Subscriber::SetStringParameter(id, value);
   }
}


//------------------------------------------------------------------------------
// virtual bool SetStringParameter(const std::string &label,
//                                 const std::string &value)
//------------------------------------------------------------------------------
bool MatlabWorkspace::SetStringParameter(const std::string &label,
                                  const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// virtual bool SetStringParameter(const Integer id, const std::string &value,
//                                 const Integer index)
//------------------------------------------------------------------------------
bool MatlabWorkspace::SetStringParameter(const Integer id, const std::string &value,
                                  const Integer index)
{
   switch (id)
   {
   case ADD:
      return AddParameter(value, index);
   default:
      return Subscriber::SetStringParameter(id, value, index);
   }
}


//------------------------------------------------------------------------------
// virtual bool SetStringParameter(const std::string &label,
//                                 const std::string &value,
//                                 const Integer index)
//------------------------------------------------------------------------------
bool MatlabWorkspace::SetStringParameter(const std::string &label,
                                  const std::string &value,
                                  const Integer index)
{
   return SetStringParameter(GetParameterID(label), value, index);
}


//------------------------------------------------------------------------------
// const StringArray& GetStringArrayParameter(const Integer id) const
//------------------------------------------------------------------------------
const StringArray& MatlabWorkspace::GetStringArrayParameter(const Integer id) const
{
   switch (id)
   {
   case ADD:
      return mParamNames;
   default:
      return Subscriber::GetStringArrayParameter(id);
   }
}


//------------------------------------------------------------------------------
// StringArray& GetStringArrayParameter(const std::string &label) const
//------------------------------------------------------------------------------
const StringArray& MatlabWorkspace::GetStringArrayParameter(const std::string &label) const
{
   return GetStringArrayParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
// virtual GmatBase* GetRefObject(const UnsignedInt type,
//                                const std::string &name)
//------------------------------------------------------------------------------
GmatBase* MatlabWorkspace::GetRefObject(const UnsignedInt type,
                                 const std::string &name)
{
   for (int i=0; i<mNumParams; i++)
   {
      if (mParamNames[i] == name)
         return mParams[i];
   }

   throw GmatBaseException("MatlabWorkspace::GetRefObject() the object name: " + name +
                           " not found\n");
}


//------------------------------------------------------------------------------
// virtual bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//                           const std::string &name = "")
//------------------------------------------------------------------------------
bool MatlabWorkspace::SetRefObject(GmatBase *obj, const UnsignedInt type,
                          const std::string &name)
{
   #ifdef DEBUG_MATLABWS_SET
   MessageInterface::ShowMessage
      ("MatlabWorkspace::SetRefObject() obj=%p, name=%s, objtype=%s, objname=%s\n",
       obj, name.c_str(), obj->GetTypeName().c_str(), obj->GetName().c_str());
   #endif
   
   for (int i=0; i<mNumParams; i++)
   {
      if (mParamNames[i] == name)
      {
         mParams[i] = (Parameter*)obj;
         return true;
      }
   }
   
   #ifdef DEBUG_MATLABWS_SET
   MessageInterface::ShowMessage
      ("MatlabWorkspace::SetRefObject() %s not found, so returning false\n", name.c_str());
   #endif
   
   return false;
}


//------------------------------------------------------------------------------
// virtual const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
const StringArray& MatlabWorkspace::GetRefObjectNameArray(const UnsignedInt type)
{
   mAllRefObjectNames.clear();
   
   switch (type)
   {
   case Gmat::PARAMETER:
   case Gmat::UNKNOWN_OBJECT:
      mAllRefObjectNames = mParamNames;
   default:
      break;
   }
   
   return mAllRefObjectNames;
}


//------------------------------------------------------------------------------
// Integer GetNumParameters()
//------------------------------------------------------------------------------
Integer MatlabWorkspace::GetNumParameters()
{
   return mNumParams;
}


//------------------------------------------------------------------------------
// bool AddParameter(const std::string &paramName, Integer index)
//------------------------------------------------------------------------------
bool MatlabWorkspace::AddParameter(const std::string &paramName, Integer index)
{
   #ifdef DEBUG_MATLABWS_PARAM
   MessageInterface::ShowMessage
      ("MatlabWorkspace::AddParameter() name = %s, index = %d\n", paramName.c_str(), index);
   #endif
   
   if (paramName != "" && index == mNumParams)
   {
      StringArray::iterator paramPos = 
         find(mParamNames.begin(), mParamNames.end(), paramName);
      
      // if name is unique, add
      if (paramPos == mParamNames.end())
      {
         mParamNames.push_back(paramName);
         mNumParams = mParamNames.size();
         mParams.push_back(NULL);
         return true;
      }
   }
   
   return false;
}

//--------------------------------------
// protected methods
//--------------------------------------

//--------------------------------------
// methods inherited from Subscriber
//--------------------------------------

//------------------------------------------------------------------------------
// bool Distribute(int len)
//------------------------------------------------------------------------------
bool MatlabWorkspace::Distribute(int len)
{      
   return false;
}


//------------------------------------------------------------------------------
// bool Distribute(const Real * dat, Integer len)
//------------------------------------------------------------------------------
bool MatlabWorkspace::Distribute(const Real * dat, Integer len)
{
   #if DBGLVL_MATLABWS_RUN
   MessageInterface::ShowMessage
      ("MatlabWorkspace::Distribute() entered. mUpdateFrequency=%d, mDataCount=%d, "
       "mSendCount=%d\n", mUpdateFrequency, mDataCount, mSendCount);
   #endif
   
   Rvector varvals = Rvector(mNumParams);
   
   if (len == 0)
   {
      return false;
   }
   else
   {
      mDataCount++;
      
      if ((mDataCount % mUpdateFrequency) == 0)
      {
         mDataCount = 0;
         mSendCount++;
         
         for (int i=0; i < mNumParams; i++)
         {
            if (mParams[i] == NULL)
               throw SubscriberException("MatlabWorkspace::Distribute: Cannot find "
                                         "parameter \"" + mParamNames[i] + "\"\n");
            
            mParams[i]->Evaluate();
            std::string paramVal = mParams[i]->ToString();
            
            #if DBGLVL_MATLABWS_RUN > 1
            MessageInterface::ShowMessage
               ("   %s = %s\n",  mParamNames[i].c_str(), paramVal.c_str());
            #endif
            
            //--------------------------------------------------------
            // if using MATLAB, send it to workspace
            //--------------------------------------------------------
            MatlabInterface *matlabIf = MatlabInterface::Instance();
            if (matlabIf == NULL || !matlabIf->Open())
               throw SubscriberException("Cannot select the GMAT MATLAB engine session for " + instanceName + ".");
            
            std::string countStr = GmatStringUtil::ToString(mSendCount, 1);
            
            if (mParams[i]->GetTypeName() == "Array")
            {
               countStr = "(" + countStr + ",:) = ";
               paramVal = "[ " + paramVal + "]";
            }
            else
            {
               countStr = "(" + countStr + ") = ";
            }
            
            std::string matlabStr = mParamNames[i] + countStr + paramVal;
            
            #if DBGLVL_MATLABWS_RUN > 1
            MessageInterface::ShowMessage
               ("   Sending \"%s\" to MATLAB workspace\n", matlabStr.c_str());
            #endif
            
            matlabIf->RunMatlabString(matlabStr);
            //--------------------------------------------------------
         }
      }
   }
   
   return true;
}

//--------------------------------------
// private methods
//--------------------------------------

//------------------------------------------------------------------------------
// void ClearParameters()
//------------------------------------------------------------------------------
void MatlabWorkspace::ClearParameters()
{
   #ifdef DEBUG_MATLABWS_PARAM
   MessageInterface::ShowMessage("\nMatlabWorkspace::ClearParameters() entered\n");
   #endif
   
   mParams.clear();
   mParamNames.clear();
   mNumParams = 0;
}



std::unique_ptr<FunctionInvocationState> MatlabWorkspace::SuspendFunctionBindings()
{
   auto native = std::make_shared<SubscriberBindingState>();
   native->Capture(mParams);
   native->Capture(matlabIf);
   native->Require([this, role0 = mParamNames]
   {
      return role0 == mParamNames;
   });
   return WrapFunctionBindings(std::move(native), Subscriber::SuspendFunctionBindings());
}


void MatlabWorkspace::Copy(const GmatBase *original)
{
   const auto *source = dynamic_cast<const MatlabWorkspace*>(original);
   if (!source || typeid(*source) != typeid(*this) || !HasSubscriberValueReferenceProvider())
      throw GmatBaseException("Cannot copy an incompatible MatlabWorkspace value.\n");
   if (source == this) return;
   const auto previousContext = GetFunctionValueContext();
   auto transaction = BeginSubscriberValueCopy(*source);
   {
      FunctionValueCopyGuard nativeCopy;
      operator=(*source);
   }
   CompleteSubscriberValueCopy(*source, transaction);
}


bool MatlabWorkspace::HasSubscriberValueReferenceProvider() const
{
   return typeid(*this) == typeid(MatlabWorkspace);
}


void MatlabWorkspace::RebindSubscriberValueReferences(const Subscriber &original,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   const auto &source = dynamic_cast<const MatlabWorkspace&>(original);
   std::vector<Parameter*> parameters;
   parameters.reserve(source.mParams.size());
   for (auto *parameter : source.mParams) parameters.push_back(MapSubscriberReference(parameter, resolve));
   Subscriber::RebindSubscriberValueReferences(source, resolve);
   mParams.swap(parameters);
   matlabIf = nullptr; // Ordinary native use obtains the current Interface.
}
