//$Id$
//------------------------------------------------------------------------------
//                                NLPFunctionGenerator
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
// Converted: 2017/10/12, Joshua Raymond, Thinking Systems, Inc.
//
/**
 * Declares NLPFunctionGenerator methods
 */
//------------------------------------------------------------------------------

#ifndef NLPFunctionGenerator_hpp
#define NLPFunctionGenerator_hpp

#include "gmatdefs.hpp"
#include "Rvector.hpp"
#include "Rmatrix.hpp"
#include "YukonUserProblem.hpp"

class YUKON_API NLPFunctionGenerator
{
public:
   NLPFunctionGenerator(YukonUserProblem *inputUserFunction);
   NLPFunctionGenerator(YukonUserProblem *inputUserFunction,
                        bool inputIsModeElastic, Real inputElasticWeight);
   ~NLPFunctionGenerator();
   NLPFunctionGenerator(const NLPFunctionGenerator &source);
   NLPFunctionGenerator& operator=(const NLPFunctionGenerator &source);
   YukonUserProblem* GetUserProblem() const;
   void RebindUserProblem(YukonUserProblem *original, YukonUserProblem *replacement);
   void ValidateUserProblem();
   void PrepareElasticMode(Real inputElasticWeight);
   void PrepareArrays();

   // User Function Interfaces
   void EvaluateCostAndJac(Integer numVars, Rvector decVector, bool isNewX,
                           Real &fun, Rvector &funJac, Integer &numFunEvals);
   void EvaluateConAndJac(Integer numVars, Rvector decVector, bool isNewX,
                          Rvector &conFunc, Rmatrix &conJac,
                          Integer &numFunEvals);
   void EvaluateAllFunJac(Integer numVars, Rvector decVector, bool isNewX,
                          Real &fun, Rvector &funJac, Integer &numFunEvals,
                          Rvector &conFunc, Rmatrix &conJac,
                          Integer &numGEvals);
   void EvaluateFuncsOnly(Integer numVars, Rvector decVector, bool isNewX,
                          Real &fun, Integer &numFunEvals, Rvector &conFunc,
                          Integer &numGEvals);
   void EvaluateDerivsOnly(Integer numVars, Rvector decVector, bool isNewX,
                           Rvector &costJac, Rmatrix &conJac,
                           Integer &numGEvals);


   // NLP Function Interfaces
   void GetNLPInfo(Integer &numVars, Integer &numCons);
   void GetNLPBoundsInfo(Rvector &varLowerBounds, Rvector &varUpperBounds,
                         Rvector &conLowerBounds, Rvector &conUpperBounds);
   Rvector GetNLPStartingPoint();
   void EvaluateNLPFunctionsOnly(Integer numVars, Rvector decVector,
                                 bool isNewX, Real &fun, Integer &numFunEvals,
                                 Rvector &conFunc, Integer &numGEvals);
   void EvaluateNLPDerivsOnly(Integer numVars, Rvector decVector, bool isNewX,
                              Rvector &nlpCostJac, Rmatrix &nlpConJac,
                              Integer &numGEvals);
   void EvaluateAllNLPFuncJac(Integer numVars, Rvector decVector, bool isNewX,
                              Real &nlpCost, Rvector &nlpCostJac,
                              Integer &numCostEvals, Rvector &nlpConFunc,
                              Rmatrix &nlpConJac, Integer &numGEvals);
   void InsertUserConJac(Rmatrix userConJac);
   void InsertUserCostJac(Rvector userCostJac);
   void UpdateElasticCostJacobian();
   Integer GetNumUserDecisionVars();
   Integer GetNumUserConstraints();
   Integer GetNumElasticVars();
   Rvector GetElasticV(Rvector decVector);
   Rvector GetElasticW(Rvector decVector);
   Real GetMaxElasticVar(Rvector decVector);
   void SetElasticWeight(Real elasticWeightInput);
   Real MaxUserConViolation(Rvector conViolation, Rvector decVector);
   Real ShiftCost(Real cost, Rvector decVector);
   Rvector EvaluateMaxVarStep();

private:
   void SwapValueState(NLPFunctionGenerator &other);
   /// Pointer to object that thes user provides
   YukonUserProblem *userFunction = NULL;
   /// Number of decision variables in the user's problem
   Integer numUserDecisionVars = 0;
   /// Number of (non-bound) constraints in the user's problem
   Integer numUserConstraints = 0;
   /// Number of decision variables with non -inf to inf bounds
   Integer numBoundCons = 0;
   /// Total number of decision variables (user + elastic)
   Integer totalNumDecisionVars = 0;
   /// Total number of constraints (user con + bound con + elastic var con)
   Integer totalNumConstraints = 0;
   /// Number of elastic mode decision variables
   Integer numElasticVars = 0;
   /// Number of elastic mode constraints
   Integer numElasticConstraints = 0;
   /// Flag indicating if problem is in elastic mode
   bool isModeElastic = false;
   /// Vector of all constraints (user cons + bound cons)
   Rvector conValues;
   /// The Jacobian of the cost function
   Rvector costJac;
   /// The Jacobian of the constraint functions
   Rmatrix conJac;
   /// The lower bounds on NLP constraints
   Rvector nlpConLowerBounds;
   /// The upper bounds opn NLP constraints
   Rvector nlpConUpperBounds;
   /// The lower bounds on NLP decision variables
   Rvector nlpVarLowerBounds;
   /// The upper bounds on NLP decision variables
   Rvector nlpVarUpperBounds;
   ///  A matrix for computing bound constraints
   Rmatrix boundAMatrix;
   /// Index into conValues array indicating where user cons start
   Integer userConJacRowStartIdx = 0;
   /// Index into conValues array indicating where user cons end
   Integer userConJacRowStopIdx = 0;
   /// Index into conValues array indicating where user vars start
   Integer userConJacColStartIdx = 0;
   /// Index into conValues array indicating where user vars end
   Integer userConJacColStopIdx = 0;
   /// Elastic weight parameter
   Real elasticWeight = 0.0;
   /// Index into decision vector indicating where elastic V vars start
   Integer elasticVarVStartIdx = 0;
   /// Index into decision vector indicating where elastic V vars end
   Integer elasticVarVStopIdx = 0;
   /// Index into decision vector indicating where elastic W vars start
   Integer elasticVarWStartIdx = 0;
   /// Index into decision vector indicating where elastic W vars end
   Integer elasticVarWStopIdx = 0;
   /// Index into conValues indicating constraints associated with user
   /// decision variable bounds
   Rvector userVarBoundConIdxs;
   /// Parameter representing infinity
   Real inf = 0.0;

};
#endif
