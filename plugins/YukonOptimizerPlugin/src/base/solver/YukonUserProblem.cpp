//$Id$
//------------------------------------------------------------------------------
//                                YukonUserProblem
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
// contract, Task Order 08.
//
// Created: 2017, Steven Hughes
//
// Converted: 2017/09/15, Joshua Raymond, Thinking Systems, Inc.
//
/**
 * Defines YukonUserProblem base class for YukonUserProblems
 */
//------------------------------------------------------------------------------

#include "YukonUserProblem.hpp"
#include "OptimizerValueCopy.hpp"
#include <utility>

//------------------------------------------------------------------------------
// YukonUserProblem()
//------------------------------------------------------------------------------
/**
* Constructor
*/
//------------------------------------------------------------------------------
YukonUserProblem::YukonUserProblem()
{
   inf = std::numeric_limits<double>::infinity();
}

//------------------------------------------------------------------------------
// void HandleOneSidedVarBounds(Rvector lowerBoundVector, 
//                              Rvector upperBoundVector)
//------------------------------------------------------------------------------
/**
* Method to set up parameters for variables with only on side bounded
*
* @param lowerBoundVector The variable lower bounds
* @param upperBoundVector The variable upper bounds
*/
//------------------------------------------------------------------------------
YukonUserProblem::YukonUserProblem(const YukonUserProblem &source) :
   AVarBound(OptimizerValueCopy::Matrix(source.AVarBound)),
   bVarBound(OptimizerValueCopy::Vector(source.bVarBound)),
   numBoundCon(source.numBoundCon),
   conType(OptimizerValueCopy::Vector(source.conType)),
   conMode(source.conMode),
   bVarBoundUpper(OptimizerValueCopy::Vector(source.bVarBoundUpper)),
   bVarBoundLower(OptimizerValueCopy::Vector(source.bVarBoundLower)),
   problemName(source.problemName),
   costFuncName(source.costFuncName),
   conFuncName(source.conFuncName),
   varLowerBounds(OptimizerValueCopy::Vector(source.varLowerBounds)),
   varUpperBounds(OptimizerValueCopy::Vector(source.varUpperBounds)),
   conLowerBounds(OptimizerValueCopy::Vector(source.conLowerBounds)),
   conUpperBounds(OptimizerValueCopy::Vector(source.conUpperBounds)),
   numNonLinIneqCon(source.numNonLinIneqCon),
   nonLinIneqConStartIdx(source.nonLinIneqConStartIdx),
   numNonLinEqCon(source.numNonLinEqCon),
   nonLinEqConStartIdx(source.nonLinEqConStartIdx),
   numLinIneqCon(source.numLinIneqCon),
   linIneqConStartIdx(source.linIneqConStartIdx),
   numLinEqCon(source.numLinEqCon),
   linEqConStartIdx(source.linEqConStartIdx),
   boundConStartIdx(source.boundConStartIdx),
   conJacobian(OptimizerValueCopy::Matrix(source.conJacobian)),
   costJacobian(OptimizerValueCopy::Vector(source.costJacobian)),
   numNonLinCon(source.numNonLinCon),
   numLinCon(source.numLinCon),
   numVars(source.numVars),
   numCons(source.numCons),
   conFunctions(OptimizerValueCopy::Vector(source.conFunctions)),
   inf(source.inf)
{
}

YukonUserProblem& YukonUserProblem::operator=(const YukonUserProblem &source)
{
   if (&source == this) return *this;
   // Complete every allocation before replacing any current problem data.
   Rmatrix next_AVarBound(OptimizerValueCopy::Matrix(source.AVarBound));
   Rvector next_bVarBound(OptimizerValueCopy::Vector(source.bVarBound));
   Rvector next_conType(OptimizerValueCopy::Vector(source.conType));
   Rvector next_bVarBoundUpper(OptimizerValueCopy::Vector(source.bVarBoundUpper));
   Rvector next_bVarBoundLower(OptimizerValueCopy::Vector(source.bVarBoundLower));
   std::string next_problemName(source.problemName);
   std::string next_costFuncName(source.costFuncName);
   std::string next_conFuncName(source.conFuncName);
   Rvector next_varLowerBounds(OptimizerValueCopy::Vector(source.varLowerBounds));
   Rvector next_varUpperBounds(OptimizerValueCopy::Vector(source.varUpperBounds));
   Rvector next_conLowerBounds(OptimizerValueCopy::Vector(source.conLowerBounds));
   Rvector next_conUpperBounds(OptimizerValueCopy::Vector(source.conUpperBounds));
   Rmatrix next_conJacobian(OptimizerValueCopy::Matrix(source.conJacobian));
   Rvector next_costJacobian(OptimizerValueCopy::Vector(source.costJacobian));
   Rvector next_conFunctions(OptimizerValueCopy::Vector(source.conFunctions));
   AVarBound.Swap(next_AVarBound);
   bVarBound.Swap(next_bVarBound);
   numBoundCon = source.numBoundCon;
   conType.Swap(next_conType);
   conMode = source.conMode;
   bVarBoundUpper.Swap(next_bVarBoundUpper);
   bVarBoundLower.Swap(next_bVarBoundLower);
   problemName.swap(next_problemName);
   costFuncName.swap(next_costFuncName);
   conFuncName.swap(next_conFuncName);
   varLowerBounds.Swap(next_varLowerBounds);
   varUpperBounds.Swap(next_varUpperBounds);
   conLowerBounds.Swap(next_conLowerBounds);
   conUpperBounds.Swap(next_conUpperBounds);
   numNonLinIneqCon = source.numNonLinIneqCon;
   nonLinIneqConStartIdx = source.nonLinIneqConStartIdx;
   numNonLinEqCon = source.numNonLinEqCon;
   nonLinEqConStartIdx = source.nonLinEqConStartIdx;
   numLinIneqCon = source.numLinIneqCon;
   linIneqConStartIdx = source.linIneqConStartIdx;
   numLinEqCon = source.numLinEqCon;
   linEqConStartIdx = source.linEqConStartIdx;
   boundConStartIdx = source.boundConStartIdx;
   conJacobian.Swap(next_conJacobian);
   costJacobian.Swap(next_costJacobian);
   numNonLinCon = source.numNonLinCon;
   numLinCon = source.numLinCon;
   numVars = source.numVars;
   numCons = source.numCons;
   conFunctions.Swap(next_conFunctions);
   inf = source.inf;
   return *this;
}

