//$Id$
//------------------------------------------------------------------------------
//                            SolverBranchCommand
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
// number NNG06CA54C
//
// Author: Darrel J. Conway
// Created: 2006/10/20
//
/**
 * Definition for the Solver loop command base class (Target, Optimize and 
 * Iterate).
 */

//------------------------------------------------------------------------------


#include <exception>
#include <typeinfo>
#include <memory>
#include "SolverBranchCommand.hpp"
#include "Spacecraft.hpp"
#include "Vary.hpp"                // For SetInitialValue() method
#include "Subscriber.hpp"
#include "MessageInterface.hpp"
#include "FormationInterface.hpp"
#include "NativeRestoration.hpp"
#include "TransientForceSelection.hpp"
#include "ListenerManagerInterface.hpp"
#include "StringUtil.hpp"

#include <sstream>                 // for <<
#include <set>
#include <utility>
#include <algorithm>

//#define DEBUG_PARSING
//#define DEBUG_OPTIONS
//#define DEBUG_SOLVERBRANCHCOMMAND_INIT
//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif


struct SolverCommandResources
{
   struct Member
   {
      GmatBase *source;
      std::weak_ptr<const void> identity;
   };
   struct Snapshot
   {
      GmatBase *source;
      std::weak_ptr<const void> identity;
      std::shared_ptr<const FunctionValueContext> memberOwner;
      std::unique_ptr<GmatBase> value;
      std::vector<Member> members;
   };
   // Keep one local instance per native solver class, retaining identity for
   // cached command/Parameter references across ordinary reinitialization.
   std::vector<std::unique_ptr<Solver>> solvers;
   std::vector<Snapshot> snapshots;
   TransientForceSelection transientSelection;
   Publisher *checkpointPublisher = nullptr;
   Publisher::ManeuveringState maneuveringState;
};


namespace {
thread_local std::vector<SolverBranchCommand *> plotSolvers;
void setPlotState(Publisher *publisher, Gmat::RunState state)
{
   if (!publisher) return;
   for (auto *subscriber : publisher->GetSubscriberList())
      if (subscriber->IsOfType("OrbitPlot") || subscriber->IsOfType("XYPlot") ||
          subscriber->IsOfType("GroundTrack"))
         subscriber->SetRunState(SolverBranchCommand::GetPlotRunState(state));
}
}

Gmat::RunState SolverBranchCommand::GetPlotRunState(Gmat::RunState state)
{
   return GetSubscriberRunState(state);
}

Gmat::RunState SolverBranchCommand::GetSubscriberRunState(Gmat::RunState state)
{
   // An inner accepted pass is still a trial until every enclosing solver is
   // accepted. Subscribers opt in without changing the publisher's state.
   for (const auto *command : plotSolvers)
      if (command->currentRunState == Gmat::SOLVING) return Gmat::SOLVING;
   return state;
}

SolverBranchCommand::PlotExecutionScope::PlotExecutionScope(SolverBranchCommand *value)
   : command(value)
{
   // Track live states: Target/Optimize change from trial to accepted within
   // one dispatch, and can execute an inner solver during initialization.
   plotSolvers.push_back(command);
}

SolverBranchCommand::PlotExecutionScope::~PlotExecutionScope()
{
   plotSolvers.pop_back();
   // Scope destruction also removes the context on Stop or runtime failure.
   // Restore the enclosing display state, without replacing any engine error.
   try { setPlotState(command->publisher, command->currentRunState); } catch (...) {}
}

//------------------------------------------------------------------------------
//  SolverBranchCommand(const std::string &typeStr)
//------------------------------------------------------------------------------
/**
 * Creates a SolverBranchCommand command.  (default constructor)
 * 
 * @param <typeStr> The type name of the SolverBranchCommand.
 */
//------------------------------------------------------------------------------
SolverBranchCommand::SolverBranchCommand(const std::string &typeStr) :
   BranchCommand  (typeStr),
   solverName     (""),
   theSolver      (NULL),
   startMode      (RUN_AND_SOLVE),
   exitMode       (DISCARD_AND_CONTINUE),
   showProgressWindow(true),
   specialState   (Solver::INITIALIZING),
   queryStatusID  (-1)
{
   parameterCount = SolverBranchCommandParamCount;
   objectTypeNames.push_back("SolverBranchCommand");
   objectTypeNames.push_back("SolverCommand");
   
   solverModes.push_back("RunInitialGuess");
   solverModes.push_back("Solve");
//   solverModes.push_back("RunCorrected");
   
   exitModes.push_back("DiscardAndContinue");
   exitModes.push_back("SaveAndContinue");
   exitModes.push_back("Stop");

}

//------------------------------------------------------------------------------
//  SolverBranchCommand(const std::string &typeStr)
//------------------------------------------------------------------------------
/**
 * Destroys a SolverBranchCommand command.  (destructor)
 */
//------------------------------------------------------------------------------
SolverBranchCommand::~SolverBranchCommand()
{
   ClearListeners();
   activeSubscribers.clear();
   localStore.clear();
   theSolver = nullptr;
   // GmatCommand retains the resource owner until branch and following
   // commands have been destroyed. Never delete a configured/master solver.
}


//------------------------------------------------------------------------------
//  SolverBranchCommand(const SolverBranchCommand &sbc)
//------------------------------------------------------------------------------
/**
 * Creates a SolverBranchCommand command based on another.  (Copy constructor)
 * 
 * @param <sbc> The SolverBranchCommand that is copied into this instance.
 */
//------------------------------------------------------------------------------
SolverBranchCommand::SolverBranchCommand(const SolverBranchCommand& sbc) :
   BranchCommand  (sbc),
   solverName     (sbc.solverName),
   theSolver      (NULL),
   startMode      (sbc.startMode),
   exitMode       (sbc.exitMode),
   showProgressWindow (sbc.showProgressWindow),
   specialState   (Solver::INITIALIZING),
   queryStatusID  (-1),
   solverModes    (sbc.solverModes),
   exitModes      (sbc.exitModes)
{
}

//------------------------------------------------------------------------------
//  SolverBranchCommand& operator=(const SolverBranchCommand& sbc)
//------------------------------------------------------------------------------
/**
 * Copies a SolverBranchCommand command.  (Assignment operator)
 * 
 * @param <sbc> The SolverBranchCommand that is copied into this instance.
 * 
 * @return This instance, configured to match sbc.
 */
//------------------------------------------------------------------------------
SolverBranchCommand& SolverBranchCommand::operator=(
   const SolverBranchCommand& sbc)
{
   if (&sbc != this)
   {
      FreeLoopData();
      ClearListeners();
      activeSubscribers.clear();
      BranchCommand::operator=(sbc);
      solverName   = sbc.solverName;
      theSolver    = NULL;
      startMode    = sbc.startMode;
      exitMode     = sbc.exitMode;
      showProgressWindow = sbc.showProgressWindow;
      specialState = Solver::INITIALIZING;
      queryStatusID = -1;
      solverModes = sbc.solverModes;
      exitModes = sbc.exitModes;
   }
   
   return *this;
}

