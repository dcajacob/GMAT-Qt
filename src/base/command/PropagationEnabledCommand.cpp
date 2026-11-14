//$Id$
//------------------------------------------------------------------------------
//                       PropagationEnabledCommand
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
// Created: 2009/07/20
//
/**
 * Implementation of the PropagationEnabledCommand base class
 */
//------------------------------------------------------------------------------


#include "PropagationEnabledCommand.hpp"
#include "CoordinateSystem.hpp"
#include "MessageInterface.hpp"

#include "ODEModel.hpp"
#include "EphemerisFile.hpp"
#include "PropagationStateManager.hpp"
#include <set>

//#define DEBUG_INITIALIZATION
//#define DEBUG_EXECUTION
//#define DEBUG_STATE_BUFFERING
//#define DEBUG_PUBLISH_DATA
//#define DEBUG_EVENTLOCATORS
//#define DEBUG_EVENT_MODEL_FORCE
//#define DEBUG_TRANSIENT_FORCES
//#define DEBUG_PROP_STEPS
//#define DEBUG_MULIPLE_PROPAGATORS
//#define DEBUG_EPOCH_UPDATES_PEC

//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif

//------------------------------------------------------------------------------
// PropagationEnabledCommand(const std::string &typeStr)
//------------------------------------------------------------------------------
/**
 * Constructor
 *
 * @param typeStr The type string describing the subclass
 *
 * PropagationEnabledCommand is the abstract base class used for commands that
 * can propagate.  The class is made abstract through the destructor; all class
 * methods are implemented.
 */
//------------------------------------------------------------------------------
PropagationEnabledCommand::PropagationEnabledCommand(const std::string &typeStr)
:
   GmatCommand          (typeStr),
   direction            (1.0),
   overridePropInit     (false),
   hasFired             (false),
   inProgress           (false),
   skipEphemerisProp    (false),
   needToResetSTM       (false),
   dim                  (0),
   expandedDim          (0),
   epochID              (-1),
   transientForces      (NULL),
   transientCount       (0),
   //j2kState             (NULL),
   pubdata              (NULL),
   pubdataSize          (0),
   publishOnStep        (true)
{
   objectTypeNames.push_back("PropagationEnabledCommand");
   physicsBasedCommand = true;
}


//------------------------------------------------------------------------------
// ~PropagationEnabledCommand()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
PropagationEnabledCommand::~PropagationEnabledCommand()
{
   EmptyBuffer();

   if (!overridePropInit)
   {
      for (std::vector<PropSetup*>::iterator i = propagators.begin();
            i != propagators.end(); ++i)
      {
         if (*i)
            delete (*i);
      }
      propagators.clear();
   }

	for (std::vector<PropObjectArray*>::iterator i = propObjects.begin();
		i != propObjects.end(); ++i)
	{
		if (*i)
		   delete (*i);
	}
   propObjects.clear();

   if (pubdata)
      delete [] pubdata;
}


//------------------------------------------------------------------------------
// PropagationEnabledCommand(const PropagationEnabledCommand& pec)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 *
 * @param pec The PropagationEnabledCommand that is copied into a new instance
 */
//------------------------------------------------------------------------------
PropagationEnabledCommand::PropagationEnabledCommand(
      const PropagationEnabledCommand& pec) :
   GmatCommand          (pec),
   direction            (pec.direction),
   overridePropInit     (pec.overridePropInit),
   hasFired             (false),
   inProgress           (false),
   skipEphemerisProp    (pec.skipEphemerisProp),
   needToResetSTM       (pec.needToResetSTM),
   dim                  (pec.dim),
   expandedDim          (pec.expandedDim),
   epochID              (pec.epochID),
   transientForces      (NULL),
   transientCount       (0),
   //j2kState             (NULL),
   pubdata              (NULL),
   pubdataSize          (0),
   //publishOnStep        (true), 
   publishOnStep        (pec.publishOnStep)
{
   isInitialized = false;
   propagatorNames = pec.propagatorNames;
   for (UnsignedInt i = 0; i < pec.propObjectNames.size(); ++i)
      propObjectNames.push_back(pec.propObjectNames[i]);
   sats.clear();

   // We expected pec.expandedDim has the same value of pec.pubdataSize
   if (pec.expandedDim > 0)
   {
      // Increasing pubdata from size 0 to pec.expandedDim
      pubdataSize = pec.expandedDim;
      pubdata = new Real[pubdataSize];
      memcpy(pubdata, pec.pubdata, pubdataSize * sizeof(Real));
   }
}


//------------------------------------------------------------------------------
// PropagationEnabledCommand& operator=(const PropagationEnabledCommand& pec)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 *
 * @param pec The PropagationEnabledCommand that is copied into this instance
 *
 * @return A reference to this instance, configured to match pec but not yet
 *         initialized.
 */
//------------------------------------------------------------------------------
PropagationEnabledCommand& PropagationEnabledCommand::operator=(
      const PropagationEnabledCommand& pec)
{
   if (this != &pec)
   {
      // Preserve command configuration (including labels and source metadata)
      // as the copy constructor already does; runtime propagation stays cold.
      GmatCommand::operator=(pec);
      EmptyBuffer();
      overridePropInit    = pec.overridePropInit;
      direction           = pec.direction;
      hasFired            = false;
      inProgress          = false;
      skipEphemerisProp   = pec.skipEphemerisProp;
      needToResetSTM      = pec.needToResetSTM;
      dim                 = pec.dim;
      epochID             = pec.epochID;
      isInitialized       = false;

      //j2kState            = NULL;

      if (pubdata)
         delete[] pubdata;
      pubdata             = NULL;
      pubdataSize = 0; 

      // We expected pec.expandedDim has the same value of pec.pubdataSize
      if (pec.expandedDim > 0)
      {
         // Increasing pubdata from size 0 to pec.expandedDim
         pubdataSize = pec.expandedDim; 
         pubdata = new Real[pubdataSize];
         memcpy(pubdata, pec.pubdata, pubdataSize * sizeof(Real)); 
      }

		for (std::vector<PropSetup*>::const_iterator i = propagators.begin();
			i != propagators.end(); ++i)
		{
			if (*i)
			   delete (*i);
		}
      propagators.clear();
      sats.clear();

      propagatorNames = pec.propagatorNames;
      for (UnsignedInt i = 0; i < pec.propObjectNames.size(); ++i)
         propObjectNames.push_back(pec.propObjectNames[i]);

      transientForces = NULL;
      transientCount  = 0;
      //publishOnStep   = true;
      publishOnStep = pec.publishOnStep;
   }

   return *this;
}


//------------------------------------------------------------------------------
// void SetTransientForces(std::vector<PhysicalModel*> *tf)
//------------------------------------------------------------------------------
/**
 * Sets the array of transient forces, so it can be passed to the PropSetups.
 *
 * @param <tf> The array of transient forces.
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::SetTransientForces(std::vector<PhysicalModel*> *tf)
{
   GmatCommand::SetTransientForces(tf);
   if (transientForces != tf) transientBindings.Clear();
   transientForces = tf;
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Initialization method for the command
 *
 * This method clones all of the objects that need cloned for a propagation
 * enabled command and sets up the corresponding data structures.  The data
 * is not filled in this method; PrepareToPropagate() performs that final piece
 * of preparation.
 *
 * @return true on success, false on failure
 */
