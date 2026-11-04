//$Id$
//------------------------------------------------------------------------------
//                                   MathNode
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
// Created: 2006/03/27
//
/**
 * Defines the MathNode base class used for Math in Scripts.
 */
//------------------------------------------------------------------------------

#include "MathNode.hpp"
#include "GmatBaseException.hpp"
#include <typeinfo>
#include "Abs.hpp"
#include "Acos.hpp"
#include "Add.hpp"
#include "Asin.hpp"
#include "Atan.hpp"
#include "Atan2.hpp"
#include "Ceil.hpp"
#include "Cos.hpp"
#include "Cross3.hpp"
#include "DegToRad.hpp"
#include "Determinant.hpp"
#include "Diag.hpp"
#include "Divide.hpp"
#include "Exp.hpp"
#include "Fix.hpp"
#include "Floor.hpp"
#include "FunctionRunner.hpp"
#include "Inverse.hpp"
#include "Log.hpp"
#include "Log10.hpp"
#include "MathElement.hpp"
#include "Min.hpp"
#include "Mod.hpp"
#include "Multiply.hpp"
#include "Negate.hpp"
#include "Norm.hpp"
#include "Power.hpp"
#include "RadToDeg.hpp"
#include "Rand.hpp"
#include "Randn.hpp"
#include "Sin.hpp"
#include "Sprintf.hpp"
#include "Sqrt.hpp"
#include "Strcat.hpp"
#include "Strcmp.hpp"
#include "Strfind.hpp"
#include "Strlen.hpp"
#include "Strrep.hpp"
#include "Substr.hpp"
#include "Subtract.hpp"
#include "Tan.hpp"
#include "Transpose.hpp"

#include "MessageInterface.hpp"

//------------------------------------------------------------------------------
//  MathNode(std::string typeStr, std::string nomme)
//------------------------------------------------------------------------------
/**
 * Constructs the MathNode object (default constructor).
 * 
 * @param <typeStr> String text identifying the object type
 * @param <nomme>   Name for the object
 */
//------------------------------------------------------------------------------
MathNode::MathNode(const std::string &typeStr, const std::string &nomme) :
   GmatBase        (Gmat::MATH_NODE, typeStr, nomme),
   isNumber        (false),
   isFunction      (false),
   isFunctionInput (false),
   elementType     (Gmat::REAL_TYPE),
   realValue       (REAL_PARAMETER_UNDEFINED),
   stringValue     (STRING_PARAMETER_UNDEFINED)
{
   objectTypes.push_back(Gmat::MATH_NODE);
   objectTypeNames.push_back("MathNode");   
}


//------------------------------------------------------------------------------
//  ~MathNode(void)
//------------------------------------------------------------------------------
/**
 * Destroys the MathNode object (destructor).
 */
//------------------------------------------------------------------------------
MathNode::~MathNode()
{
   //MessageInterface::ShowMessage
   //   ("==> MathNode::~MathNode() deleting %s, %s\n", GetTypeName().c_str(),
   //    GetName().c_str());
}


//------------------------------------------------------------------------------
//  MathNode(const MathNode &mn)
//------------------------------------------------------------------------------
/**
 * Constructs the MathNode object (copy constructor).
 * 
 * @param <mn> Object that is copied
 */
//------------------------------------------------------------------------------
MathNode::MathNode(const MathNode &mn) :
   GmatBase        (mn),
   isNumber        (mn.isNumber),
   isFunction      (mn.isFunction),
   isFunctionInput (mn.isFunctionInput),
   elementType     (mn.elementType),
   realValue       (mn.realValue),
   stringValue     (mn.stringValue)
{
   // Scalar/default nodes do not have a sized matrix cache.
   if (mn.matrix.IsSized())
      matrix = mn.matrix;
}


//------------------------------------------------------------------------------
//  MathNode& operator=(const MathNode &mn)
//------------------------------------------------------------------------------
/**
 * Sets one MathNode object to match another (assignment operator).
 * 
 * @param <mn> The object that is copied.
 * 
 * @return this object, with the parameters set as needed.
 */
//------------------------------------------------------------------------------
MathNode& MathNode::operator=(const MathNode &mn)
{
   if (this == &mn)
      return *this;
   
   GmatBase::operator=(mn);
   
   isNumber = mn.isNumber;
   isFunction = mn.isFunction;
   isFunctionInput = mn.isFunctionInput;
   elementType = mn.elementType;
   realValue = mn.realValue;
   stringValue = mn.stringValue;
   if (mn.matrix.IsSized())
   {
      if (!matrix.IsSized() || matrix.GetNumRows() != mn.matrix.GetNumRows() ||
          matrix.GetNumColumns() != mn.matrix.GetNumColumns())
         matrix.SetSize(mn.matrix.GetNumRows(), mn.matrix.GetNumColumns());
      matrix = mn.matrix;
   }
   
   return *this;
}


//------------------------------------------------------------------------------
// void SetRealValue(Real val)
//------------------------------------------------------------------------------
void MathNode::SetRealValue(Real val)
{
   elementType = Gmat::REAL_TYPE;
   realValue = val;
}