//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Performs the initialization needed to run the BranchCommand command.
 *
 * @return true if the GmatCommand is initialized, false if an error occurs.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::Initialize()
{
   SetCloneNotificationsEnabled(false);
   #ifdef DEBUG_SOLVERBRANCHCOMMAND_INIT
   ShowCommand("SolverBranchCommand::Initialize() entered ", "this=", this);
   #endif
   
   if (!BranchCommand::Initialize())
      return false;

   // Sandbox's top-level action does not visit solvers inside ordinary
   // branches or functions. Each initialized solver owns its notification
   // marker; reinitialization also clears markers left by an edited sequence.
   SolverBranchCommand::TakeAction("SetLastVaryCommand");

   if (theSolver)
      queryStatusID = theSolver->GetParameterID("IntegerSolverStatus");

   if (showProgressWindow) 
   {
      // THIS IS IDIOTIC BUT SINCE INITIALIZE IS CALLED TWICE, JUST DON'T CREATE NO NEW LISTENERS
      // AFTER THE FIRST ONE (Grubb.T 2015-10-09)
      if (listeners.size() == 0)
      {
         ISolverListener* aListener = ListenerManagerInterface::CreateSolverListener(GetGeneratingString(Gmat::NO_COMMENTS), "", 0, 0, 0, 0, false);
         if (aListener != NULL)
            AddListener( aListener );
      }
   }
   #ifdef DEBUG_SOLVERBRANCHCOMMAND_INIT
   ShowCommand("SolverBranchCommand::Initialize() exited ", "this=", this);
   #endif
   SetCloneNotificationsEnabled(true);
   return true;

}


//------------------------------------------------------------------------------
//  GmatCommand* GetNext()
//------------------------------------------------------------------------------
/**
 * Access the next command in the mission sequence.
 *
 * For SolverBranchCommands, this method returns its own pointer while the child
 * commands are executing, and it tells the Publisher about a state change after
 * the Solver has finished its work.
 *
 * @return The next command, or NULL if the sequence has finished executing.
 */
//------------------------------------------------------------------------------
GmatCommand* SolverBranchCommand::GetNext()
{
   // Return the next pointer in the command sequence if this command -- 
   // includng its branches -- has finished executing.
   if ((commandExecuting) && (!commandComplete))
      return this;
   
   // Set state back to RUNNING
   ChangeRunState(Gmat::RUNNING);
   
   if (((commandExecuting) && (commandComplete)) && (exitMode == STOP))
   {
      std::string msg = 
         "Mission interrupted -- Solver is running with ExitMode = \"Stop\"\n";
      throw CommandException(msg);
   }
   
   return next;
}