//------------------------------------------------------------------------------
bool PropagationEnabledCommand::Initialize()
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage
         ("PropagationEnabledCommand::Initialize() '%s' entered\n",
          GetGeneratingString(Gmat::NO_COMMENTS).c_str());
      MessageInterface::ShowMessage("  Size of propName is %d\n",
            propagatorNames.size());
      for (UnsignedInt ind = 0; ind < propagatorNames.size(); ++ind)
         MessageInterface::ShowMessage("     %d:  %s\n",
               ind, propagatorNames[ind].c_str());

      MessageInterface::ShowMessage("  Direction is %s\n",
            (direction == 1.0?"Forwards":"Backwards"));
   #endif
   
   bool retval = false;

   if (GmatCommand::Initialize())
   {
      inProgress = false;
      hasFired = false;
      sats.clear();

      for (std::vector<PropObjectArray*>::iterator o = propObjects.begin();
            o != propObjects.end(); ++o)
         delete (*o);

      propObjects.clear();
      std::string pName;

      //// Ensure that we are using fresh objects when buffering stops
      EmptyBuffer();

      // Remove old PropSetups
      if (!overridePropInit)
      {
         if (propagators.size() > 0)
         {
            for (std::vector<PropSetup*>::iterator ps = propagators.begin();
                  ps != propagators.end(); ++ps)
            {
               #ifdef DEBUG_MEMORY
               MemoryTracker::Instance()->Remove
                  ((*ps), (*ps)->GetName(), "PropagationEnabledCommand::"
                        "Initialize()", "deleting oldPs");
               #endif
               if (*ps)
                  delete (*ps);
            }

            propagators.clear();
            p.clear();
            fm.clear();
            maxSteps.clear();
         }

         // Todo Build the prop clones and set the related pointers
      }

      // Set the participant pointers
      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage("Found %d lists of prop object names\n",
               propObjectNames.size());
      #endif

      // Now set the pointers for the objects that get propagated
      for (UnsignedInt i = 0; i < propObjectNames.size(); ++i)
      {
         PropObjectArray *objects;
         ObjectArray els;

         #ifdef DEBUG_INITIALIZATION
            MessageInterface::ShowMessage("List %d contains %d prop objects\n",
                  i+1, propObjectNames[i].size());
         #endif

         if (propObjects.size() > i)
         {
            objects = propObjects[i];
            objects->clear();
         }
         else
         {
            // todo Memory debug and deallocation for this object
            objects = new PropObjectArray;
            propObjects.push_back(objects);
         }

         StringArray names = propObjectNames[i];
         PropSetup *currentPS = propagators[i];
         currentPS->SetPrecisionTimeFlag(true);

         Propagator *currentP = currentPS->GetPropagator();
         ODEModel *currentODE = currentPS->GetODEModel();
         PropagationStateManager *currentPSM = currentPS->GetPropStateManager();

         if ((!currentODE) && currentP->UsesODEModel())
            throw CommandException("ForceModel not set in Propagator " +
                  currentPS->GetName());


         //StringArray owners, elements;
         /// @todo Check to see if All and All.Epoch belong for all modes.
         //owners.push_back("All");
         //elements.push_back("All.epoch"); 

         bool finiteBurnActive = false;

         for (StringArray::iterator j = names.begin(); j != names.end(); ++j)
         {
            GmatBase *obj = FindObject(*j);
            if (obj == NULL)
               throw CommandException("Cannot find the object named " + (*j) +
                     " needed for propagation in the command\n" +
                     GetGeneratingString());
            
            obj->SetPrecisionTimeFlag(true);

            if (obj->IsOfType(Gmat::SPACEOBJECT))
            {
               // Reset STM to [I] matrix
               if (obj->IsOfType(Gmat::SPACECRAFT) && needToResetSTM)
                  ((Spacecraft*)obj)->SetSTMToIdentityMatrix();

               objects->push_back((SpaceObject*)obj);
               #ifdef DEBUG_INITIALIZATION
                  MessageInterface::ShowMessage("   Added the space object "
                        "named %s\n", obj->GetName().c_str());
               #endif

               // Now load up the PSM
               currentPSM->SetObject(obj);

               SpaceObject *so = (SpaceObject*)obj;
               if (epochID == -1)
                  epochID = so->GetParameterID("A1Epoch");
               if (so->IsManeuvering())
               {
                  #ifdef DEBUG_TRANSIENT_FORCES
                     MessageInterface::ShowMessage("PEC detected that %s is "
                           "maneuvering\n", so->GetName().c_str());
                  #endif
                  finiteBurnActive = true;
               }
               #ifdef DEBUG_TRANSIENT_FORCES
                  else
                     MessageInterface::ShowMessage("PEC detected that %s is "
                           "not maneuvering\n", so->GetName().c_str());
               #endif
               
               sats.push_back(so);
               AddToBuffer(so);
               
               //// Uncommented out SetNames() for GMT-5101 fix (LOJ: 2015.05.14)
               //if (so->GetType() == Gmat::FORMATION)
               //{
               //   // FillFormation(so, owners, elements);
               //}
               //else
               //{
               //   #ifdef DEBUG_INITIALIZATION
               //   MessageInterface::ShowMessage 
               //      ("   Setting data labels for '%s'\n", so->GetName().c_str());
               //   #endif
               //   SetNames(so->GetName(), owners, elements);
               //}
            }
            #ifdef DEBUG_INITIALIZATION
               else
                  MessageInterface::ShowMessage("   Found %s, not a space "
                        "object\n", obj->GetName().c_str());
            #endif
         }
         
         // Check for finite thrusts and update the force model if there are any
         if (finiteBurnActive == true)
         {
            if (currentODE != NULL)
               AddTransientForce(&(propObjectNames[i]), currentODE, currentPSM);
            else
               MessageInterface::ShowMessage("Spacecraft is performing a "
                     "finite maneuver but also propagating with an ephemeris "
                     "propagator; no independent maneuvering will be "
                     "performed.\n"); //, satName[index].c_str());
         }

         // Provide opportunity for derived cmds to set propagation properties
         SetPropagationProperties(currentPSM);
         
         if (currentPSM->BuildState() == false)
            throw CommandException("Could not build the state for the "
                  "command \n" + generatingString);
         if (currentPSM->MapObjectsToVector() == false)
            throw CommandException("Could not map state objects for the "
                  "command\n" + generatingString);
         

         if (currentODE != NULL)
         {
            currentODE->SetState(currentPSM->GetState());

            // Set solar system to ForceModel for Propagate inside a GmatFunction(loj: 2008.06.06)
            currentODE->SetSolarSystem(solarSys);
//            // Check for finite thrusts and update the force model if there are any
//            if (finiteBurnActive == true)
//               AddTransientForce(satName[index], currentODE);
            currentP->SetPhysicalModel(currentODE);
         }
         else
         {
            ObjectArray pObjects;

            currentPSM->GetStateObjects(pObjects, Gmat::SPACEOBJECT);
            for (UnsignedInt i = 0; i < pObjects.size(); ++i)
               currentP->SetRefObject(pObjects[i], Gmat::SPACEOBJECT,
                     pObjects[i]->GetName());
            currentP->SetSolarSystem(solarSys);
         }
         


         //streamID = publisher->RegisterPublishedData(this, streamID, owners, elements);
         currentP->Initialize();

         // Set spacecraft parameters for forces that need them
         if (currentODE != NULL)
            if (currentODE->SetupSpacecraftData((ObjectArray*)objects, 0) <= 0)
               throw PropagatorException("Propagate::Initialize -- "
                     "ODE model cannot set spacecraft parameters");

      }
      
      //StringArray owners, elements;
      //expandedDim = PrepareElementInfoToPublish(owners, elements); 
      //streamID = publisher->RegisterPublishedData(this, streamID, owners, elements);
      
      // Now we have everything we need to init the prop subsystem
      retval = true;
      isInitialized = true;
      // retval = AssemblePropagators();

      #ifdef DEBUG_INITIALIZATION
         if (retval == true)
            MessageInterface::ShowMessage("PEC Initialize() succeeded\n");
         else
            MessageInterface::ShowMessage(
                  "PEC Initialize() failed to initialize the PropSetups\n");
      #endif
   }
   return retval;
}


//------------------------------------------------------------------------------
// bool HasLocalClones()
//------------------------------------------------------------------------------
/**
 * Method to check for the presence of local clones
 *
 * @return true if there are local clones, false if not
 */
//------------------------------------------------------------------------------
bool PropagationEnabledCommand::HasLocalClones()
{
   return true;
}


//------------------------------------------------------------------------------
// void UpdateClonedObject(GmatBase *obj)
//------------------------------------------------------------------------------
/**
 * Updates cloned objects that are copies of the object passed in
 *
 * @param obj The object with setting updates
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::UpdateClonedObject(GmatBase *obj)
{
   if (obj->IsOfType(Gmat::PROP_SETUP))
   {
      for (UnsignedInt i = 0; i < propagators.size(); ++i)
         if (obj->GetName() == propagators[i]->GetName())
         {
            propagators[i]->operator=(*((PropSetup*)obj));
            // Reset the fired flag so everything gets reconstructed
            hasFired = false;
         }
   }

   if (obj->IsOfType(Gmat::ODE_MODEL))
   {
      for (UnsignedInt j = 0; j < fm.size(); ++j)
         if (obj->GetName() == fm[j]->GetName())
         {
            fm[j]->operator=(*((ODEModel*)obj));
            #ifdef DEBUG_CLONES
               MessageInterface::ShowMessage("--------%p\n%s\nshould match %p"
                     "\n%s\n--------\n", fm[j],
                     fm[j]->GetGeneratingString(Gmat::NO_COMMENTS).c_str(),
                     obj, obj->GetGeneratingString(Gmat::NO_COMMENTS).c_str());
            #endif
         }

      for (UnsignedInt i = 0; i < propagators.size(); ++i)
      {
         ODEModel *odem = propagators[i]->GetODEModel();
         if (obj->GetName() == odem->GetName())
         {
            odem->operator=(*((ODEModel*)obj));

            #ifdef DEBUG_CLONES
               MessageInterface::ShowMessage("--------[%d] %p\n%s\nshould "
                     "match %p\n%s\n--------\n", i, odem,
                     odem->GetGeneratingString(Gmat::NO_COMMENTS).c_str(),
                     obj, obj->GetGeneratingString(Gmat::NO_COMMENTS).c_str());
            #endif
         }
      }
   }
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
void PropagationEnabledCommand::UpdateClonedObjectParameter(GmatBase *obj,
      Integer updatedParameterId)
{
   GmatBase *theClone = NULL;
   if (obj->IsOfType(Gmat::PROP_SETUP))
      for (UnsignedInt i = 0; i < propagators.size(); ++i)
         if (obj->GetName() == propagators[i]->GetName())
            theClone = propagators[i];

   if (obj->IsOfType(Gmat::ODE_MODEL))
   {
      for (UnsignedInt j = 0; j < fm.size(); ++j)
         if (obj->GetName() == fm[j]->GetName())
            theClone = fm[j];

      for (UnsignedInt i = 0; i < propagators.size(); ++i)
         if (obj->GetName() == propagators[i]->GetName())
            theClone = propagators[i];
   }

   if (theClone != NULL)
      theClone->CopyParameter(*obj, updatedParameterId);
}


//------------------------------------------------------------------------------
// protected methods
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// void SetPropagationProperties(PropagationStateManager *psm)
//------------------------------------------------------------------------------
/**
 * This method provides an interface derived commands can use to set specific
 * propagation properties where needed.  As an example the orbit STM can be set
 * using in an overridden implementation of this method for estimators that need
 * to propagate the orbit STM.
 *
 * @param psm A PropagationStateManager that controls the propagation state
 *            vector
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::SetPropagationProperties(
      PropagationStateManager *psm)
{
}

//------------------------------------------------------------------------------
// bool PrepareToPropagate(bool preparePublisher)
//------------------------------------------------------------------------------
/**
 * Fills in the data structures needed to start propagation
 *
 * This method completes the setup of the ODEModel, prepares the publisher data
 * array, fills in the data needed to propagate, and completes the steps needed
 * to propagate the objects selected for propagation by the user.
 *
 * @param preparePublisher True if this function should prepare the publisher,
 *                         false if it should not.
 *
 * @return true on success, false on failure
 *
 * todo The current implementation does not yet incorporate transient forces
 */