void YukonUserProblem::HandleOneSidedVarBounds(Rvector lowerBoundVector,
                                               Rvector upperBoundVector)
{
   if (conMode == 1)
   {
      numBoundCon = 0;
      for (Integer i = 0; i < lowerBoundVector.GetSize(); ++i)
      {
         if (lowerBoundVector[i] > -inf || upperBoundVector[i] < inf)
            ++numBoundCon;
      }
      
      AVarBound.SetSize(numBoundCon, numVars);
      bVarBound.SetSize(numBoundCon);
      for (Integer i = 0; i < numBoundCon; ++i)
         bVarBound[i] = 0;
      Integer cb = -1;
      if (lowerBoundVector.GetSize() != 0)
      {
         for (Integer i = 0; i < numVars; ++i)
         {
            // Construct the lower bound constraint matrix
            if (lowerBoundVector[i] > -inf)
            {
               ++cb;
               AVarBound(cb, i) = 1.0;
               bVarBound[cb] = lowerBoundVector[i];
            }
         }
      }

      if (upperBoundVector.GetSize() != 0)
      {
         for (Integer i = 0; i < numVars; ++i)
         {
            // Construct the upper bound constraint matrix
            if (upperBoundVector[i] < inf)
            {
               ++cb;
               AVarBound(cb, i) = -1;
               bVarBound[cb] = -upperBoundVector[i];
            }
         }
      }
   }

   else
   {
      if (conMode == 2)
      {
         numBoundCon = numVars;
         AVarBound.SetSize(numVars, numVars);
         for (Integer i = 0; i < numVars; ++i)
            AVarBound(i, i) = 1;
         if (lowerBoundVector.GetSize() == 0)
         {
            lowerBoundVector.SetSize(numVars);
            for (Integer i = 0; i < numVars; ++i)
               lowerBoundVector[i] = -inf;
         }
         if (upperBoundVector.GetSize() == 0)
         {
            upperBoundVector.SetSize(numVars);
            for (Integer i = 0; i < numVars; ++i)
               upperBoundVector[i] = inf;
         }

         bVarBoundLower.SetSize(lowerBoundVector.GetSize());
         bVarBoundLower = lowerBoundVector;
         bVarBoundUpper.SetSize(upperBoundVector.GetSize());
         bVarBoundUpper = upperBoundVector;

         // Remove spurions bounds (-inf to inf) to avoid numerical issues
         Rvector deleteRows(0);
         for (Integer conIdx = 0; conIdx < numVars; ++conIdx)
         {
            if (bVarBoundLower[conIdx] == -inf &&
               bVarBoundUpper[conIdx] == inf)
            {
               deleteRows.Resize(deleteRows.GetSize() + 1);
               deleteRows[deleteRows.GetSize() - 1] = conIdx;
            }
         }

         if (deleteRows.GetSize() != 0)
         {
            Rmatrix ACopy = AVarBound;
            Rvector bLowerCopy = bVarBoundLower;
            Rvector bUpperCopy = bVarBoundUpper;

            AVarBound.SetSize(AVarBound.GetNumRows() - deleteRows.GetSize(),
               AVarBound.GetNumColumns());
            bVarBoundLower.SetSize(bVarBoundLower.GetSize() -
               deleteRows.GetSize());
            bVarBoundUpper.SetSize(bVarBoundUpper.GetSize() -
               deleteRows.GetSize());
            Integer rowSkipCount = 0;
            bool rowWasDeleted = false;

            for (Integer i = 0; i < ACopy.GetNumRows(); ++i)
            {
               for (Integer deleteIdx = 0; deleteIdx < deleteRows.GetSize();
                  ++deleteIdx)
               {
                  if (i == deleteRows[deleteIdx])
                  {
                     ++rowSkipCount;
                     rowWasDeleted = true;
                     break;
                  }
               }
               if (!rowWasDeleted)
               {
                  bVarBoundLower[i - rowSkipCount] = bLowerCopy[i];
                  bVarBoundUpper[i - rowSkipCount] = bUpperCopy[i];
                  for (Integer j = 0; j < AVarBound.GetNumColumns(); ++j)
                     AVarBound(i - rowSkipCount, j) = ACopy(i, j);
               }
               rowWasDeleted = false;
            }
         }
         numBoundCon = AVarBound.GetNumRows();
      }

      else
      {
         if (conMode == 3)
         {
            AVarBound.SetSize(0, 0);
            bVarBoundLower.SetSize(0);
            bVarBoundUpper.SetSize(0);
            numBoundCon = 0;
         }
      }
   }
}