//------------------------------------------------------------------------------
// protected methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// void StoreLoopData()
//------------------------------------------------------------------------------
/**
 * Makes local copies of the data so that a solver loop can recover initial
 * data while iterating.
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::StoreLoopData()
{
   PrepareSolverResources();
   TransientForceSelection activeForces;
   activeForces.Capture(forces);
   Publisher::ManeuveringState publicationState;
   if (publisher) publicationState = publisher->CaptureManeuveringState();
   std::vector<GmatBase*> sources;
   std::set<GmatBase*> identities;
   std::set<GmatBase*> forbidden;
   for (ObjectMap *map : {objectMap, globalObjectMap})
      if (map)
         for (const auto &entry : *map)
         {
            GmatBase *object = entry.second;
            if (!object) continue;
            forbidden.insert(object);
            if ((object->IsOfType(Gmat::SPACECRAFT) || object->IsOfType(Gmat::FORMATION)) &&
                  identities.insert(object).second) sources.push_back(object);
         }
   for (auto *object : GetCommandOwnedObjects()) forbidden.insert(object);
   // A retained formation can reference private spacecraft that have no entry
   // in either script map. Capture each actual member once, including aliases.
   for (size_t i = 0; i < sources.size(); ++i)
      if (sources[i]->IsOfType(Gmat::FORMATION))
      {
         const ObjectArray members = sources[i]->GetRefObjectArray(Gmat::SPACEOBJECT);
         for (auto *member : members)
         {
            if (!member || (!member->IsOfType(Gmat::SPACECRAFT) && !member->IsOfType(Gmat::FORMATION)))
               throw CommandException("Cannot capture an unresolved native formation member.\n");
            forbidden.insert(member);
            if (identities.insert(member).second) sources.push_back(member);
         }
      }
   std::vector<SolverCommandResources::Snapshot> snapshots;
   snapshots.reserve(sources.size());
   GmatBase::FunctionValueCopyGuard nativeCopy;
   for (GmatBase *source : sources)
   {
      std::vector<SolverCommandResources::Member> members;
      if (source->IsOfType(Gmat::FORMATION))
      {
         const ObjectArray values = source->GetRefObjectArray(Gmat::SPACEOBJECT);
         for (auto *member : values) members.push_back({member, member->GetLifetimeToken()});
      }
      std::unique_ptr<GmatBase> copy(source->Clone());
      if (forbidden.count(copy.get()))
      {
         copy.release();
         throw CommandException("Loop snapshot Clone returned a source or existing owned object.\n");
      }
      if (!copy || typeid(*copy) != typeid(*source) || copy->GetTypeName() != source->GetTypeName())
         throw CommandException("Cannot copy the native loop snapshot type.\n");
      forbidden.insert(copy.get());
      // Frozen values keep the current frame/owner context. Rebinding a value
      // into another private scene here would change its restoration roles.
      snapshots.push_back({source, source->GetLifetimeToken(), source->GetFunctionValueContext(),
            std::move(copy), std::move(members)});
   }
   ObjectArray views;
   views.reserve(snapshots.size());
   for (const auto &snapshot : snapshots) views.push_back(snapshot.value.get());
   // All captures succeed before replacing the previous iteration's values.
   solverResources->snapshots.swap(snapshots);
   localStore.swap(views);
   solverResources->transientSelection = std::move(activeForces);
   solverResources->checkpointPublisher = publisher;
   solverResources->maneuveringState = std::move(publicationState);
}


//------------------------------------------------------------------------------
// void ResetLoopData()
//------------------------------------------------------------------------------
/**
 * Resets starting data from local copies so that a solver loop can iterate.
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::ResetLoopData()
{
   if (!solverResources || !solverResources->transientSelection.IsCaptured()) return;
   auto activeForces = solverResources->transientSelection.PrepareRestoration(forces);
   if (solverResources->checkpointPublisher != publisher)
      throw CommandException("The captured loop publisher is no longer selected.");
   auto maneuvering = publisher ? publisher->PrepareManeuveringRestoration(
         solverResources->maneuveringState) : std::unique_ptr<NativeRestorationPlan>();
   auto present = [&](GmatBase *object)
   {
      for (ObjectMap *map : {objectMap, globalObjectMap})
         if (map) for (const auto &entry : *map) if (entry.second == object) return true;
      return false;
   };
   // Validate the complete restore set first. A new object with the same name
   // cannot inherit an earlier object's numerical checkpoint.
   for (const auto &snapshot : solverResources->snapshots)
      if (!GmatBase::IsLiveReference(snapshot.source, snapshot.identity) ||
            typeid(*snapshot.source) != typeid(*snapshot.value))
         throw CommandException("The original loop snapshot target is no longer in this scope.\n");
   std::set<GmatBase*> reachable;
   std::set<GmatBase*> forbidden;
   for (ObjectMap *map : {objectMap, globalObjectMap})
      if (map) for (const auto &entry : *map) forbidden.insert(entry.second);
   for (auto *object : GetCommandOwnedObjects()) forbidden.insert(object);
   for (const auto &snapshot : solverResources->snapshots)
   {
      if (present(snapshot.source)) reachable.insert(snapshot.source);
      forbidden.insert(snapshot.source);
      forbidden.insert(snapshot.value.get());
      if (snapshot.source->IsOfType(Gmat::FORMATION))
      {
         for (const auto &member : snapshot.members)
            if (!GmatBase::IsLiveReference(member.source, member.identity))
               throw CommandException("The actual captured formation member is no longer alive.\n");
      }
   }
   // Follow the captured graph from actual script-map roots. A loop may
   // change its current membership; captured owners keep the original private
   // destinations alive. An equal-name replacement is never a fallback.
   for (size_t previous = 0; previous != reachable.size();)
   {
      previous = reachable.size();
      for (const auto &snapshot : solverResources->snapshots)
         if (reachable.count(snapshot.source))
            for (const auto &member : snapshot.members) reachable.insert(member.source);
   }
   ObjectArray destinations, values;
   std::vector<std::unique_ptr<GmatBase>> prepared, probes;
   prepared.reserve(solverResources->snapshots.size());
   probes.reserve(solverResources->snapshots.size());
   auto clone = [&](GmatBase &source, bool restorationProbe = false)
   {
      GmatBase::FunctionValueCopyGuard nativeCopy;
      auto *formation = restorationProbe ? dynamic_cast<FormationInterface*>(&source) : nullptr;
      std::unique_ptr<GmatBase> value(formation ? formation->CloneRestorationProbe() : source.Clone());
      if (value && forbidden.count(value.get()))
      {
         value.release();
         throw CommandException("Loop restoration Clone returned a borrowed object.\n");
      }
      if (!value || typeid(*value) != typeid(source) || value->GetTypeName() != source.GetTypeName())
         throw CommandException("Cannot prepare a native loop restoration value.\n");
      forbidden.insert(value.get());
      return value;
   };
   for (const auto &snapshot : solverResources->snapshots)
   {
      if (!reachable.count(snapshot.source))
         throw CommandException("The original loop snapshot target is no longer in this scope.\n");
      auto value = clone(*snapshot.value);
      if (!value->SetName(snapshot.source->GetName()))
         throw CommandException("Cannot retain a loop restoration resource name.\n");
      value->SetFullName(snapshot.source->GetFullName());
      // Navigation commands can enable precision time after this loop's
      // initial snapshot. Preserve the live mode and its storage, promoting
      // only the detached saved value from the epoch it originally used.
      // All other native layout/role checks remain in the restoration path.
      auto *liveState = dynamic_cast<SpaceObject*>(snapshot.source);
      auto *savedState = dynamic_cast<SpaceObject*>(value.get());
      if (liveState && savedState && liveState->HasPrecisionTime() &&
            !savedState->HasPrecisionTime())
      {
         const Real savedEpoch = savedState->GetEpoch();
         savedState->SetPrecisionTimeFlag(true);
         savedState->SetEpochGT(GmatTime(savedEpoch));
      }
      destinations.push_back(snapshot.source);
      values.push_back(value.get());
      prepared.push_back(std::move(value));
   }
   NativeRestoration::Validate(values, destinations);
   std::vector<std::unique_ptr<FormationRestorationPlan>> layouts;
   layouts.reserve(values.size());
   for (size_t i = 0; i < values.size(); ++i)
   {
      auto *formation = dynamic_cast<FormationInterface*>(destinations[i]);
      auto *frozen = dynamic_cast<FormationInterface*>(values[i]);
      ObjectArray members;
      std::unique_ptr<FormationRestorationPlan> layout;
      if (formation)
      {
         if (!frozen) throw CommandException("Incompatible native formation restoration type.");
         for (const auto &member : solverResources->snapshots[i].members)
            members.push_back(member.source);
         layout = formation->PrepareRestorationLayout(*frozen, members);
      }
      auto probe = clone(*destinations[i], true);
      if (layout)
      {
         auto *trial = dynamic_cast<FormationInterface*>(probe.get());
         if (!trial) throw CommandException("Incompatible native formation restoration probe.");
         auto preparation = trial->PrepareRestorationLayout(*frozen, members);
         if (preparation) preparation->Apply();
      }
      layouts.push_back(std::move(layout));
      GmatBase::NativeRestorationGuard restore;
      NativeRestoration::Copy({values[i]}, {probe.get()});
      probes.push_back(std::move(probe));
   }
   // Every layout allocation and native probe succeeds before changing a live
   // root. Swaps install captured membership/storage; ordinary fixed-layout
   // native Copy then restores all packed and member values.
   for (auto &layout : layouts) if (layout) layout->Apply();
   {
      GmatBase::NativeRestorationGuard restore;
      NativeRestoration::Copy(values, destinations);
   }
   if (forces) forces->swap(activeForces);
   if (maneuvering) maneuvering->Apply();
   BranchCommand::TakeAction("ResetLoopData");
}


//------------------------------------------------------------------------------
//  void RunComplete()
//------------------------------------------------------------------------------
/**
 * Tells the sequence that the run was ended, possibly before reaching the end.
 *
 * BranchCommands clear the "current" pointer and call RunComplete on their
 * branches, ensuring that the command sequence has reset the branches to an
 * idle state.
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::RunComplete()
{
   std::exception_ptr firstError;
   try { BranchCommand::RunComplete(); }
   catch (...) { firstError = std::current_exception(); }
   ClearListeners();
   if (firstError) std::rethrow_exception(firstError);
}

//------------------------------------------------------------------------------
// void FreeLoopData()
//------------------------------------------------------------------------------
/**
 * Cleans up starting data store after solver has completed.
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::FreeLoopData()
{
   localStore.clear();
   if (solverResources)
   {
      solverResources->snapshots.clear();
      solverResources->transientSelection.Clear();
      solverResources->checkpointPublisher = nullptr;
      solverResources->maneuveringState = {};
   }
}


//------------------------------------------------------------------------------
// bool InterpretAction()
//------------------------------------------------------------------------------
/**
 * Parses the command string and builds the corresponding command structures.
 *
 * The Solver commands have one of the following syntaxes:
 * 
 *    Target DC
 *    Target DC {SolveMode = Solve}
 *    Target DC {ExitMode = DiscardAndContinue}
 *    Target DC {SolveMode = RunInitialGuess, ExitMode = SaveAndContinue}
 * 
 *    Optimize VF13
 *    Optimize VF13 {SolveMode = Solve}
 *    Optimize VF13 {ExitMode = SaveAndContinue}
 *    Optimize VF13 {SolveMode = RunInitialGuess, ExitMode = Stop}
 * 
 * If the undecorated command is used, the default values (SolveMode = Solve, 
 * ExitMode = DiscardAndContinue) are used.
 *
 * This method breaks the script line into the corresponding pieces, and stores
 * the name of the Solver and all specified solver options.
 *
 * @return true on successful parsing of the command.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::InterpretAction()
{
   #ifdef DEBUG_SOLVERBC_ASSEMBLE
      MessageInterface::ShowMessage
         ("%s::InterpretAction() genString = \"%s\"\n", typeName.c_str(),
          generatingString.c_str());
   #endif
   
   StringArray blocks = parser.DecomposeBlock(generatingString);

   StringArray chunks = parser.SeparateBrackets(blocks[0], "{}", " ", false);

   #ifdef DEBUG_PARSING
      MessageInterface::ShowMessage("Chunks from \"%s\":\n", 
            blocks[0].c_str());
      for (StringArray::iterator i = chunks.begin(); i != chunks.end(); ++i)
         MessageInterface::ShowMessage("   \"%s\"\n", i->c_str());
   #endif
   
   if (chunks.size() < 2)
      throw CommandException(typeName + "::InterpretAction() cannot identify "
            "the Solver -- is it missing? -- in line\n" + generatingString);

   if (chunks.size() > 3)
      throw CommandException(typeName + 
            "::InterpretAction() found too many components to parse in the "
            "line\n" + generatingString);

   if (chunks[0] != typeName)
      throw CommandException(typeName + "::InterpretAction() does not identify "
            "the correct Solver type in line\n" + generatingString);
   
   solverName = chunks[1];

   if (chunks.size() == 3)
      CheckForOptions(chunks[2]);

   #ifdef DEBUG_PARSING
      MessageInterface::ShowMessage("%s::InterpretAction for \"%s\", type = %s\n", 
            typeName.c_str(), generatingString.c_str(), typeName.c_str());
      MessageInterface::ShowMessage("   Solver name:     \"%s\"\n", 
            solverName.c_str());
      MessageInterface::ShowMessage("   SolveMode:          %d\n", startMode);
      MessageInterface::ShowMessage("   ExitMode:           %d\n", exitMode);
   #endif
   
   return true;
}

//---------------------------------------------------------------------------
// bool RenameRefObject(const UnsignedInt type,
//                      const std::string &oldName, const std::string &newName)
//---------------------------------------------------------------------------
/**
 * Renames referenced objects.
 *
 * @param type Type of the object that is renamed.
 * @param oldName The current name for the object.
 * @param newName The name the object has when this operation is complete.
 *
 * @return true on success.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::RenameRefObject(const UnsignedInt type,
                                          const std::string &oldName,
                                          const std::string &newName)
{
   if (type == Gmat::SOLVER)
   {
      if (solverName == oldName)
         solverName = newName;
   }
   
   BranchCommand::RenameRefObject(type, oldName, newName);
   
   return true;
}

//------------------------------------------------------------------------------
// const ObjectTypeArray& GetRefObjectTypeArray()
//------------------------------------------------------------------------------
/**
 * Retrieves the list of ref object types used by the Vary.
 *
 * @return the list of object types.
 * 
 */