//------------------------------------------------------------------------------
bool PropagationEnabledCommand::PrepareToPropagate(bool preparePublisher)
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage(
            "PropagationEnabledCommand::PrepareToPropagate() entered\n");
   #endif
   bool retval = false;

   // Detect reentry after a value/Formation assignment before using cached
   // PSM/force object pointers. Keep old graph owners alive through Initialize.
   const auto previousRoots = bufferRoots;
   if (BufferBindingsChanged()) hasFired = false;

   dim = 0;

   if (hasFired == true)
   {
      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage(
               "   PrepareToPropagate() in hasFired state\n");
      #endif

      // Handle the transient forces
      for (UnsignedInt i = 0; i < p.size(); ++i)
         if (fm[i] != NULL)
            AddTransientForce(&(propObjectNames[i]), fm[i], psm[i]);


//      for (std::vector<PropObjectArray*>::iterator poa = propObjects.begin();
//           poa != propObjects.end(); ++poa)
//      {
//         for (PropObjectArray::iterator sc = (*poa)->begin();
//               sc != (*poa)->end(); ++sc)
//         {
//            if (((SpaceObject*)(*sc))->IsManeuvering())
//            {
//               #ifdef DEBUG_FINITE_MANEUVER
//                  MessageInterface::ShowMessage(
//                     "SpaceObject %s is maneuvering\n", (*sc)->GetName().c_str());
//               #endif
//
// todo: Transient forces here
//               // Add the force
//               for (UnsignedInt index = 0; index < propagators.size(); ++index)
//               {
//                  for (std::vector<PhysicalModel*>::iterator i = transientForces->begin();
//                       i != transientForces->end(); ++i)
//                  {
//                     #ifdef DEBUG_TRANSIENT_FORCES
//                     MessageInterface::ShowMessage
//                        ("Propagate::PrepareToPropagate() Adding transientForce<%p>'%s'\n",
//                         *i, (*i)->GetName().c_str());
//                     #endif
//                     prop[index]->GetODEModel()->AddForce(*i);
//
//                     // todo: Rebuild ODEModel by calling BuildModelFromMap()
//                  }
//               }
//            }
//         }
//      }

      for (Integer n = 0; n < (Integer)propagators.size(); ++n)
      {
         elapsedTime[n] = 0.0;
         currEpoch[n]   = 0.0;
         currEpochGT[n] = 0.0;
         fm[n]->SetTime(0.0);
         fm[n]->SetPropStateManager(propagators[n]->GetPropStateManager());
         fm[n]->UpdateInitialData();
         dim += fm[n]->GetDimension();

         p[n]->Initialize();
         p[n]->Update(true /*direction > 0.0*/);
         //state = fm[n]->GetState();
         //j2kState = fm[n]->GetJ2KState();
      }

      baseEpoch.clear();
      baseEpochGT.clear();

      for (Integer n = 0; n < (Integer)propagators.size(); ++n)
      {
         if (propObjectNames[n].empty())
            throw CommandException(
               "Propagator has no associated space objects.");

         GmatBase* sat1 = FindObject(*(propObjectNames[n].begin()));
         baseEpoch.push_back(sat1->GetRealParameter(epochID));
         baseEpochGT.push_back(sat1->GetGmatTimeParameter(epochID));

         elapsedTime[n] = fm[n]->GetTime();
         currEpoch[n] = baseEpoch[n] + elapsedTime[n] /
            GmatTimeConstants::SECS_PER_DAY;
         currEpochGT[n] = baseEpochGT[n]; currEpochGT[n].AddSeconds(elapsedTime[n]);
      }

      inProgress = true;
   }
   else
   {
      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage(
               "PropagationEnabledCommand::PrepareToPropagate() first entry\n");
      #endif

      // Reinitialize the subsystem to pick up transients and other updates
      Initialize();

      // Loop through the PropSetups and build the models
      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage("   Looping through %d propagators\n",
               propagators.size());
      #endif
      for (std::vector<PropSetup*>::iterator i=propagators.begin();
            i != propagators.end(); ++i)
      {
         #ifdef DEBUG_INITIALIZATION
            MessageInterface::ShowMessage(
                  "   Setting PSM on ODEModel for propagator %s\n",
                  (*i)->GetName().c_str());
         #endif
         ODEModel *ode = (*i)->GetODEModel();
         if (ode != NULL)    // Only do this for the PropSetups that integrate
         {
            // Build the ODE model
            ode->SetPropStateManager((*i)->GetPropStateManager());
            if (ode->BuildModelFromMap() == false)
               throw CommandException(typeName + "::PrepareToPropagate(): "
                     "Unable to assemble the ODE model for " + (*i)->GetName());
         }
         else
         {
            (*i)->GetPropagator()->SetPropStateManager(
                  (*i)->GetPropStateManager());
         }
      }

      p.clear();
      fm.clear();
      psm.clear();
      maxSteps.clear();
      baseEpoch.clear();
	   currEpoch.clear();				// fix bug #1 of ticket GMT-4314
	   elapsedTime.clear();          // fix bug #1 of ticket GMT-4314
      baseEpochGT.clear();
      currEpochGT.clear();

      for (UnsignedInt n = 0; n < propagators.size(); ++n)
      {
         #ifdef DEBUG_INITIALIZATION
            MessageInterface::ShowMessage(
                  "   Setting pointers for propagator %s\n",
                  propagators[n]->GetName().c_str());
         #endif
         elapsedTime.push_back(0.0);

         Propagator *gator = propagators[n]->GetPropagator();
         ODEModel *fModel = propagators[n]->GetODEModel();
         p.push_back(gator);

         if (gator->IsOfType("Integrator"))
         {
            // If no error control, use the init step, otherwise maxstep
            if (fModel->GetStringParameter("ErrorControl") == "None")
               maxSteps.push_back(gator->GetRealParameter("InitialStepSize"));
            else
               maxSteps.push_back(gator->GetRealParameter("MaxStep"));
         }
         else
            maxSteps.push_back(gator->GetRealParameter("InitialStepSize"));

         fm.push_back(fModel);
         psm.push_back(propagators[n]->GetPropStateManager());

         currEpoch.push_back(psm[n]->GetState()->GetEpoch());
         currEpochGT.push_back(psm[n]->GetState()->GetEpochGT());

         #ifdef DEBUG_INITIALIZATION
            MessageInterface::ShowMessage(
                  "   Initializing propagator %s\n",
                  propagators[n]->GetName().c_str());
         #endif

         p[n]->Initialize();
         psm[n]->MapObjectsToVector();

         p[n]->Update(true/*direction > 0.0*/);
         
         if (fm[n])
         {
            //j2kState = fm[n]->GetJ2KState();
            dim += fm[n]->GetDimension();
         }
         else
         {
            //j2kState = p[n]->GetJ2KState();
            dim += p[n]->GetDimension();
         }
         baseEpoch.push_back(psm[n]->GetState()->GetEpoch());
         baseEpochGT.push_back(psm[n]->GetState()->GetEpochGT());

         hasFired = true;
         inProgress = true;
      }
   }

   /// This code is used to prevent all PredictorCorrector from using in RunSimulator command and RunEstimator command.
   /// It will be removed when ticket GMT-7586 is completed fixing bug in PredictoreCorrector.cpp code.
   if (this->IsOfType("RunSimulator") || this->IsOfType("RunEstimator"))
   {
      for (Integer idx = 0; idx < propagators.size(); ++idx)
      {
         Propagator *prop = propagators[idx]->GetPropagator();
         if ((prop != NULL) && (prop->IsOfType("PredictorCorrector")))
         {
            //throw GmatBaseException("Error: Integrator with type '" + 
            //   propagators[idx]->GetStringParameter("Type") + "' is not allowed to use for running " + 
            //   (this->IsOfType("RunSimulator") ? "simulation" : "estimation"));
            if (this->IsOfType("RunSimulator"))
               throw GmatBaseException("Error: The simulator does not currently permit use of an '" 
                  + prop->GetTypeName() + "' integrator."); 
            else if (this->IsOfType("RunEstimator"))
               throw GmatBaseException("Error: The estimator does not currently permit use of an '" 
                  + prop->GetTypeName() + "' integrator.");

         }
      }
   }


   if (preparePublisher)
   {
      if (pubdata)
      {
         #ifdef DEBUG_MEMORY
            MemoryTracker::Instance()->Remove
               (pubdata, "pubdata", "Propagate::PrepareToPropagate()",
                "deleting pub data");
         #endif
         delete [] pubdata;
         pubdata = NULL;
         pubdataSize = 0;
      }
            
      //pubdata = new Real[dim+1];

      // where does the code set value to expandedDim?
      pubdataSize = expandedDim;
      pubdata = new Real[pubdataSize];


      #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Add
            (pubdata, "pubdata", "Propagate::PrepareToPropagate()",
             //"pubdata = new Real[dim+1]");
             "pubdata = new Real[pubdataSize]"); 
      #endif

      // Publish the data
      for (Integer idx = 0; idx < pubdataSize; ++idx)
         pubdata[idx] = GmatMathConstants::QUIET_NAN;

      if (hasPrecisionTime)
         pubdata[0] = currEpochGT[0].GetMjd();
      else
         pubdata[0] = currEpoch[0];

      // Prepare data to buffer for publishing
      // memcpy(&pubdata[1], j2kState, dim*sizeof(Real));
      //Integer len = 1;
      PrepareElementDataToPublish(pubdata, pubdataSize, pubdataElements, covStartIndex, attStartIndex);

      #ifdef DEBUG_PUBLISH_DATA
         MessageInterface::ShowMessage
            ("PropagationEnabledCommand::PrepareToPropagate() '%s' publishing initial %d data to "
             "stream %d, 1st data = %f\n", GetGeneratingString(Gmat::NO_COMMENTS).c_str(),
             dim+1, streamID, pubdata[0]);
      #endif

      // Publish
      //publisher->Publish(this, streamID, pubdata, dim+1, direction);
      PublishElementData(direction);
   }

   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage(
            "PropagationEnabledCommand::PrepareToPropagate() finished\n");
   #endif

   transientBindings.Capture(transientForces);
   return retval;
}


//------------------------------------------------------------------------------
// bool PropagationEnabledCommand::AssemblePropagators()
//------------------------------------------------------------------------------
/**
 * Prepares all of the propagators for propagation by setting the object
 * pointers to the objects that need to be propagated and then initializing the
 * PropSetups
 *
 * @return true on success, false if a PropSetup failed to initialize
 */
//------------------------------------------------------------------------------
bool PropagationEnabledCommand::AssemblePropagators()
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage(
            "PropagationEnabledCommand::AssemblePropagators() entered\n");
   #endif
   bool retval = true;

   PropObjectArray *currentObjects;
   PropagationStateManager *psm;
   for (UnsignedInt i = 0; i < propagators.size(); ++i)
   {
      // For each PropSetup, set the SpaceObjects
      currentObjects = propObjects[i];
      psm = propagators[i]->GetPropStateManager();
      for (UnsignedInt j = 0; j < currentObjects->size(); ++j)
      {
         #ifdef DEBUG_INITIALIZATION
            MessageInterface::ShowMessage(
                  "Adding SpaceObject %s to PropSetup %s\n",
                  (*currentObjects)[j]->GetName().c_str(),
                  propagators[i]->GetName().c_str());
         #endif

         psm->SetObject((SpaceObject*)((*currentObjects)[j]));
      }

      // Now initialize the current PropSetup
      if (propagators[i]->Initialize() == false)
         return false;
   }

   return retval;
}

//------------------------------------------------------------------------------
// bool Step(Real dt)
//------------------------------------------------------------------------------
/**
 * Steps each PropSetup by a specified amount
 *
 * todo: This is the method that needs to be adapted to handle large input time
 * steps
 *
 * @param dt The desired time step
 *
 * @return true on success, false on failure
 */
