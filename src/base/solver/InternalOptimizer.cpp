//$Id$
//------------------------------------------------------------------------------
//                             InternalOptimizer
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
// number NNG06CA54C
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: 2007/03/27
//
/**
 * Implementation of the internal optimizer. 
 */
//------------------------------------------------------------------------------


#include "InternalOptimizer.hpp"
#include "MessageInterface.hpp"
#include "SolverException.hpp"
#include <sstream>
#include <iomanip>

//-----------------------------------------
// public methods
//-----------------------------------------


//------------------------------------------------------------------------------
// InternalOptimizer()
//------------------------------------------------------------------------------
/**
 * Default constructor
 */
//------------------------------------------------------------------------------
InternalOptimizer::InternalOptimizer(std::string type, std::string name) :
   Optimizer      (type, name)
{
   objectTypeNames.push_back("InternalOptimizer");
}

//------------------------------------------------------------------------------
// InternalOptimizer()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
InternalOptimizer::~InternalOptimizer()
{
}

//------------------------------------------------------------------------------
// InternalOptimizer(const InternalOptimizer &opt)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 */
//------------------------------------------------------------------------------
InternalOptimizer::InternalOptimizer(const InternalOptimizer &opt) :
   Optimizer      (opt)
{
}

//------------------------------------------------------------------------------
// InternalOptimizer& operator=(const InternalOptimizer &jac)
//------------------------------------------------------------------------------
/**
 * InternalOptimizer assignment operator
 * 
 * Copies the data from another InternalOptimizer instance into this one.
 * 
 * @param jac The supplier of the InternalOptimizer data.
 * 
 * @return A reference to this instance
 */
//------------------------------------------------------------------------------
InternalOptimizer& InternalOptimizer::operator=(const InternalOptimizer& opt)
{
   if (&opt != this)
   {
      Optimizer::operator=(opt);
   }
   
   return *this;
}

//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Method used to set up the InternalOptimizer data structures prior to use.
 * 
 * @return true if initialization succeeds
 */
//------------------------------------------------------------------------------
bool InternalOptimizer::Initialize()
{
   bool retval = Optimizer::Initialize();
   
   return retval;
}

//------------------------------------------------------------------------------
//  void ReportProgress()
//------------------------------------------------------------------------------
/**
 * Shows the progress string to the user.
 *
 * This default version just passes the progress string to the MessageInterface.
 */
//------------------------------------------------------------------------------
void InternalOptimizer::ReportProgress(const SolverState forState)
{
   // The command requests this complete snapshot explicitly after refreshing
   // physical reporting values. Provider callbacks retain their original text.
   if (typeName != "VF13ad" || forState != CHECKINGRUN)
   {
      Solver::ReportProgress(forState);
      return;
   }
   if (!showProgress || !isInitialized)
      return;

   // The provider restores its nominal residuals before this callback. Use the
   // same physical constraint reconstruction as the solver-window listener;
   // the last finite-difference evaluation's achieved-value cache can be stale.
   const RealArray equality = *GetSolverData("EqConstraints");
   const RealArray inequality = *GetSolverData("IneqConstraints");
   std::ostringstream report;
   report << std::setprecision(17)
          << "VF13ad iteration result " << iterationsTaken << "\n";
   for (UnsignedInt i = 0; i < variableNames.size(); ++i)
   {
      report << "   Control " << variableNames[i] << " = " << unscaledVariable.at(i) << "\n";
      if (textFileMode == "Verbose")
         report << "   Scaled control " << variableNames[i] << " = " << variable.at(i) << "\n";
   }
   if (objectiveDefined)
      report << "   Objective " << objectiveFnName << " = " << cost << "\n";
   for (UnsignedInt i = 0; i < eqConstraintNames.size(); ++i)
      report << "   Constraint " << eqConstraintNames[i] << " =; desired = "
             << eqConstraintDesiredValues.at(i) << "; achieved = " << equality.at(i)
             << "; residual = " << equality.at(i) - eqConstraintDesiredValues.at(i) << "\n";
   for (UnsignedInt i = 0; i < ineqConstraintNames.size(); ++i)
   {
      const bool lower = ineqConstraintOp.at(i) == 1;
      report << "   Constraint " << ineqConstraintNames[i] << (lower ? " >=" : " <=")
             << "; desired = " << ineqConstraintDesiredValues.at(i)
             << "; achieved = " << inequality.at(i) << "; residual = "
             << (lower ? ineqConstraintDesiredValues.at(i) - inequality.at(i)
                       : inequality.at(i) - ineqConstraintDesiredValues.at(i)) << "\n";
   }
   report << "End VF13ad iteration result " << iterationsTaken << "\n";
   const std::string text = report.str();
   MessageInterface::ShowMessage("%s", text.c_str());
   if (textFile.is_open())
   {
      textFile << text;
      textFile.flush();
      if (!textFile)
         throw SolverException("Cannot write VF13ad iteration results to " + solverTextFileFullPath);
   }
}