//------------------------------------------------------------------------------
const ObjectTypeArray& SolverBranchCommand::GetRefObjectTypeArray()
{
   refObjectTypes.clear();
   ObjectTypeArray parentRefTypes = BranchCommand::GetRefObjectTypeArray();
   copy(parentRefTypes.begin(), parentRefTypes.end(), back_inserter(refObjectTypes));
   refObjectTypes.push_back(Gmat::SOLVER);
   return refObjectTypes;
}


//------------------------------------------------------------------------------
// const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Retrieves the list of ref objects used by the Vary.
 *
 * @param <type> The type of object desired, or Gmat::UNKNOWN_OBJECT for the
 *               full list.
 * 
 * @return the list of object names.
 * 
 */
//------------------------------------------------------------------------------
const StringArray& SolverBranchCommand::GetRefObjectNameArray(const UnsignedInt type)
{
   refObjectNames.clear();
   StringArray parentRefNames = BranchCommand::GetRefObjectNameArray(type);
   copy(parentRefNames.begin(), parentRefNames.end(), back_inserter(refObjectNames));
   
   if (type == Gmat::UNKNOWN_OBJECT || type == Gmat::SOLVER)
      refObjectNames.push_back(solverName);
   
   return refObjectNames;
}

//------------------------------------------------------------------------------
// void CheckForOptions(std::string &opts)
//------------------------------------------------------------------------------
void SolverBranchCommand::CheckForOptions(std::string &opts)
{
   StringArray chunks = parser.SeparateBrackets(opts, "{}", ", ", true);

   #ifdef DEBUG_PARSING
      MessageInterface::ShowMessage("Chunks from \"%s\":\n", opts.c_str());
      for (StringArray::iterator i = chunks.begin(); i != chunks.end(); ++i)
         MessageInterface::ShowMessage("   \"%s\"\n", i->c_str());
   #endif
   
   for (StringArray::iterator i = chunks.begin(); i != chunks.end(); ++i)
   {
      StringArray option = parser.SeparateBy(*i, "= ");

      #ifdef DEBUG_PARSING
         MessageInterface::ShowMessage("Options from \"%s\":\n", i->c_str());
         for (StringArray::iterator i = option.begin(); i != option.end(); ++i)
            MessageInterface::ShowMessage("   \"%s\"\n", i->c_str());
      #endif
         
      if (option.size() != 2)
         throw CommandException(typeName + "::InterpretAction() Solver option "
               "is not in the form option = value in line\n" + 
               generatingString);
         
      if (option[0] == "SolveMode")
      {
         if (option[1] == "Solve")
            startMode = RUN_AND_SOLVE;
         else if (option[1] == "RunInitialGuess")
            startMode = RUN_INITIAL_GUESS;
         else
            throw CommandException(typeName + "::InterpretAction() Solver "
                  "SolveMode option " + option[1] + 
                  " is not a recognized value on line\n" + generatingString +
                  "\nAllowed values are \"Solve\" and \"RunInitialGuess\"\n");
      }
      else if (option[0] == "ExitMode")
      {
         #ifdef DEBUG_EXITMODE
            MessageInterface::ShowMessage("Setting solvemode to %s\n",
                  option[1].c_str());
         #endif
         if (option[1] == "DiscardAndContinue")
            exitMode = DISCARD_AND_CONTINUE;
         else if (option[1] == "SaveAndContinue")
            exitMode = SAVE_AND_CONTINUE;
         else if (option[1] == "Stop")
            exitMode = STOP;
         else
            throw CommandException(typeName + "::InterpretAction() Solver "
                  "ExitMode option " + option[1] + 
                  " is not a recognized value on line\n" + generatingString +
                  "\nAllowed values are \"DiscardAndContinue\", "
                  "\"SaveAndContinue\", and \"Stop\"\n");
      }
      else if (option[0] == "ShowProgressWindow")
      {
         #ifdef DEBUG_EXITMODE
            MessageInterface::ShowMessage("Setting ShowProgressWindow to %s\n",
                  option[1].c_str());
         #endif
         std::string opt1 = GmatStringUtil::ToUpper(option[1]);
         if (opt1 == "TRUE")
            showProgressWindow = true;
         else if (opt1 == "FALSE")
            showProgressWindow = false;
         else
            throw CommandException(typeName + "::InterpretAction() Solver "
                  "ShowProgressWindow option " + option[1] + 
                  " is not a recognized value on line\n" + generatingString +
                  "\nAllowed values are \"true\", "
                  "\"false\"\n");
      }
      else
      {
         throw CommandException(typeName + 
               "::InterpretAction() Solver option " + option[0] + 
               " is not a recognized option on line\n" + generatingString +
                                 "\nAllowed options are \"SolveMode\" and "
               "\"ExitMode\"\n");
      }         
   }
}


std::string SolverBranchCommand::GetSolverOptionText()
{
   #ifdef DEBUG_OPTIONS
      MessageInterface::ShowMessage("Entering GetSolverOptionText with startMode = %d, and exitMode = %d\n",
            (Integer) startMode, (Integer) exitMode);
   #endif
   std::string optionString = "";
   optionString += " {SolveMode = ";
   optionString += GetStringParameter(SOLVER_SOLVE_MODE);
   optionString += ", ExitMode = ";
   optionString += GetStringParameter(SOLVER_EXIT_MODE);
   optionString += ", ShowProgressWindow = ";
   if (showProgressWindow)
      optionString += "true";
   else
      optionString += "false";
   optionString += "}";

   #ifdef DEBUG_OPTIONS
      MessageInterface::ShowMessage("Exiting GetSolverOptionText and optionString = %s\n",
            optionString.c_str());
   #endif
   
   return optionString;
}


//-----------------------------------------------------------------------------
// bool TakeAction(const std::string &action, const std::string &actionData)
//-----------------------------------------------------------------------------
/**
 * Applies a programmatic action to the comamnd
 *
 * @param action String describing the desired action
 * @param actionData Additional data used by teh action (when needed)
 * 
 * @return true is the action was applied, false if not.
 */