//------------------------------------------------------------------------------
bool PropagationEnabledCommand::Step(Real dt)
{
   bool retval = true;

   for (UnsignedInt i = 0; i < fm.size(); ++i)
   {
      if (fm[i])
      {
         fm[i]->UpdateInitialData(false, false);
         fm[i]->BufferState();
      }
      else
      {
         p[i]->BufferState();
      }
   }
   
   std::vector<Propagator*>::iterator current = p.begin();

   // Step all of the propagators by the input amount
   #ifdef DEBUG_MULIPLE_PROPAGATORS
      MessageInterface::ShowMessage("PropagationEnabledCommand::Step(%lf)  ", dt);
   #endif

   while (current != p.end())
   {
      #ifdef DEBUG_MULIPLE_PROPAGATORS
         MessageInterface::ShowMessage("%s by %lf  ", (*current)->GetName().c_str(), dt);
      #endif
      
      try
      {
         if (!(*current)->Step(dt))
            // Check if Failure is due to Finite Burn Segmenting
         {
            if ((*current)->GetHasFiniteBurn())
            {
               Real fullStep = (*current)->GetStepSize();
               Real totalStepTaken = (*current)->GetStepTaken();
               if ((!(*current)->IsBurning()))
               {
                  if (CreateNewSegments(currEpochGT[0].GetMjd()))
                  {
                     (*current)->SetStepSize(fullStep);
                     (*current)->SetFollowUpStep(false);
                  }
               }
               else if ((*current)->GetFollowUpStep() == true)
               {
                  
                  if ((*current)->PropagatesForward())
                  {
                     while (fullStep > totalStepTaken)
                     {
                        (*current)->SetStepSize(fullStep - totalStepTaken);
                        if (!(*current)->Step())
                        {
                           if (!(*current)->IsBurning())
                           {
                              CreateNewSegments(currEpochGT[0].GetMjd());
                           }
                        }
                        totalStepTaken += (*current)->GetStepTaken();
                     }
                  }
                  else
                  {
                     while (fullStep < totalStepTaken)
                     {
                        (*current)->SetStepSize(fullStep - totalStepTaken);
                        if (!(*current)->Step())
                        {
                           if ((*current)->IsBurning())
                              CreateNewSegments(currEpochGT[0].GetMjd());
                        }
                        totalStepTaken += (*current)->GetStepTaken();
                     }

                  }

                  (*current)->SetStepSize(totalStepTaken);
                  (*current)->SetFollowUpStep(false);
               }
            }
            else if ((*current)->GetIntegerParameter("MaxStepAttempts")*(*current)->GetStepSize() < dt)
            {
               char sizeBig[32];
               std::sprintf(sizeBig, "%.2lf", dt);
               char sizeSmall[32];
               std::sprintf(sizeSmall, "%.2lf", (*current)->GetStepSize());
               char stepsNeeded[32];
               std::sprintf(stepsNeeded, "%.1f", dt / (*current)->GetStepSize());
               throw CommandException("In PropagationEnabledCommand::Step(), "
                  "Propagator " + (*current)->GetName() + "With step size of " + sizeSmall +
                  " seconds has insufficient a MaxStepAttempts to take the needed step of " + sizeBig +
                  " seconds. MaxStepAttempts must be at least " + stepsNeeded + " (rounding up as needed). \n");
            }
            else
            {
               char size[32];
               std::sprintf(size, "%.12lf", dt);
               throw CommandException("In PropagationEnabledCommand::Step(), "
                  "Propagator " + (*current)->GetName() +
                  " failed to take a good final step (size = " + size + ")\n");
            }
         }
      }
      catch (BaseException &)
      {
         if (!skipEphemerisProp)
            throw;
         else
            if (((*current)->IsOfType("Integrator")))
               throw;
      }
      
      #ifdef DEBUG_PROP_STEPS
         MessageInterface::ShowMessage("%.12lf ", dt);
      #endif
      ++current;
   }
   #ifdef DEBUG_MULIPLE_PROPAGATORS
      MessageInterface::ShowMessage("\n");
   #endif
   
   Real timeToAdd;
   Real timeSoFar;
   GmatTime timeSoFarGT;

   for (UnsignedInt i = 0; i < fm.size(); ++i)
   {
      // orbit related parameters use spacecraft for data
      if (fm[i])
         timeToAdd = fm[i]->GetTime();
      else
         timeToAdd = p[i]->GetTime();
      if (elapsedTime.size() > 0)
         elapsedTime[i] = timeToAdd;

      timeSoFar = baseEpoch[i] + timeToAdd / GmatTimeConstants::SECS_PER_DAY;
      timeSoFarGT = baseEpochGT[i];
      timeSoFarGT.AddSeconds(timeToAdd);
      if (currEpoch.size() > 0)
      {
         currEpoch[i] = timeSoFar;
         currEpochGT[i] = timeSoFarGT;
      }
      

      // Update spacecraft epoch, without argument the spacecraft epoch
      // won't get updated for consecutive Propagate command
      try
      {
         if (fm[i])
         {
            if (fm[i]->HasPrecisionTime())
               fm[i]->UpdateSpaceObjectGT(timeSoFarGT);
            else
               fm[i]->UpdateSpaceObject(timeSoFar);
#ifdef DEBUG_EPOCH_UPDATES_PEC
            for (UnsignedInt i = 0; i < fm.size(); ++i)
               if (fm[i])
                  fm[i]->ReportEpochData();
#endif
         }
         else
         {
            if (p[i]->HasPrecisionTime())
               p[i]->UpdateSpaceObjectGT(timeSoFarGT);
            else
               p[i]->UpdateSpaceObject(timeSoFar);
         }
      }
      catch (BaseException &)
      {
         if (!skipEphemerisProp)
            throw;
         else
         {
            if (!fm[i])
            {
               if (p[i]->IsOfType("EphemerisPropagator"))
               {
                  //Do nothing
               }
               else
                  throw;
            }
            else
               throw;
         }
      }

      #ifdef DEBUG_PROP_STEPS
         MessageInterface::ShowMessage("  ---> elapsed time = %.12lf, epoch "
               "%.12lf\n", elapsedTime[i], currEpoch[i]);
         MessageInterface::ShowMessage("  ---> elapsed time = %.12lf, epochGT "
            "%s\n", elapsedTime[i], currEpochGT[i].ToString().c_str());
      #endif
   }
   
   #ifdef DEBUG_PROP_STEPS
      MessageInterface::ShowMessage("\n");
   #endif

   if (isAllowDataPublishing)
   {
      // only publishing on step when command is allowed data publishing.
      // Note thate: RunSimulator command is not allowed publishing data.
      // Therefore, this step is omitted.
      if (publishOnStep)
      {
         // Publish the data here
         for (Integer idx = 0; idx < pubdataSize; ++idx)
            pubdata[idx] = GmatMathConstants::QUIET_NAN;

         if (hasPrecisionTime)
            pubdata[0] = timeSoFarGT.GetMjd();
         else
            pubdata[0] = timeSoFar;

         // Prepare data to buffer for publishing
         //memcpy(&pubdata[1], j2kState, dim*sizeof(Real));
         //Integer len = 1;
         PrepareElementDataToPublish(pubdata, pubdataSize, pubdataElements, covStartIndex, attStartIndex);

         #ifdef DEBUG_PUBLISH_DATA
         MessageInterface::ShowMessage
         ("PropagationEnabledCommand::Step() '%s' publishing %d data to stream %d, 1st data = "
            "%f\n", GetGeneratingString(Gmat::NO_COMMENTS).c_str(),
            dim + 1, streamID, pubdata[0]);
         #endif

         // Publish
         //publisher->Publish(this, streamID, pubdata, dim+1, direction);
         PublishElementData(direction);
         if (createNewSegmentsNow == true)
         {
            if (HasPrecisionTime())
               CreateNewSegments(currEpochGT[0].GetMjd());
            else
               CreateNewSegments(currEpoch[0]);
         }
      }
   }

   return retval;
}


//------------------------------------------------------------------------------
// bool TakeAStep(Real propStep)
//------------------------------------------------------------------------------
/**
 * Advances state but the input interval
 *
 * @param propStep Interval to step, in seconds.
 *
 * @return true on success, false on failure
 *
 * @note This default version always throws.  The method must be overridden in
 *       a derived class before using it.
 */
//------------------------------------------------------------------------------
bool PropagationEnabledCommand::TakeAStep(Real propStep)
{
   throw CommandException("TakeAStep must be overridden to use it; no override "
         "exists for " + typeName + " commands.");
}

//------------------------------------------------------------------------------
// void AddToBuffer(SpaceObject *so)
//------------------------------------------------------------------------------
/**
 * Adds satellites and formations to the state buffer.
 *
 * @param <so> The SpaceObject that is added.
 */