//------------------------------------------------------------------------------
// void SetMatrixValue(const Rmatrix &mat)
//------------------------------------------------------------------------------
void MathNode::SetMatrixValue(const Rmatrix &mat)
{
   elementType = Gmat::RMATRIX_TYPE;
   Integer theRowCount = mat.GetNumRows();
   Integer theColCount = mat.GetNumColumns();
   
   if (!matrix.IsSized() || matrix.GetNumRows() != theRowCount ||
       matrix.GetNumColumns() != theColCount)
      matrix.SetSize(theRowCount, theColCount);
   
   matrix = mat;
   
   #if DEBUG_MATRIX_NODE
   std::stringstream ss("");
   ss << matarix;
   MessageInterface::ShowMessage
      ("MathNode::SetMatrixValue() theReturnType=%d, theRowCount=%d, "
       "theColCount=%d, matarix=\n%s\n", theReturnType, theRowCount, theColCount,
       ss.str().c_str());
   #endif
}


//---------------------------------------------------------------------------
// virtual void SetMathWrappers(WrapperMap *wrapperMap)
//---------------------------------------------------------------------------
void MathNode::SetMathWrappers(WrapperMap *wrapperMap)
{
   // do nothing here
}

//---------------------------------------------------------------------------
// GmatBase* EvaluateObject()
//---------------------------------------------------------------------------
GmatBase* MathNode::EvaluateObject()
{
   return NULL;
}

//---------------------------------------------------------------------------
// std::string EvaluateString()
//---------------------------------------------------------------------------
std::string MathNode::EvaluateString()
{
   return STRING_PARAMETER_UNDEFINED;
}

//---------------------------------------------------------------------------
//  bool RenameRefObject(const UnsignedInt type,
//                       const std::string &oldName, const std::string &newName)
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
bool MathNode::RenameRefObject(const UnsignedInt type,
                               const std::string &oldName,
                               const std::string &newName)
{
   // There are no renamable objects
   return true;
}


namespace
{
   template <typename Node>
   bool CopyExactMathNode(MathNode *destination, const GmatBase *original)
   {
      if (typeid(*destination) != typeid(Node))
         return false;
      const auto *source = dynamic_cast<const Node*>(original);
      if (!source || typeid(*source) != typeid(Node))
         throw GmatBaseException("Cannot copy incompatible math node into " +
                                 destination->GetTypeName());
      if (source != destination)
         *static_cast<Node*>(destination) = *source;
      return true;
   }
}

void MathNode::Copy(const GmatBase *original)
{
   // Mirror the concrete MathFactory classes. Unknown derived types retain
   // the requirement to implement their own Copy contract instead of slicing.
   if (CopyExactMathNode<Abs>(this, original)) return;
   if (CopyExactMathNode<Acos>(this, original)) return;
   if (CopyExactMathNode<Add>(this, original)) return;
   if (CopyExactMathNode<Asin>(this, original)) return;
   if (CopyExactMathNode<Atan>(this, original)) return;
   if (CopyExactMathNode<Atan2>(this, original)) return;
   if (CopyExactMathNode<Ceil>(this, original)) return;
   if (CopyExactMathNode<Cos>(this, original)) return;
   if (CopyExactMathNode<Cross3>(this, original)) return;
   if (CopyExactMathNode<DegToRad>(this, original)) return;
   if (CopyExactMathNode<Determinant>(this, original)) return;
   if (CopyExactMathNode<Diag>(this, original)) return;
   if (CopyExactMathNode<Divide>(this, original)) return;
   if (CopyExactMathNode<Exp>(this, original)) return;
   if (CopyExactMathNode<Fix>(this, original)) return;
   if (CopyExactMathNode<Floor>(this, original)) return;
   if (CopyExactMathNode<FunctionRunner>(this, original)) return;
   if (CopyExactMathNode<Inverse>(this, original)) return;
   if (CopyExactMathNode<Log>(this, original)) return;
   if (CopyExactMathNode<Log10>(this, original)) return;
   if (CopyExactMathNode<MathElement>(this, original)) return;
   if (CopyExactMathNode<Min>(this, original)) return;
   if (CopyExactMathNode<Mod>(this, original)) return;
   if (CopyExactMathNode<Multiply>(this, original)) return;
   if (CopyExactMathNode<Negate>(this, original)) return;
   if (CopyExactMathNode<Norm>(this, original)) return;
   if (CopyExactMathNode<Power>(this, original)) return;
   if (CopyExactMathNode<RadToDeg>(this, original)) return;
   if (CopyExactMathNode<Rand>(this, original)) return;
   if (CopyExactMathNode<Randn>(this, original)) return;
   if (CopyExactMathNode<Sin>(this, original)) return;
   if (CopyExactMathNode<Sprintf>(this, original)) return;
   if (CopyExactMathNode<Sqrt>(this, original)) return;
   if (CopyExactMathNode<Strcat>(this, original)) return;
   if (CopyExactMathNode<Strcmp>(this, original)) return;
   if (CopyExactMathNode<Strfind>(this, original)) return;
   if (CopyExactMathNode<Strlen>(this, original)) return;
   if (CopyExactMathNode<Strrep>(this, original)) return;
   if (CopyExactMathNode<Substr>(this, original)) return;
   if (CopyExactMathNode<Subtract>(this, original)) return;
   if (CopyExactMathNode<Tan>(this, original)) return;
   if (CopyExactMathNode<Transpose>(this, original)) return;
   throw GmatBaseException("Cannot copy unsupported derived math node " +
                           GetTypeName());
}