//-----------------------------------------------------------------------------
bool SolverBranchCommand::TakeAction(const std::string &action, 
      const std::string &actionData)
{
   #ifdef DEBUG_SOLVER_ACTIONS
      MessageInterface::ShowMessage("Taking action %s\n", action.c_str());
   #endif
   if (action == "ApplyCorrections")
   {
      if (theSolver == NULL)
      {
         MessageInterface::PopupMessage(Gmat::INFO_, 
               "Please run the mission first.  Corrections cannot be applied "
               "until the solver control sequence has been run.");
         return true;
      }
      // This action walks through the solver loop and passes the Solver's 
      // variable values to the Vary commands that use that solver, updating the
      // initial values for the variables.  The solver must have run once first,
      // though it need not have converged.
      Integer status = theSolver->GetIntegerParameter(
            theSolver->GetParameterID("IntegerSolverStatus"));
      
      if ((status == Gmat::CREATED) ||
          (status == Gmat::COPIED) ||
          (status == Gmat::INITIALIZED))
      {
         MessageInterface::PopupMessage(Gmat::INFO_, 
               "Please run the mission first.  Corrections cannot be applied "
               "until the solver control sequence has been run.");
         return true;
      }
      
      ApplySolution();
   
      return true;
   }
   else if (action == "SetLastVaryCommand")
   {
      // Preserve the last-in-script-order notification convention, including
      // ordinary branch contents. Nested solvers mark their own controls.
      Vary *lastVary = NULL;
      for (GmatCommand *command : GetCommandsInSolverScope())
         if (Vary *vary = dynamic_cast<Vary*>(command))
         {
            vary->SetIsThisLastVaryCommand(false);
            lastVary = vary;
         }

      if (lastVary != NULL)
         lastVary->SetIsThisLastVaryCommand(true);
      
      return true;
   }
   
   return BranchCommand::TakeAction(action, actionData);
}


// Parameter access methods

//------------------------------------------------------------------------------
//  std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Read accessor for parameter names.
 *
 * @param <id> the ID of the parameter.
 *
 * @return the text string for the parameter.
 */
//------------------------------------------------------------------------------
std::string SolverBranchCommand::GetParameterText(const Integer id) const
{
   if (id == SOLVER_NAME_ID)
      return "SolverName";
   if (id == SOLVER_SOLVE_MODE)
      return "SolveMode";
   if (id == SOLVER_EXIT_MODE)
       return "ExitMode";
   if (id == SOLVER_SOLVE_MODE_OPTIONS)
       return "SolveModeOptions";
   if (id == SOLVER_EXIT_MODE_OPTIONS)
      return "ExitModeOptions";
   if (id == SOLVER_SHOW_PROGRESS)
      return "ShowProgressWindow";
    
   return BranchCommand::GetParameterText(id);
}


//------------------------------------------------------------------------------
//  Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
/**
 * Read accessor for parameter IDs.
 *
 * @param <str> the text description of the parameter.
 *
 * @return the integer ID for the parameter.
 */
//------------------------------------------------------------------------------
Integer SolverBranchCommand::GetParameterID(const std::string &str) const
{
   if (str == "SolverName")
      return SOLVER_NAME_ID;
   if (str == "SolveMode")
      return SOLVER_SOLVE_MODE;
   if (str == "ExitMode")
      return SOLVER_EXIT_MODE;
   if (str == "SolveModeOptions")
      return SOLVER_SOLVE_MODE_OPTIONS;
   if (str == "ExitModeOptions")
      return SOLVER_EXIT_MODE_OPTIONS;
   if (str == "ShowProgressWindow")
      return SOLVER_SHOW_PROGRESS;
    
   return BranchCommand::GetParameterID(str);
}


//------------------------------------------------------------------------------
//  Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Read accessor for parameter types.
 *
 * @param <id> the integer ID of the parameter.
 *
 * @return the type of the parameter.
 */
//------------------------------------------------------------------------------
Gmat::ParameterType SolverBranchCommand::GetParameterType(const Integer id) const
{
   if (id == SOLVER_NAME_ID)
      return Gmat::STRING_TYPE;
   if (id == SOLVER_SOLVE_MODE)
      return Gmat::STRING_TYPE;
   if (id == SOLVER_SHOW_PROGRESS)
      return Gmat::BOOLEAN_TYPE;
   if (id == SOLVER_SOLVE_MODE_OPTIONS)
      return Gmat::STRINGARRAY_TYPE;
    
   return BranchCommand::GetParameterType(id);
}


//------------------------------------------------------------------------------
//  std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Read accessor for parameter type data description.
 *
 * @param <id> the integer ID of the parameter.
 *
 * @return a string describing the type of the parameter.
 */
//------------------------------------------------------------------------------
std::string SolverBranchCommand::GetParameterTypeString(const Integer id) const
{
   if (id == SOLVER_NAME_ID)
      return PARAM_TYPE_STRING[Gmat::STRING_TYPE];
   if (id == SOLVER_SOLVE_MODE)
      return PARAM_TYPE_STRING[Gmat::STRING_TYPE];
   if (id == SOLVER_SHOW_PROGRESS)
      return PARAM_TYPE_STRING[Gmat::BOOLEAN_TYPE];
   if (id == SOLVER_SOLVE_MODE_OPTIONS)
      return PARAM_TYPE_STRING[Gmat::STRINGARRAY_TYPE];
    
   return BranchCommand::GetParameterTypeString(id);
}


//------------------------------------------------------------------------------
//  bool SetStringParameter(const Integer id, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Write accessor for string parameters.
 *
 * @param <id> the integer ID of the parameter.
 * @param <value> the new string stored in the parameter.
 *
 * @return true on success, false on failure.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::SetStringParameter(const Integer id, const std::string &value)
{
   if (id == SOLVER_NAME_ID)
   {
      solverName = value;
      return true;
   }

   if (id == SOLVER_SOLVE_MODE) 
   {
      if (value == "RunInitialGuess")
         startMode = RUN_INITIAL_GUESS;
      else if (value == "Solve")
         startMode = RUN_AND_SOLVE;
      else if (value == "RunCorrected")
         startMode = RUN_SOLUTION;
      else
         throw CommandException("Unknown solver mode \"" + value + 
               "\"; known values are {\"RunInitialGuess\", \"Solve\" "
               "\"RunCorrected\"}");
      return true;
   }
    
   if (id == SOLVER_EXIT_MODE) 
   {
      if (value == "DiscardAndContinue")
         exitMode = DISCARD_AND_CONTINUE;
      else if (value == "SaveAndContinue")
         exitMode = SAVE_AND_CONTINUE;
      else if (value == "Stop")
         exitMode = STOP;
      else
         throw CommandException("Unknown solver exit mode \"" + value + 
               "\"; known values are {\"DiscardAndContinue\", "
               "\"SaveAndContinue\", \"Stop\"}");
      return true;
   }
   
   return BranchCommand::SetStringParameter(id, value);
}

std::string SolverBranchCommand::GetStringParameter(const Integer id) const
{
   if (id == SOLVER_NAME_ID)
      return solverName;

   if (id == SOLVER_SOLVE_MODE) 
   {
      if (startMode == RUN_INITIAL_GUESS)
         return "RunInitialGuess";
      if (startMode == RUN_AND_SOLVE)
         return "Solve";
      if (startMode == RUN_SOLUTION)
         return "RunCorrected";
   }
   
   if (id == SOLVER_EXIT_MODE) 
   {
      if (exitMode == DISCARD_AND_CONTINUE)
         return "DiscardAndContinue";
      if (exitMode == SAVE_AND_CONTINUE)
         return "SaveAndContinue";
      if (exitMode == STOP)
         return "Stop";
   }
   
   return BranchCommand::GetStringParameter(id);
}