//------------------------------------------------------------------------------
std::vector<Publisher::CartesianColumns> PropagationEnabledCommand::GetPublicationColumns()
{
   if (propagators.size() != psm.size() || pubdataElements.empty() || expandedDim <= 0 ||
       pubdataElements.size() != static_cast<size_t>(expandedDim))
      throw CommandException("Propagation publication groups or packet extent are inconsistent.");
   std::vector<Publisher::CartesianColumns> columns;
   size_t offset = 1;
   const char *components[] = {"X", "Y", "Z", "Vx", "Vy", "Vz"};
   auto check = [&](size_t start, const StringArray &elements)
   {
      if (start > pubdataElements.size() || elements.size() > pubdataElements.size() - start ||
          !std::equal(elements.begin(), elements.end(), pubdataElements.begin() + start))
         throw CommandException("Propagation publication column registration is stale.");
   };
   for (size_t group = 0; group < psm.size(); ++group)
   {
      auto *manager = psm[group];
      const auto *map = manager->GetStateMap();
      StringArray owners, elements;
      manager->PrepareStateInfoToPublish(owners, elements);
      if (!map || map->size() != static_cast<size_t>(manager->GetState()->GetSize()) ||
          owners.size() != map->size() || elements.size() != map->size())
         throw CommandException("Propagation publication differs from the native state map.");
      check(offset, elements);
      // Distinguish the definition from per-command runtime clones. Separate
      // commands using the same declared PropSetup do not imply a new provider.
      auto *definition = FindObject(propagators[group]->GetName());
      if (!definition || !definition->IsOfType(Gmat::PROP_SETUP)) definition = propagators[group];
      const auto providerIdentity = definition->GetLifetimeToken();
      for (size_t i = 0; i < map->size(); ++i)
      {
         const auto *item = (*map)[i];
         if (!item) throw CommandException("Published state map has a NULL item.");
         if (item->elementName != "CartesianState" || item->subelement < 1 ||
             (item->subelement - 1) % 6 != 0) continue;
         if (!item->object || map->size() - i < 6)
            throw CommandException("Published Cartesian state is incomplete.");
         GmatBase *object = item->object;
         if (object->IsOfType(Gmat::FORMATION))
         {
            const ObjectArray members = object->GetRefObjectArray(Gmat::SPACECRAFT);
            const StringArray roles = object->GetStringArrayParameter("Add");
            const size_t member = static_cast<size_t>((item->subelement - 1) / 6);
            if (members.size() != roles.size() || member >= roles.size() || roles[member] != owners[i])
               throw CommandException("Published Formation member layout is inconsistent.");
            object = members[member];
         }
         if (!object || !object->IsOfType(Gmat::SPACECRAFT))
            throw CommandException("Published Cartesian object is not a spacecraft.");
         Publisher::CartesianColumns column;
         column.object = object;
         column.propagationGroup = static_cast<Integer>(group);
         column.propagatorName = definition->GetName();
         column.propagatorIdentity = providerIdentity;
         column.hasEpoch = true;
         auto *state = manager->GetState();
         column.epoch = state->HasPrecisionTime() ? state->GetEpochGT() : GmatTime(state->GetEpoch());
         column.frame = static_cast<Spacecraft*>(object)->GetInternalCoordSystem();
         if (!column.frame) throw CommandException("Published spacecraft has no internal coordinate system.");
         column.frameIdentity = column.frame->GetLifetimeToken();
         for (size_t j = 0; j < 6; ++j)
         {
            const auto *component = (*map)[i+j];
            if (!component || component->object != item->object || component->elementName != "CartesianState" ||
                component->subelement != item->subelement + static_cast<Integer>(j) ||
                owners[i+j] != owners[i] || elements[i+j] != owners[i] + "." + components[j])
               throw CommandException("Published Cartesian component layout is inconsistent.");
            column.indices[j] = static_cast<Integer>(offset+i+j);
         }
         columns.push_back(column);
         i += 5;
      }
      offset += map->size();
   }
   if (offset != static_cast<size_t>(covStartIndex))
      throw CommandException("Published Cartesian/state blocks differ from the covariance start.");
   for (size_t group = 0; group < psm.size(); ++group)
   {
      const auto spacecraft = psm[group]->GetPublishedSpacecraft();
      StringArray owners, elements;
      psm[group]->PrepareCovAndAccelerationInfoToPublish(owners, elements);
      if (elements.size() != spacecraft.size() * 24)
         throw CommandException("Published covariance/acceleration group has an invalid shape.");
      check(offset, elements);
      for (size_t i = 0; i < spacecraft.size(); ++i)
         for (auto &column : columns)
            if (column.object == spacecraft[i].object && column.propagationGroup == static_cast<Integer>(group))
            {
               for (size_t j = 0; j < 21; ++j) column.covariance[j] = static_cast<Integer>(offset + 21*i+j);
               for (size_t j = 0; j < 3; ++j) column.acceleration[j] = static_cast<Integer>(offset + 21*spacecraft.size() + 3*i+j);
            }
      offset += elements.size();
   }
   if (offset != static_cast<size_t>(attStartIndex))
      throw CommandException("Published covariance groups differ from the attitude start.");
   for (size_t group = 0; group < psm.size(); ++group)
   {
      const auto spacecraft = psm[group]->GetPublishedSpacecraft();
      StringArray owners, elements;
      psm[group]->PrepareAttitudeInfoToPublish(owners, elements);
      if (elements.size() != spacecraft.size() * 4)
         throw CommandException("Published attitude group has an invalid shape.");
      check(offset, elements);
      for (size_t i = 0; i < spacecraft.size(); ++i)
         for (auto &column : columns)
            if (column.object == spacecraft[i].object && column.propagationGroup == static_cast<Integer>(group))
               for (size_t j = 0; j < 4; ++j) column.quaternion[j] = static_cast<Integer>(offset + 4*i+j);
      offset += elements.size();
   }
   if (offset != pubdataElements.size())
      throw CommandException("Published spacecraft groups do not cover the registered packet.");
   return columns;
}

bool PropagationEnabledCommand::PublishElementData(Real direction)
{
   const auto columns = GetPublicationColumns();
   return publisher->PublishPropagatePacket(this, streamID, pubdata, expandedDim,
      direction, sats, false, NULL, &columns);
}

ObjectArray PropagationEnabledCommand::GetPropagatedSpacecraft() const
{
   ObjectArray spacecraft;
   auto add = [&](GmatBase *object)
   {
      if (object && object->IsOfType(Gmat::SPACECRAFT) &&
          std::find(spacecraft.begin(), spacecraft.end(), object) == spacecraft.end())
         spacecraft.push_back(object);
   };
   for (auto *root : sats)
   {
      if (!root) throw CommandException("Propagation event has a NULL object.");
      if (root->IsOfType(Gmat::FORMATION))
         for (auto *member : root->GetRefObjectArray(Gmat::SPACECRAFT)) add(member);
      else add(root);
   }
   return spacecraft;
}

std::vector<SpaceObject*> PropagationEnabledCommand::ResolveFormationMembers(
      FormationInterface *formation)
{
   if (!formation) throw CommandException("Cannot resolve members of a NULL Formation.");
   const StringArray names = formation->GetStringArrayParameter("Add");
   const ObjectArray bound = formation->GetRefObjectArray(Gmat::SPACECRAFT);
   std::vector<SpaceObject*> members;
   for (const auto &name : names)
   {
      GmatBase *object = NULL;
      for (GmatBase *candidate : bound)
         if (candidate && candidate->GetName() == name)
         {
            if (object && object != candidate)
               throw CommandException("Ambiguous bound member in Formation " + formation->GetName() + ": " + name);
            object = candidate;
         }
      // An unresolved configuration retains the native object-store fallback.
      // An actual retained binding always takes precedence over equal names.
      if (!object) object = FindObject(name);
      auto *member = dynamic_cast<SpaceObject*>(object);
      if (!member || !member->IsOfType(Gmat::SPACECRAFT))
         throw CommandException("Cannot resolve spacecraft " + name + " in Formation " + formation->GetName());
      if (std::find(members.begin(), members.end(), member) != members.end())
         throw CommandException("Repeated spacecraft in Formation " + formation->GetName());
      members.push_back(member);
   }
   return members;
}

PropagationEnabledCommand::BufferedRoot PropagationEnabledCommand::CaptureBufferRoot(SpaceObject *object)
{
   if (!object) throw CommandException("Cannot buffer a NULL propagation object.");
   BufferedRoot root;
   root.object = object;
   root.context = object->GetFunctionValueContext();
   root.stateLayout = object->GetState().GetLayoutIdentity();
   if (auto *formation = dynamic_cast<FormationInterface*>(object))
      root.members = ResolveFormationMembers(formation);
   else if (object->IsOfType(Gmat::SPACECRAFT)) root.members.push_back(object);
   else throw CommandException("Cannot buffer propagation object " + object->GetName());
   for (SpaceObject *member : root.members)
   {
      root.stateSizes.push_back(member->GetState().GetSize());
      root.memberContexts.push_back(member->GetFunctionValueContext());
      root.memberStateLayouts.push_back(member->GetState().GetLayoutIdentity());
   }
   return root;
}

bool PropagationEnabledCommand::BufferBindingsChanged()
{
   if (!transientBindings.Matches(transientForces)) return true;
   // RunComplete retires snapshots. The next preparation must capture new
   // sources before touching models that can still reference the old scope.
   if (bufferRoots.empty()) return true;
   for (const auto &entry : bufferRoots)
   {
      auto *object = dynamic_cast<SpaceObject*>(FindObject(entry.first));
      if (!object || object != entry.second.object) return true;
      const BufferedRoot current = CaptureBufferRoot(object);
      if (current.context != entry.second.context || current.members != entry.second.members ||
          current.stateLayout != entry.second.stateLayout ||
          current.stateSizes != entry.second.stateSizes ||
          current.memberContexts != entry.second.memberContexts ||
          current.memberStateLayouts != entry.second.memberStateLayouts) return true;
   }
   return false;
}

void PropagationEnabledCommand::AppendBuffer(SpaceObject *object)
{
   for (const auto &entry : bufferSources)
      if (entry.second == object) return;
   std::unique_ptr<GmatBase> clone(object->Clone());
   bool borrowed = clone.get() == object;
   for (const auto &entry : bufferSources)
      borrowed = borrowed || clone.get() == entry.first || clone.get() == entry.second;
   if (borrowed)
   {
      clone.release();
      throw CommandException("Propagation buffer Clone returned an existing value.");
   }
   auto *buffer = dynamic_cast<SpaceObject*>(clone.get());
   if (!buffer || buffer->GetTypeName() != object->GetTypeName())
      throw CommandException("Incompatible propagation buffer clone for " + object->GetName());
   bufferSources.emplace(buffer, object);
   try
   {
      if (auto *satellite = dynamic_cast<Spacecraft*>(buffer)) satBuffer.push_back(satellite);
      else if (auto *formation = dynamic_cast<FormationInterface*>(buffer)) formBuffer.push_back(formation);
      else throw CommandException("Unsupported propagation buffer type " + object->GetTypeName());
   }
   catch (...)
   {
      bufferSources.erase(buffer);
      throw;
   }
   clone.release();
   #ifdef DEBUG_MEMORY
   MemoryTracker::Instance()->Add(buffer, buffer->GetName(), "PropagationEnabledCommand::AppendBuffer()",
         "native propagation buffer Clone");
   #endif
}

void PropagationEnabledCommand::AddToBuffer(SpaceObject *so)
{
   BufferedRoot root = CaptureBufferRoot(so);
   const auto existing = bufferRoots.find(so->GetName());
   if (existing != bufferRoots.end() && existing->second.object != so)
      throw CommandException("Ambiguous propagation root name " + so->GetName());
   try
   {
      bufferRoots[so->GetName()] = root;
      AppendBuffer(so);
      if (so->IsOfType(Gmat::FORMATION))
         for (SpaceObject *member : root.members) AppendBuffer(member);
   }
   catch (...)
   {
      EmptyBuffer();
      throw;
   }
}