std::string SolverBranchCommand::GetStringParameter(const std::string &label) const
{
   return GetStringParameter(GetParameterID(label));
}


const StringArray& SolverBranchCommand::GetStringArrayParameter(const Integer id) const
{
   if (id == SOLVER_SOLVE_MODE_OPTIONS)
      return solverModes;
   
   if (id == SOLVER_EXIT_MODE_OPTIONS)
      return exitModes;
   
   return BranchCommand::GetStringArrayParameter(id);
}


const StringArray& SolverBranchCommand::GetStringArrayParameter(const std::string &label) const
{
   return GetStringArrayParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
//  bool GetBooleanParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param id The integer ID for the parameter.
 *
 * @return the boolean value for this parameter, or throw an exception if the
 *         parameter access in invalid.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::GetBooleanParameter(const Integer id) const
{
   if (id == SOLVER_SHOW_PROGRESS)
      return showProgressWindow;
   
   return BranchCommand::GetBooleanParameter(id);
}


//------------------------------------------------------------------------------
//  bool SetBooleanParameter(const Integer id, const bool value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new value.
 * 
 * @return the boolean value for this parameter, or throw an exception if the 
 *         parameter is invalid or not boolean.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::SetBooleanParameter(const Integer id, const bool value)
{
   if (id == SOLVER_SHOW_PROGRESS)
   {
      showProgressWindow = value;
      return showProgressWindow;
   }
   
   return BranchCommand::SetBooleanParameter(id, value);
}

//------------------------------------------------------------------------------
//  bool GetBooleanParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param <label> The (string) label for the parameter.
 *
 * @return the boolean value for this parameter, or false if the parameter is
 *         not boolean.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::GetBooleanParameter(const std::string &label) const
{
   Integer id = GetParameterID(label);
   return GetBooleanParameter(id);
}


//------------------------------------------------------------------------------
//  bool SetBooleanParameter(const std::string &label, const bool value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param <label> The (string) label for the parameter.
 *
 * @return the boolean value for this parameter, or false if the parameter is
 *         not boolean.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::SetBooleanParameter(const std::string &label, const bool value)
{
   Integer id = GetParameterID(label);
   return SetBooleanParameter(id, value);
}


//------------------------------------------------------------------------------
// bool NeedsServerStartup()
//------------------------------------------------------------------------------
/**
 * Indicates in the engine needs to start an external process to run the command
 * 
 * The Sandbox calls this method and tells the Moderator to start the MATLAB
 * engine if a true value is returned.  Note that the method name is not MATLAB 
 * specific; future builds may provide additional interfaces so that specific
 * servers can be started -- for example, Octave -- rather than just assuming 
 * that MATLAB is the engine to start.
 * 
 * @return true if a solver is an external solver, false otherwise.
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::NeedsServerStartup()
{
   if (theSolver == NULL)
      throw CommandException("The Solver pointer is not set in command\n" +
               GetGeneratingString());
   
   //if (theSolver->IsSolverInternal())
   //   return false;
   if (theSolver->NeedsServerStartup())
      return true;
   
   return false;
}

//------------------------------------------------------------------------------
// void ApplySolution()
//------------------------------------------------------------------------------
/**
 * Tells the Solver to update the initial values of the variables with the 
 * most recent solved state. 
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::ApplySolution()
{
   for (GmatCommand *command : GetCommandsInSolverScope())
      if (command->GetTypeName() == "Vary")
         ((Vary*)command)->SetInitialValue(theSolver);
}


//------------------------------------------------------------------------------
// std::vector<GmatCommand*> GetCommandsInSolverScope()
//------------------------------------------------------------------------------
/**
 * Walks this solver's structural command scope in script order, including all
 * arms of ordinary control-flow branches. Each branch ends at its owner's
 * back-link. A nested solver owns its own controls, even when it uses the same
 * configured solver resource as its parent, so its children are not included.
 * Execution-dependent GetNext() must not be used: it can return the branch
 * itself and change the publisher state when a solver is idle.
 */
//------------------------------------------------------------------------------
std::vector<GmatCommand*> SolverBranchCommand::GetCommandsInSolverScope()
{
   typedef std::pair<GmatCommand*, GmatCommand*> ScopeEntry;
   std::vector<ScopeEntry> pending;
   for (auto entry = branch.rbegin(); entry != branch.rend(); ++entry)
      pending.push_back(ScopeEntry(*entry, this));

   std::vector<GmatCommand*> commands;
   std::set<GmatCommand*> visited;
   visited.insert(this);
   while (!pending.empty())
   {
      const ScopeEntry entry = pending.back();
      pending.pop_back();
      GmatCommand *command = entry.first;
      if (command == entry.second)
         continue;
      if (!command)
         throw CommandException("Branch command \"" + generatingString +
                                "\" was not terminated!");
      if (!visited.insert(command).second)
         throw CommandException("Solver command \"" + generatingString +
                                "\" contains an invalid command cycle");

      commands.push_back(command);
      pending.push_back(ScopeEntry(command->GetNextWhileExecuting(), entry.second));
      if (dynamic_cast<SolverBranchCommand*>(command))
         continue;

      std::vector<GmatCommand*> children;
      for (Integer i = 0; GmatCommand *child = command->GetChildCommand(i); ++i)
         children.push_back(child);
      for (auto child = children.rbegin(); child != children.rend(); ++child)
         pending.push_back(ScopeEntry(*child, command));
   }
   return commands;
}


//------------------------------------------------------------------------------
// void RegisterSolverControlData(const StringArray&, VaryRegistrationReset)
//------------------------------------------------------------------------------
/**
 * Registers solver controls in structural order without executing ordinary
 * statements, choosing conditional arms, or iterating loops. Solver problem
 * dimensions are fixed before nominal/perturbation passes begin. Nested solver
 * controls belong to that solver and are never registered with this one.
 *
 * The native initialization sweep also runs direct nested solvers of the same
 * command type to completion. Preserve that dispatch (including GetNext's
 * completion/ExitMode behavior), but do not run a solver hidden in an ordinary
 * branch before its enclosing control flow is evaluated during execution.
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::RegisterSolverControlData(
      const StringArray &controlTypes, VaryRegistrationReset reset)
{
   // Validate the complete structural scope before registering any data.
   const std::vector<GmatCommand*> commands = GetCommandsInSolverScope();
   std::set<GmatCommand*> directCommands;
   for (GmatCommand *entry : branch)
      for (GmatCommand *command = entry; command != this;
           command = command->GetNextWhileExecuting())
         directCommands.insert(command);

   for (GmatCommand *command : commands)
   {
      const std::string &type = command->GetTypeName();
      if (std::find(controlTypes.begin(), controlTypes.end(), type) !=
            controlTypes.end())
      {
         const bool isVary = type == "Vary";
         if (isVary && reset == VARY_RESET_BEFORE)
            command->TakeAction("SolverReset");
         if (!command->Execute())
            throw CommandException("Failed to register solver control: " +
                  command->GetGeneratingString(Gmat::NO_COMMENTS));
         if (isVary && reset == VARY_RESET_AFTER)
            command->TakeAction("SolverReset");
      }
      else if (type == GetTypeName() && directCommands.count(command) &&
               dynamic_cast<SolverBranchCommand*>(command))
      {
         do
         {
            if (!command->Execute())
               throw CommandException("Failed to initialize nested solver: " +
                     command->GetGeneratingString(Gmat::NO_COMMENTS));
         } while (command->GetNext() == command);
      }
   }
}


//------------------------------------------------------------------------------
// void GetActiveSubscribers()
//------------------------------------------------------------------------------
/**
 * Builds a list of subscribers that are active for use in color changes and
 * pen up/down actions
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::GetActiveSubscribers()
{
   // Builds a table of Subscribers that are currently receiving data
   // XY and ground-track plots use the solver's iteration breakpoints.
   // OrbitPlot manages its own solver buffer through end-of-receive.

   activeSubscribers.clear();

   for (ObjectMap::iterator i = objectMap->begin(); i != objectMap->end(); ++i)
   {
      GmatBase *obj = i->second;
      if (obj->IsOfType(Gmat::SUBSCRIBER))
      {
         // Here's where we go XY specific
         if (obj->IsOfType("XYPlot"))
         {
            if (obj->GetBooleanParameter("Drawing"))
               activeSubscribers.push_back((Subscriber*)(obj));
         }
         else if (obj->IsOfType("GroundTrack") && obj->GetBooleanParameter("ShowPlot"))
            activeSubscribers.push_back((Subscriber*)(obj));
      }
   }

   for (ObjectMap::iterator i = globalObjectMap->begin();
         i != globalObjectMap->end(); ++i)
   {
      GmatBase *obj = i->second;
      if (obj->IsOfType(Gmat::SUBSCRIBER))
      {
         // Here's where we go XY specific
         if (obj->IsOfType("XYPlot"))
         {
            if (obj->GetBooleanParameter("Drawing"))
               activeSubscribers.push_back((Subscriber*)(obj));
         }
         else if (obj->IsOfType("GroundTrack") && obj->GetBooleanParameter("ShowPlot"))
            activeSubscribers.push_back((Subscriber*)(obj));
      }
   }
}


//------------------------------------------------------------------------------
// void PenUpSubscribers()
//------------------------------------------------------------------------------
/**
 * Sends a PenUp command to all active subscribers
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::PenUpSubscribers()
{
   for (UnsignedInt i = 0; i < activeSubscribers.size(); ++i)
      activeSubscribers[i]->TakeAction("PenUp");
}


//------------------------------------------------------------------------------
// void PenDownSubscribers()
//------------------------------------------------------------------------------
/**
 * Sends a PenDown command to all active subscribers
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::PenDownSubscribers()
{
   for (UnsignedInt i = 0; i < activeSubscribers.size(); ++i)
   {
      activeSubscribers[i]->TakeAction("PenDown");
   }
}


//------------------------------------------------------------------------------
// void DarkenSubscribers(Integer denominator = 1)
//------------------------------------------------------------------------------
/**
 * Darkens subscribers by a specified amount
 *
 * This method darkens by 1 / denominator
 *
 * @param denominator The darkening factor
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::DarkenSubscribers(Integer denominator)
{
//   MessageInterface::ShowMessage("Darkening by %d\n", denominator);

   std::stringstream factor;
   factor << denominator;
   for (UnsignedInt i = 0; i < activeSubscribers.size(); ++i)
      activeSubscribers[i]->TakeAction("Darken", factor.str().c_str());
}


//------------------------------------------------------------------------------
// void LightenSubscribers(Integer denominator = 1)
//------------------------------------------------------------------------------
/**
 * Lightens subscribers by a specified amount
 *
 * This method lightens by 1 / denominator
 *
 * @param denominator The lightening factor
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::LightenSubscribers(Integer denominator)
{
   std::stringstream factor;
   factor << denominator;
   for (UnsignedInt i = 0; i < activeSubscribers.size(); ++i)
      activeSubscribers[i]->TakeAction("Lighten", factor.str().c_str());
}


//------------------------------------------------------------------------------
// void SetSubscriberBreakpoint()
//------------------------------------------------------------------------------
/**
 * Marks a break point on a plot
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::SetSubscriberBreakpoint()
{
   std::ostringstream scope;
   scope << this;
   for (UnsignedInt i = 0; i < activeSubscribers.size(); ++i)
   {
      activeSubscribers[i]->TakeAction("SolverScope", scope.str());
      activeSubscribers[i]->TakeAction("MarkBreak");
   }
   // OrbitPlot owns its renderer's solver buffers and must not enter the XY
   // pen/color list. Its explicit integration sampler still needs the same
   // trajectory checkpoint as the numerical solver.
   for (auto *subscriber : publisher->GetSubscriberList())
      if (subscriber->IsOfType("OrbitPlot") &&
          subscriber->GetIntegerParameter(subscriber->GetParameterID("IntegrationStepsToSkip")) >= 0)
      {
         subscriber->TakeAction("SolverScope", scope.str());
         subscriber->TakeAction("MarkBreak");
      }
}

//------------------------------------------------------------------------------
// void ApplySubscriberBreakpoint(Integer bp = -1)
//------------------------------------------------------------------------------
/**
 * Breaks the curves on the subscribers, throwing away data beyond the break
 * point
 *
 * @param bp The index of the breakpoint
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::ApplySubscriberBreakpoint(Integer bp)
{
   std::stringstream breakpoint;
   breakpoint << bp;
   std::ostringstream scope;
   scope << this;
   for (UnsignedInt i = 0; i < activeSubscribers.size(); ++i)
   {
      activeSubscribers[i]->TakeAction("SolverScope", scope.str());
      activeSubscribers[i]->TakeAction("ClearFromBreak", breakpoint.str());
   }
   for (auto *subscriber : publisher->GetSubscriberList())
      if (subscriber->IsOfType("OrbitPlot") &&
          subscriber->GetIntegerParameter(subscriber->GetParameterID("IntegrationStepsToSkip")) >= 0)
      {
         subscriber->TakeAction("SolverScope", scope.str());
         subscriber->TakeAction("ClearFromBreak", breakpoint.str());
      }
}

// RunInitialGuess also has a nominal and a final publication pass. Its plot
// checkpoint must not enroll those plots in Solve-mode pen/color automation,
// which would override explicit plot commands in the initial-guess sequence.
void SolverBranchCommand::UpdateInitialGuessSubscriberBreakpoint(bool clear)
{
   std::vector<Subscriber*> previous;
   previous.swap(activeSubscribers);
   try
   {
      GetActiveSubscribers();
      if (clear) ApplySubscriberBreakpoint();
      else SetSubscriberBreakpoint();
   }
   catch (...)
   {
      activeSubscribers.swap(previous);
      throw;
   }
   activeSubscribers.swap(previous);
}


//------------------------------------------------------------------------------
// Integer GetCloneCount()
//------------------------------------------------------------------------------
/**
 * Retrieves the clone count for the members of the solver control sequence
 *
 * @return The count
 */
//------------------------------------------------------------------------------
Integer SolverBranchCommand::GetCloneCount()
{
   cloneCount = BranchCommand::GetCloneCount();
   if (theSolver != NULL)
      ++cloneCount;

   return cloneCount;
}


//------------------------------------------------------------------------------
// GmatBase* GetClone(Integer cloneIndex = 0)
//------------------------------------------------------------------------------
/**
 * Retrieves a pointer to a clone so its attributes can be accessed
 *
 * @param cloneIndex The index into the clone array
 *
 * @return The clone pointer, or NULL if no clone exists
 */
//------------------------------------------------------------------------------
GmatBase* SolverBranchCommand::GetClone(Integer cloneIndex)
{
   GmatBase *retPtr = NULL;

   if (cloneIndex == 0)
      retPtr = theSolver;
   else
      retPtr = BranchCommand::GetClone(cloneIndex - 1);

   return retPtr;
}


//------------------------------------------------------------------------------
// bool HasLocalClones()
//------------------------------------------------------------------------------
/**
 * Method used to check for the (possible) existence of local clones
 *
 * @return true if this object can have local clones of configured objects
 */
//------------------------------------------------------------------------------
bool SolverBranchCommand::HasLocalClones()
{
   return true;
}

//------------------------------------------------------------------------------
// void UpdateClonedObject(GmatBase *obj)
//------------------------------------------------------------------------------
/**
 * Passes updated parameter data in to an owned clone object
 *
 * @param obj The configured object that was cloned
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::UpdateClonedObject(GmatBase *obj)
{
   if (!obj || !theSolver || (obj != theSolver && !theSolver->IsCloneOf(*obj))) return;
   auto *source = dynamic_cast<Solver*>(obj);
   if (!source || typeid(*source) != typeid(*theSolver) || source->GetTypeName() != theSolver->GetTypeName())
      throw CommandException("Object type mismatch when updating a local solver clone.\n");
   if (source == theSolver) { isInitialized = false; return; }
   const auto previousContext = theSolver->GetFunctionValueContext();
   GmatBase::FunctionValueCopyGuard copying;
   isInitialized = false;
   theSolver->Copy(source);
}


//------------------------------------------------------------------------------
// void UpdateClonedObjectParameter(GmatBase *obj, Integer updatedParameterId)
//------------------------------------------------------------------------------
/**
 * This method changes a single parameter on an owned clone
 *
 * @param obj The master object holding the new parameter value
 * @param updatedParameterId The ID of the updated parameter
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::UpdateClonedObjectParameter(GmatBase *obj,
      Integer updatedParameterId)
{
   if (!obj || !theSolver || (obj != theSolver && !theSolver->IsCloneOf(*obj))) return;
   if (typeid(*obj) != typeid(*theSolver) || obj->GetTypeName() != theSolver->GetTypeName())
      throw CommandException("Object type mismatch when updating a local solver parameter.\n");
   if (obj != theSolver) theSolver->CopyParameter(*obj, updatedParameterId);
}


//------------------------------------------------------------------------------
// void PrepareToPublish(bool publishAll)
//------------------------------------------------------------------------------
/**
 * Prepares the command for data publishing
 *
 * @param publishAll Flag used to indicate if everything should be published
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::PrepareToPublish(bool publishAll)
{
   if (isAllowDataPublishing)
   {
      // It only register to publish when this command is allowed
      //StringArray owners, elements;
      pubdataOwners.clear();
      pubdataElements.clear();

      if (publishAll)
      {
         pubdataOwners.push_back("All");
         pubdataElements.push_back("All.epoch");
      }

      streamID = publisher->RegisterPublishedData(this, streamID, 
         pubdataOwners, pubdataElements);
   }
}


//------------------------------------------------------------------------------
// void PublishData()
//------------------------------------------------------------------------------
/**
 * Pushes provider data to the publisher
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::PublishData()
{
   publisher->Publish(this, streamID, NULL, 0);
}


//------------------------------------------------------------------------------
// void ChangeRunState(Gmat::RunState newState)
//------------------------------------------------------------------------------
/**
 * Passes run state into all members of the Solver Control Sequence & Publisher
 *
 * @param newState The new run state
 */
//------------------------------------------------------------------------------
void SolverBranchCommand::ChangeRunState(Gmat::RunState newState)
{
   currentRunState = newState;

   if (publisher != NULL)
      publisher->SetRunState(currentRunState);

   GmatCommand *current;
   std::vector<GmatCommand*>::iterator node;
   for (node = branch.begin(); node != branch.end(); ++node)
   {
      current = *node;

      if ((current != NULL) && (current != this))
         current->SetRunState(currentRunState);
   }
}


void SolverBranchCommand::AddListener( ISolverListener* listener )
{
   listeners.push_back( listener );
}

void SolverBranchCommand::RemoveListener( ISolverListener* listener )
{
   listeners.remove( listener );
}

void SolverBranchCommand::ClearListeners()
{
   listeners.clear();
}

void SolverBranchCommand::NotifyVariabledChanged(std::string name, Real value)
{
   for (std::list<ISolverListener *>::iterator it = listeners.begin(); it != listeners.end(); it++)
   {
      (*it)->VariabledChanged(name, value);
   }
}

void SolverBranchCommand::NotifyVariabledChanged(std::string name, std::string &value)
{
   for (std::list<ISolverListener *>::iterator it = listeners.begin(); it != listeners.end(); it++)
   {
      (*it)->VariabledChanged(name, value);
   }
}

void SolverBranchCommand::NotifyConstraintChanged(std::string name, Real desiredValue, Real value)
{
   for (std::list<ISolverListener *>::iterator it = listeners.begin(); it != listeners.end(); it++)
   {
      (*it)->ConstraintChanged(name, desiredValue, value);
   }
}


void SolverBranchCommand::RetireFunctionBindings(ObjectMap *store)
{
   if (!store || objectMap != store) return;
   ClearListeners();
   FreeLoopData();
   activeSubscribers.clear();
   queryStatusID = -1;
   specialState = Solver::INITIALIZING;
   // The solver definition remains with its native command owner. Native
   // initialization replaces that instance on the next invocation.
   BranchCommand::RetireFunctionBindings(store);
}

void SolverBranchCommand::PrepareSolverResources()
{
   if (solverResources) return;
   auto resources = std::make_shared<SolverCommandResources>();
   RetainCommandResourceOwner(resources);
   solverResources = std::move(resources);
}

Solver *SolverBranchCommand::PrepareLocalSolver(GmatBase *source)
{
   auto *solver = dynamic_cast<Solver*>(source);
   if (!solver) throw CommandException("A solver command requires a Solver resource.\n");
   PrepareSolverResources();
   std::set<GmatBase*> forbidden{source};
   for (auto *object : GetCommandOwnedObjects()) forbidden.insert(object);
   for (ObjectMap *map : {objectMap, globalObjectMap, configObjectMap})
      if (map) for (const auto &entry : *map) if (entry.second) forbidden.insert(entry.second);
   GmatBase::FunctionValueCopyGuard copying;
   std::unique_ptr<GmatBase> value(source->Clone());
   if (forbidden.count(value.get()))
   {
      value.release();
      throw CommandException("Solver Clone returned a source or existing owned object.\n");
   }
   auto *copy = dynamic_cast<Solver*>(value.get());
   if (!copy || typeid(*copy) != typeid(*source) || copy->GetTypeName() != source->GetTypeName())
      throw CommandException("Cannot clone the native solver type.\n");
   copy->SetCloneSource(source);
   copy->SetCloneOwner(this);
   for (const auto &owned : solverResources->solvers)
      if (typeid(*owned) == typeid(*copy) && owned->GetTypeName() == copy->GetTypeName())
      {
         const auto previousContext = owned->GetFunctionValueContext();
         owned->Copy(copy);
         owned->SetCloneSource(source);
         owned->SetCloneOwner(this);
         return owned.get();
      }
   std::unique_ptr<Solver> owned(static_cast<Solver*>(value.release()));
   Solver *result = owned.get();
   solverResources->solvers.push_back(std::move(owned));
   return result;
}

ObjectArray SolverBranchCommand::GetCommandOwnedObjects() const
{
   ObjectArray objects;
   if (solverResources)
   {
      for (const auto &solver : solverResources->solvers) objects.push_back(solver.get());
      for (const auto &snapshot : solverResources->snapshots) objects.push_back(snapshot.value.get());
   }
   return objects;
}