//------------------------------------------------------------------------------
// void EmptyBuffer()
//------------------------------------------------------------------------------
/**
 * Cleans up the satellite state buffer.
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::EmptyBuffer()
{
   for (std::vector<Spacecraft *>::iterator i = satBuffer.begin();
        i != satBuffer.end(); ++i)
   {
      #ifdef DEBUG_MEMORY
      MemoryTracker::Instance()->Remove
         ((*i), (*i)->GetName(), "Propagate::EmptyBuffer()", "deleting from satBuffer");
      #endif
      delete (*i);
   }
   satBuffer.clear();

   for (std::vector<FormationInterface *>::iterator i = formBuffer.begin();
        i != formBuffer.end(); ++i)
   {
      #ifdef DEBUG_MEMORY
      MemoryTracker::Instance()->Remove
         ((*i), (*i)->GetName(), "Propagate::EmptyBuffer()", "deleting from fromBuffer");
      #endif
      delete (*i);
   }
   formBuffer.clear();
   bufferSources.clear();
   bufferRoots.clear();
   transientBindings.Clear();
}

void PropagationEnabledCommand::RunComplete()
{
   EmptyBuffer();
   inProgress = false;
   hasFired = false;
   GmatCommand::RunComplete();
}

//------------------------------------------------------------------------------
// void BufferSatelliteStates(bool fillingBuffer)
//------------------------------------------------------------------------------
/**
 * Preserves satellite state data so it can be restored after interpolating the
 * stopping condition propagation time.
 *
 * @param <fillingBuffer> Flag used to indicate the fill direction.
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::BufferSatelliteStates(bool fillingBuffer)
{
   Spacecraft *fromSat, *toSat;
   FormationInterface *fromForm, *toForm;
   std::string soName;

   for (std::vector<Spacecraft *>::iterator i = satBuffer.begin();
        i != satBuffer.end(); ++i)
   {
      soName = (*i)->GetName();
      const auto origin = bufferSources.find(*i);
      auto *original = origin == bufferSources.end() ? NULL : dynamic_cast<Spacecraft*>(origin->second);
      if (!original) throw CommandException("Missing exact spacecraft source for propagation buffer " + soName);
      fromSat = fillingBuffer ? original : *i;
      toSat = fillingBuffer ? *i : original;

      #ifdef DEBUG_STATE_BUFFERING
         MessageInterface::ShowMessage(
            "   Sat is %s, fill direction is %s; fromSat epoch = %.12lf   "
            "toSat epoch = %.12lf\n",
            fromSat->GetName().c_str(),
            (fillingBuffer ? "from propagator" : "from buffer"),
            fromSat->GetRealParameter("A1Epoch"),
            toSat->GetRealParameter("A1Epoch"));

         MessageInterface::ShowMessage(
            "   '%s' Satellite state:\n", fromSat->GetName().c_str());
         Real *satrv = fromSat->GetState().GetState();
         MessageInterface::ShowMessage(
            "      %.12lf  %.12lf  %.12lf\n      %.12lf  %.12lf  %.12lf\n",
            satrv[0], satrv[1], satrv[2], satrv[3], satrv[4], satrv[5]);
      #endif

      // Whole native Copy adds destination graph rebinding after operator=,
      // so retained frames do not keep backlinks to the temporary buffer root.
      toSat->Copy(fromSat);

      #ifdef DEBUG_STATE_BUFFERING
         MessageInterface::ShowMessage(
            "After copy, From epoch %.12lf to epoch %.12lf\n",
            fromSat->GetRealParameter("A1Epoch"),
            toSat->GetRealParameter("A1Epoch"));
      #endif
   }

   for (std::vector<FormationInterface *>::iterator i = formBuffer.begin();
        i != formBuffer.end(); ++i)
   {
      soName = (*i)->GetName();
      #ifdef DEBUG_STATE_BUFFERING
         MessageInterface::ShowMessage("Buffering formation %s, filling = %s\n",
            soName.c_str(), (fillingBuffer?"true":"false"));
      #endif
      const auto origin = bufferSources.find(*i);
      auto *original = origin == bufferSources.end() ? NULL : dynamic_cast<FormationInterface*>(origin->second);
      if (!original) throw CommandException("Missing exact Formation source for propagation buffer " + soName);
      fromForm = fillingBuffer ? original : *i;
      toForm = fillingBuffer ? *i : original;

      #ifdef DEBUG_STATE_BUFFERING
         MessageInterface::ShowMessage(
            "   Formation is %s, fill direction is %s; fromForm epoch = %.12lf"
            "   toForm epoch = %.12lf\n",
            fromForm->GetName().c_str(),
            (fillingBuffer ? "from propagator" : "from buffer"),
            fromForm->GetRealParameter("A1Epoch"),
            toForm->GetRealParameter("A1Epoch"));
      #endif

      (*toForm) = (*fromForm);

      toForm->UpdateState();

      #ifdef DEBUG_STATE_BUFFERING
         Integer count = fromForm->GetStringArrayParameter("Add").size();

         MessageInterface::ShowMessage(
            "After copy, From epoch %.12lf to epoch %.12lf\n",
            fromForm->GetRealParameter("A1Epoch"),
            toForm->GetRealParameter("A1Epoch"));

         MessageInterface::ShowMessage(
            "   %s for '%s' Formation state:\n",
            (fillingBuffer ? "Filling buffer" : "Restoring states"),
            fromForm->GetName().c_str());

         Real *satrv = fromForm->GetState().GetState();

         for (Integer i = 0; i < count; ++i)
            MessageInterface::ShowMessage(
               "      %d:  %.12lf  %.12lf  %.12lf  %.12lf  %.12lf  %.12lf\n",
               i, satrv[i*6], satrv[i*6+1], satrv[i*6+2], satrv[i*6+3],
               satrv[i*6+4], satrv[i*6+5]);
      #endif
   }

   #ifdef DEBUG_STATE_BUFFERING
      for (std::vector<Spacecraft *>::iterator i = satBuffer.begin();
           i != satBuffer.end(); ++i)
         MessageInterface::ShowMessage(
            "   Epoch of '%s' is %.12lf\n", (*i)->GetName().c_str(),
            (*i)->GetRealParameter("A1Epoch"));
   #endif
}


//------------------------------------------------------------------------------
// void AddTransientForce(StringArray *satnames, ForceModel *p,
//       PropagationStateManager *propMan)
//------------------------------------------------------------------------------
/**
 * Passes transient forces into the ForceModel(s).
 *
 * @param satnames The array of satellites used in the ForceModel.
 * @param p        The current ForceModel that is receiving the forces.
 * @param propMan  PropagationStateManager for this PropSetup
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::AddTransientForce(StringArray *satnames,
      ODEModel *p, PropagationStateManager *propMan)
{
   #ifdef DEBUG_TRANSIENT_FORCES
   MessageInterface::ShowMessage
      ("PropagationEnabledCommand::AddTransientForce() entered, ODEModel=<%p>,"
       " transientForces=<%p>\n", p, transientForces);
   #endif

   // Find any transient force that is active and add it to the force model
   StringArray satsThatManeuver, formationSatsThatManeuver, formsThatManeuver;
   bool flagMultipleBurns = false;

   for (std::vector<PhysicalModel*>::iterator i = transientForces->begin();
        i != transientForces->end(); ++i)
   {
      PhysicalModel *transient = *i;


      // Loop through the spacecraft that go with the force model, and see if
      // they are in the spacecraft list for the current transient force
      for (StringArray::iterator current = satnames->begin();
           current != satnames->end(); ++current)
      {
         auto *target = dynamic_cast<Spacecraft*>(FindObject(*current));
         if (target && transient->TargetsSpacecraft(target))
         {
            #ifdef DEBUG_TRANSIENT_FORCES
            MessageInterface::ShowMessage
               ("   Adding transientForce <%p>'%s' to ODEModel\n", transient,
                transient->GetName().c_str());
            #endif
//            if (find(satsThatManeuver.begin(), satsThatManeuver.end(), *current)
//                  == satsThatManeuver.end())
               p->AddForce(transient);
//            else
//               flagMultipleBurns = true;
            if (find(satsThatManeuver.begin(), satsThatManeuver.end(), *current) == satsThatManeuver.end())
                  satsThatManeuver.push_back(*current);
            if (transient->DepletesMass())
            {
               propMan->SetProperty("MassFlow");
//               propMan->SetProperty("MassFlow",
//                     transient->GetRefObject(Gmat::SPACECRAFT, *current));
               #ifdef DEBUG_TRANSIENT_FORCES
                  MessageInterface::ShowMessage("   %s depletes mass\n",
                        transient->GetName().c_str());
               #endif
            }
            break;      // Avoid multiple adds
         }
         else
         {
            // Check to see if a Formation is masking the Spacecraft, and if so
            // whine and stop
            // Find the object that matches the current name
            GmatBase *obj;
            for (UnsignedInt i = 0; i < sats.size(); ++i)
            {
               if (sats[i]->GetName() == *current)
               {
                  obj = sats[i];

                  #ifdef DEBUG_TRANSIENT_FORCES
                     MessageInterface::ShowMessage("Checking %s\n",
                           current->c_str());
                  #endif

                  if (obj->IsOfType(Gmat::FORMATION))
                  {
                     StringArray mansats =
                           ((SpaceObject*)(obj))->GetManeuveringMembers();

                     #ifdef DEBUG_TRANSIENT_FORCES
                        MessageInterface::ShowMessage("***It's a Formation "
                              "with %d maneuvering sats\n", mansats.size());
                     #endif

                     for (UnsignedInt memberIndex = 0; memberIndex < mansats.size(); ++memberIndex)
                     {
                        #ifdef DEBUG_TRANSIENT_FORCES
                           MessageInterface::ShowMessage("   %d: %s is maneuvering\n",
                                 memberIndex, mansats[memberIndex].c_str());
                        #endif
                        auto *member = dynamic_cast<Spacecraft*>(obj->GetRefObject(Gmat::SPACECRAFT, mansats[memberIndex]));
                        if (transient->TargetsSpacecraft(member))
                        {
                           if (find(formationSatsThatManeuver.begin(),
                                 formationSatsThatManeuver.end(),
                                 mansats[memberIndex]) == formationSatsThatManeuver.end())
                              formationSatsThatManeuver.push_back(mansats[memberIndex]);
                           if (find(formsThatManeuver.begin(),
                                 formsThatManeuver.end(), obj->GetName()) ==
                                       formsThatManeuver.end())
                              formsThatManeuver.push_back(obj->GetName());
                        }
                     }
                  }
               }
            }
         }
      }
   }

   #ifdef DEBUG_TRANSIENT_FORCES
      MessageInterface::ShowMessage("Found %d sats that maneuver (outside of "
            "formations):\n", satsThatManeuver.size());
      for (UnsignedInt i = 0; i < satsThatManeuver.size(); ++i)
         MessageInterface::ShowMessage("   %s\n", satsThatManeuver[i].c_str());
   #endif

   if (flagMultipleBurns)
   {
      StringArray duplicates;
      for (StringArray::iterator name = satsThatManeuver.begin();
            name != satsThatManeuver.end(); ++name)
      {
         if (find(name+1, satsThatManeuver.end(), *name) !=
               satsThatManeuver.end())
         {
            if (find(duplicates.begin(), duplicates.end(), *name) ==
                  duplicates.end())
               duplicates.push_back(*name);
         }
      }
      std::string errmsg = "The Spacecraft [";
      for (UnsignedInt i = 0; i < duplicates.size(); ++i)
      {
         if (i != 0)
            errmsg += ", ";
         errmsg += duplicates[i];
      }
      if (duplicates.size() == 1)
         errmsg += "] has ";
      else
         errmsg += "] have ";
      errmsg += "more than one finite burn active, but GMAT only allows one "
         "finite burn on a given spacecraft.";
      throw CommandException(errmsg);
   }

   if (formationSatsThatManeuver.size() > 0)
   {
      std::string whatToSay, formToSay;
      if (formationSatsThatManeuver.size() == 1)
         whatToSay = "a maneuvering Spacecraft named \"" +
                     formationSatsThatManeuver[0] + "\"";
      else
      {
         whatToSay = "maneuvering Spacecraft named ";
         for (UnsignedInt i = 0; i < formationSatsThatManeuver.size(); ++i)
         {
            if (i != 0)
            {
               if (formationSatsThatManeuver.size() > 2)
                  whatToSay += ", ";
               else
                  whatToSay += " ";
               if (i == formationSatsThatManeuver.size()-1)
                  whatToSay += "and ";
            }
            whatToSay += "\"" + formationSatsThatManeuver[i] + "\"";
         }
      }

      if (formsThatManeuver.size() == 1)
         formToSay = "Formation \"" + formsThatManeuver[0] + "\" contains " +
         whatToSay;
      else
      {
         formToSay = "Formations ";
         for (UnsignedInt i = 0; i < formsThatManeuver.size(); ++i)
         {
            if (i != 0)
            {
               if (formsThatManeuver.size() > 2)
                  formToSay += ", ";
               else
                  formToSay += " ";
               if (i == formsThatManeuver.size()-1)
                  formToSay += "and ";
            }
            formToSay += "\"" + formsThatManeuver[i] + "\"";
         }

         formToSay += " contain " + whatToSay;
      }

      throw CommandException("The " + formToSay + ", but GMAT does not support "
            "finite burn maneuvers inside of Formation objects.");
   }

   #ifdef DEBUG_TRANSIENT_FORCES
      ODEModel *fm;
      PhysicalModel *pm;

      MessageInterface::ShowMessage(
         "PropagationEnabledCommand::AddTransientForces completed; force details:\n");
      for (std::vector<PropSetup*>::iterator p = propagators.begin();
           p != propagators.end(); ++p)
      {
         fm = (*p)->GetODEModel();
         if (!fm)
            throw CommandException("ODEModel not set in PropSetup \"" +
                                   (*p)->GetName() + "\"");
         MessageInterface::ShowMessage(
            "   Forces in %s:\n", fm->GetName().c_str());
         for (Integer i = 0; i < fm->GetNumForces(); ++i)
         {
            pm = fm->GetForce(i);
            MessageInterface::ShowMessage(
               "      %15s   %s\n", pm->GetTypeName().c_str(),
               pm->GetName().c_str());
         }
      }
   #endif
}


//------------------------------------------------------------------------------
// void ClearTransientForce()
//------------------------------------------------------------------------------
/**
 * Removes transient forces from the ForceModel(s) after propagation.
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::ClearTransientForces()
{
   #ifdef DEBUG_TRANSIENT_FORCES
   MessageInterface::ShowMessage
      ("PropagationEnabledCommand::ClearTransientForces() entered, prop.size()=%d\n",
       propagators.size());
   #endif

   ODEModel *fm;
   PhysicalModel *pm;

   // Loop through the forces in each force model, and remove transient ones
   for (std::vector<PropSetup*>::iterator p = propagators.begin();
        p != propagators.end(); ++p)
   {
      if ((*p)->GetPropagator()->UsesODEModel())
      {
         fm = (*p)->GetODEModel();
         if (!fm)
            throw CommandException("ForceModel not set in PropSetup \"" +
                                   (*p)->GetName() + "\"");

         #ifdef DEBUG_TRANSIENT_FORCES
         MessageInterface::ShowMessage("   ODEModel=<%p>\n", fm);
         #endif
         for (Integer i = 0; i < fm->GetNumForces(); ++i)
         {
            pm = fm->GetForce(i);
            #ifdef DEBUG_TRANSIENT_FORCES
            MessageInterface::ShowMessage
               ("      Checking if pm<%p>'%s' is transient\n", pm, pm->GetName().c_str());
            #endif
            if (pm->IsTransient())
            {
               #ifdef DEBUG_TRANSIENT_FORCES
               MessageInterface::ShowMessage("   calling fm->DeleteForce() for %p\n", pm);
               #endif
               fm->DeleteForce(pm);
               --i;
            }
         }
      }
   }

   #ifdef DEBUG_TRANSIENT_FORCES
      MessageInterface::ShowMessage(
         "PropagationEnabledCommand::ClearTransientForces completed; force details:\n");
      for (std::vector<PropSetup*>::iterator p = propagators.begin();
           p != propagators.end(); ++p)
      {
         if ((*p)->GetPropagator()->UsesODEModel())
         {
            fm = (*p)->GetODEModel();
            if (!fm)
               throw CommandException("ForceModel not set in PropSetup \"" +
                                      (*p)->GetName() + "\"");
            #ifdef DEBUG_TRANSIENT_FORCES
            MessageInterface::ShowMessage("   ODEModel=<%p>\n", fm);
            #endif
            MessageInterface::ShowMessage(
               "      Forces in %s:\n", fm->GetName().c_str());
            for (Integer i = 0; i < fm->GetNumForces(); ++i)
            {
               pm = fm->GetForce(i);
               MessageInterface::ShowMessage(
                   "      %15s   %s\n", pm->GetTypeName().c_str(),
                   pm->GetName().c_str());
             }
         }
      }
   #endif
}

//------------------------------------------------------------------------------
// bool PropagationEnabledCommand::IsSmootherPredicting()
//------------------------------------------------------------------------------
/**
 *  UNDEFINED 
 */
 //------------------------------------------------------------------------------
bool PropagationEnabledCommand::IsSmootherPredicting()
{
   throw CommandException("IsSmootherPredicting() is not defined in PropagationEnabledCommand()\n");
}


//Moved from PropagateCommand for GMT-5101 fix (LOJ: 2015.05.14)
//------------------------------------------------------------------------------
// void SetNames(const std::string& name, StringArray& owners,
//               StringArray& elements)
//------------------------------------------------------------------------------
/**
 * Sets the parameter names used when publishing Spacecraft data.
 *
 * @param <name>     Name of the Spacecraft that is referenced.
 * @param <owners>   Array of published data identifiers.
 * @param <elements> Individual elements of the published data.
 */
//------------------------------------------------------------------------------
void PropagationEnabledCommand::SetNames(const std::string& name, StringArray& owners,
                         StringArray& elements)
{
   // Add satellite labels
   for (Integer i = 0; i < 6; ++i)
      owners.push_back(name);
   
   // X, Y, Z, Vx, Vy, Vz
   elements.push_back(name+".X");
   elements.push_back(name+".Y");
   elements.push_back(name+".Z");
   elements.push_back(name+".Vx");
   elements.push_back(name+".Vy");
   elements.push_back(name+".Vz");
   
   #ifdef DEBUG_PUBLISH_DATA
   MessageInterface::ShowMessage
      ("PropagationEnabledCommand::SetNames() Setting data labels:\n");
   for (unsigned int i = 0; i < elements.size(); i++)
      MessageInterface::ShowMessage("%s ", elements[i].c_str());
   MessageInterface::ShowMessage("\n");
   #endif
}


//------------------------------------------------------------------------------
// Real CreateNewSegments(Real epochNow)
//------------------------------------------------------------------------------
/**
 * Creates a new segment when called. Used for creating finite burn segments
 * 
 * @param epochNow the A1MJD real epoch at which to make the cut
 *
 * @return true if needs to set up a segment cut, returns false if the cut has been made or is not needed
 *
 */
 //------------------------------------------------------------------------------
bool PropagationEnabledCommand::CreateNewSegments(Real epochNow)
{
   if (createNewSegmentsNow == false)
   {
      const ObjectArray eventSpacecraft = GetPropagatedSpacecraft();
      std::list<Subscriber *> subList = publisher->GetSubscriberList();
      std::list<Subscriber *> ::iterator it;
      for (it = subList.begin(); it != subList.end(); it++)
      {
         if ((*it)->GetTypeName() == "EphemerisFile" && (*it)->GetBooleanParameter("WriteEphemeris") == true)
         {
            auto *subject = (*it)->GetRefObject(Gmat::SPACECRAFT,
               (*it)->GetStringParameter("Spacecraft"));
            if (!subject || std::find(eventSpacecraft.begin(), eventSpacecraft.end(), subject) == eventSpacecraft.end())
               continue;
            bool ephemTypeCorrect = ((*it)->GetStringParameter("FileFormat").substr(0, 3) == "STK" || 
                                     (*it)->GetStringParameter("FileFormat").substr(0, 9) == "CCSDS-OEM" || 
                                     (*it)->GetStringParameter("FileFormat").substr(0, 3) == "SPK");
            if (ephemTypeCorrect)
            {
               createNewSegmentsNow = true;
            }
         }
      }
      if (publishOnStep == false && createNewSegmentsNow == true)
      {
         
         // This block adds the duplicate state entry on each side of the segment
         Real timeToAdd;
         Real timeSoFar;
         GmatTime timeSoFarGT;
         // Integer stateSize = 0;
         // Integer indx = 1;
         for (UnsignedInt i = 0; i < fm.size(); ++i)
         {
            // orbit related parameters use spacecraft for data
            if (fm[i])
               timeToAdd = fm[i]->GetTime();
            else
               timeToAdd = p[i]->GetTime();
            if (elapsedTime.size() > 0)
               elapsedTime[i] = timeToAdd;

            timeSoFar = baseEpoch[i] + timeToAdd / GmatTimeConstants::SECS_PER_DAY;
            timeSoFarGT = baseEpochGT[i];
            timeSoFarGT.AddSeconds(timeToAdd);
            if (currEpoch.size() > 0)
            {
               currEpoch[i] = timeSoFar;
               currEpochGT[i] = timeSoFarGT;
            }
            // Update spacecraft epoch, without argument the spacecraft epoch
            // won't get updated for consecutive Propagate command
            if (fm[i])
            {
               if (fm[i]->HasPrecisionTime())
                  fm[i]->UpdateSpaceObjectGT(timeSoFarGT);
               else
                  fm[i]->UpdateSpaceObject(timeSoFar);
            }
            else
            {
               if (p[i]->HasPrecisionTime())
                  p[i]->UpdateSpaceObjectGT(timeSoFarGT);
               else
                  p[i]->UpdateSpaceObject(timeSoFar);
            }

            // j2kState = p[i]->GetJ2KState();
            // Integer size = p[i]->GetDimension();
            // memcpy(&pubdata[indx], j2kState, size * sizeof(Real));
            // indx += size;
            // stateSize += size;
         }
         // Publish the data here
         for (Integer idx = 0; idx < pubdataSize; ++idx)
            pubdata[idx] = GmatMathConstants::QUIET_NAN;

         if (hasPrecisionTime)
            pubdata[0] = timeSoFarGT.GetMjd();
         else
            pubdata[0] = timeSoFar;

         //Updates covariance if applicable
         UpdateCov();         
         
         //publisher->Publish(this, streamID, pubdata, stateSize + 1, direction);
         //Integer covDim = 21 * sats.size();
         //memcpy(&pubdata[1], j2kState, dim * sizeof(Real));
         //publisher->Publish(this, streamID, pubdata, dim + covDim + 1, direction);
         
         // Prepare data to buffer for publishing
         //Integer len = 1;
         PrepareElementDataToPublish(pubdata, pubdataSize, pubdataElements, covStartIndex, attStartIndex);
         // Publish 
         PublishElementData(direction);


         for (ObjectArray::iterator sat = sats.begin(); sat != sats.end(); sat++)
         {
            //publisher->SetSpacecraftPropertyChanged(this, currEpochGT[0].GetMjd(), (*sat)->GetName(), "FiniteBurn");
         }
         //createNewSegmentsNow = false;
         CreateNewSegments(epochNow);
      }
   }
   else
   {
      const ObjectArray eventSpacecraft = GetPropagatedSpacecraft();
      std::list<Subscriber *> subList = publisher->GetSubscriberList();
      std::list<Subscriber *> ::iterator it;
      for (it = subList.begin(); it != subList.end(); it++)
      {
         if ((*it)->GetTypeName() == "EphemerisFile" && (*it)->GetBooleanParameter("WriteEphemeris") == true)
         {
            auto *subject = (*it)->GetRefObject(Gmat::SPACECRAFT,
               (*it)->GetStringParameter("Spacecraft"));
            if (!subject || std::find(eventSpacecraft.begin(), eventSpacecraft.end(), subject) == eventSpacecraft.end())
               continue;
            bool ephemTypeCorrect = ((*it)->GetStringParameter("FileFormat").substr(0, 3) == "STK" || 
                                     (*it)->GetStringParameter("FileFormat").substr(0, 9) == "CCSDS-OEM" || 
                                     (*it)->GetStringParameter("FileFormat").substr(0, 3) == "SPK");
            if (ephemTypeCorrect)
            {
               createNewSegmentsNow = false;
               if ((*it)->GetStringParameter("FileFormat").substr(0, 3) == "STK" || 
                   (*it)->GetStringParameter("FileFormat").substr(0, 9) == "CCSDS-OEM" ||
                   (*it)->GetStringParameter("FileFormat").substr(0, 3) == "SPK")
               {
                  if (IsOfType("RunSmoother"))
                  {
                     std::string warningMsg = "**** WARNING ****  The output ephemeris file " + (*it)->GetFullName() + " includes a maneuver that is active for a portion of the ephemeris span.  The output states may lose precision around the epochs of start and end times of the maneuver(s).\n";
                     
                     std::string errMsg = "**** ERROR ****  The output ephemeris file " + (*it)->GetFullName() + " includes a maneuver that is active for a portion of the ephemeris span. This is disallowed in the Smoother. No ephemeris will be written out as the output states may lose precision around the epochs of start and end times of the maneuver(s)\n";

                     Integer runmode = GmatGlobal::Instance()->GetRunModeStartUp();
                     if (runmode != GmatGlobal::TESTING) 
                        if(!IsSmootherPredicting())
                           throw CommandException(errMsg);
                     else
                        MessageInterface::ShowMessage(warningMsg);
                  }
                  // This makes the cuts for the segments for STK and SPK Ephem Types
                  ((EphemerisFile*)(*it))->StartNewSegment("FiniteBurn", false, true, true);

               }
               else
               {
                  // This makes the cuts for CCSDS type Ephems
                  for (auto *satellite : GetPropagatedSpacecraft())
                     publisher->SetSpacecraftPropertyChanged(this, epochNow,
                        satellite->GetName(), "FiniteBurn", satellite);
               }
            }
         }
      }
      
      // This block adds the duplicate state entry on each side of the segment
      Real timeToAdd;
      Real timeSoFar;
      GmatTime timeSoFarGT;
      // Integer stateSize = 0;
      // Integer indx = 1;

      for (UnsignedInt i = 0; i < fm.size(); ++i)
      {
         // orbit related parameters use spacecraft for data
         if (fm[i])
            timeToAdd = fm[i]->GetTime();
         else
            timeToAdd = p[i]->GetTime();
         if (elapsedTime.size() > 0)
            elapsedTime[i] = timeToAdd;

         timeSoFar = baseEpoch[i] + timeToAdd / GmatTimeConstants::SECS_PER_DAY;
         timeSoFarGT = baseEpochGT[i];
         timeSoFarGT.AddSeconds(timeToAdd);
         if (currEpoch.size() > 0)
         {
            currEpoch[i] = timeSoFar;
            currEpochGT[i] = timeSoFarGT;
         }
         // Update spacecraft epoch, without argument the spacecraft epoch
         // won't get updated for consecutive Propagate command
         if (fm[i])
         {
            if (fm[i]->HasPrecisionTime())
               fm[i]->UpdateSpaceObjectGT(timeSoFarGT);
            else
               fm[i]->UpdateSpaceObject(timeSoFar);
         }
         else
         {
            if (p[i]->HasPrecisionTime())
               p[i]->UpdateSpaceObjectGT(timeSoFarGT);
            else
               p[i]->UpdateSpaceObject(timeSoFar);
         }

         // j2kState = p[i]->GetJ2KState();
         // Integer size = p[i]->GetDimension();
         // memcpy(&pubdata[indx], j2kState, size * sizeof(Real));
         // indx += size; 
         // stateSize += size;
      }
      // Publish the data here
      for (Integer idx = 0; idx < pubdataSize; ++idx)
         pubdata[idx] = GmatMathConstants::QUIET_NAN;

      if (hasPrecisionTime)
         pubdata[0] = timeSoFarGT.GetMjd();
      else
         pubdata[0] = timeSoFar;

      // publisher->Publish(this, streamID, pubdata, stateSize + 1, direction);
      //Integer covDim = 21 * sats.size();
      //memcpy(&pubdata[1], j2kState, dim * sizeof(Real));
      //publisher->Publish(this, streamID, pubdata, dim + covDim + 1, direction);
      
      // Prepare data to buffer for publishing
      //Integer len = 1;
      PrepareElementDataToPublish(pubdata, pubdataSize, pubdataElements, covStartIndex, attStartIndex);
      // Publish
      PublishElementData(direction);

   }
   return createNewSegmentsNow;
}



Integer PropagationEnabledCommand::PrepareElementInfoToPublish(StringArray &owners, StringArray &elements, Integer &covStartIndex, Integer &attStartIndex)
{
   // clear the list
   owners.clear();
   elements.clear();

   // Add epoch infor with assumtion currEpoch[i] or currEpochGT[i] are the same for all i 
   owners.push_back("All");
   elements.push_back("All.epoch");

   for (Integer i = 0; i < propagators.size(); ++i)
   {
      // Add elements in ListItems for each propagator
      PropagationStateManager *psm = propagators[i]->GetPropStateManager();
      psm->PrepareStateInfoToPublish(owners, elements);
   }
   covStartIndex = elements.size();

   for (Integer i = 0; i < propagators.size(); ++i)
   {
      // Add accelerations, and covariances
      PropagationStateManager *psm = propagators[i]->GetPropStateManager();
      psm->PrepareCovAndAccelerationInfoToPublish(owners, elements);
   }
   attStartIndex = elements.size();

   for (Integer i = 0; i < propagators.size(); ++i)
   {
      // Add attitude
      PropagationStateManager* psm = propagators[i]->GetPropStateManager();
      psm->PrepareAttitudeInfoToPublish(owners, elements);
   }
   
   Integer numOfElements = elements.size();

   return numOfElements;
}



bool PropagationEnabledCommand::PrepareElementDataToPublish(Real* publishData, Integer publishDataSize, StringArray &elementNames, Integer covStartIndex, Integer attStartIdx)
{
   // PrepareElementInfoToPublish registers one state block per PSM in this
   // exact order. Searching for the first matching label merges distinct
   // retained/function objects which happen to share the same instance name.
   size_t stateOffset = 1;
   for (auto *propagator : propagators)
   {
      auto *psm = propagator->GetPropStateManager();
      auto *state = psm->GetState();
      const auto *map = psm->GetStateMap();
      StringArray owners, elements;
      psm->PrepareStateInfoToPublish(owners, elements);
      if (!publishData || !state || !map || publishDataSize <= 0 || covStartIndex < 1 ||
          map->size() != static_cast<size_t>(state->GetSize()) || elements.size() != map->size() ||
          stateOffset > static_cast<size_t>(publishDataSize) || elements.size() > static_cast<size_t>(publishDataSize) - stateOffset ||
          stateOffset > elementNames.size() || elements.size() > elementNames.size() - stateOffset ||
          stateOffset > static_cast<size_t>(covStartIndex) || elements.size() > static_cast<size_t>(covStartIndex) - stateOffset)
         throw CommandException("Published state storage differs from the registered PSM layout.");
      for (size_t j = 0; j < elements.size(); ++j)
      {
         if (elementNames[stateOffset+j] != elements[j])
            throw CommandException("Published state column registration is stale.");
         publishData[stateOffset+j] = state->GetState()[j];
      }
      stateOffset += elements.size();
   }
   if (stateOffset != static_cast<size_t>(covStartIndex))
      throw CommandException("Published state blocks do not match their registered extent.");

   size_t offset = static_cast<size_t>(covStartIndex);
   auto fillGroup = [&](PropagationStateManager *manager, bool attitude)
   {
      StringArray owners, elements;
      if (attitude) manager->PrepareAttitudeInfoToPublish(owners, elements);
      else manager->PrepareCovAndAccelerationInfoToPublish(owners, elements);
      const size_t end = attitude ? elementNames.size() : static_cast<size_t>(attStartIdx);
      if (offset > end || elements.size() > end - offset || offset > elementNames.size() ||
          elements.size() > elementNames.size() - offset ||
          !std::equal(elements.begin(), elements.end(), elementNames.begin() + offset))
         throw CommandException("Published auxiliary group differs from the registered layout.");
      if (attitude) manager->PrepareAttitudeDataToPublish(publishData, publishDataSize, elementNames,
         static_cast<Integer>(offset), hasPrecisionTime);
      else manager->PrepareCovAndAccelerationDataToPublish(publishData, publishDataSize, elementNames,
         static_cast<Integer>(offset), hasPrecisionTime);
      offset += elements.size();
   };
   for (auto *propagator : propagators) fillGroup(propagator->GetPropStateManager(), false);
   if (offset != static_cast<size_t>(attStartIdx))
      throw CommandException("Published covariance groups do not end at the attitude start.");
   for (auto *propagator : propagators) fillGroup(propagator->GetPropStateManager(), true);
   if (offset != elementNames.size() || offset > static_cast<size_t>(publishDataSize))
      throw CommandException("Published attitude groups do not match the packet extent.");

   return true;
}


//------------------------------------------------------------------------------
// void PropagationEnabledCommand::UpdateCov()
//------------------------------------------------------------------------------
/**
 * Updates covariance during a finite burn. 
 * This method does nothing and will rarely be reached in this class.
 *
 */
 //------------------------------------------------------------------------------
void PropagationEnabledCommand::UpdateCov()
{
   //Do nothing
}
