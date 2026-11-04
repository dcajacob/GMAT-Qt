//$Id$
//------------------------------------------------------------------------------
//                            Publisher
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
// number S-67573-G
//
// Author: LaMont Ruley
// Created: 2003/09/21
//
/**
 *  Implementation code for the Publisher class.
 */
//------------------------------------------------------------------------------

#include "Publisher.hpp"
#include "PlotOriginScope.hpp"
#include "SpaceObject.hpp"
#include <limits>
#include "PublisherException.hpp"
#include "MessageInterface.hpp"
#include "Moderator.hpp"
#include <string>
#include <algorithm>               // for find()
#include <cmath>
#include <set>

namespace
{
   struct EpochPacket
   {
      const Real *data = NULL;
      Integer count = 0;
      const GmatTime *epoch = NULL;
      const std::vector<Publisher::CartesianSample> *samples = NULL;
      const void *token = NULL;
   };
   thread_local EpochPacket requestedEpoch;
   thread_local EpochPacket activeEpoch;
   thread_local const void *parameterToken = NULL;

   struct EpochRequest
   {
      EpochPacket previous;
      EpochRequest(const Real *data, Integer count, const GmatTime &epoch,
         const std::vector<Publisher::CartesianSample> *samples = NULL) :
         previous(requestedEpoch)
      { requestedEpoch = {data, count, &epoch, samples, this}; }
      ~EpochRequest() { requestedEpoch = previous; }
   };

   struct EpochDispatch
   {
      EpochPacket previous;
      EpochDispatch(const Real *data, Integer count) : previous(activeEpoch)
      {
         activeEpoch = requestedEpoch.data == data && requestedEpoch.count == count
            ? requestedEpoch : EpochPacket{};
         // Consume the request before callbacks. Nested legacy publications,
         // even with the same pointer, must not inherit the outer epoch.
         requestedEpoch = EpochPacket{};
      }
      ~EpochDispatch() { activeEpoch = previous; }
   };
}

bool Publisher::PublishWithEpoch(GmatBase *provider, Integer id, Real *data,
                                 Integer count, const GmatTime &epoch, Real propDir)
{
   if (!data || count <= 0 || !std::isfinite(data[0]) ||
       !std::isfinite(epoch.GetFracSec()) || data[0] != epoch.GetMjd())
      throw PublisherException("Precise publication epoch must match its Real packet epoch.");
   EpochRequest request(data, count, epoch);
   // Preserve existing derived-Publisher dispatch and behavior.
   return Publish(provider, id, data, count, propDir);
}

bool Publisher::GetPublishedEpoch(const Real *data, Integer count, GmatTime &epoch)
{
   if (!data || count <= 0 || activeEpoch.data != data ||
       activeEpoch.count != count || !activeEpoch.epoch ||
       data[0] != activeEpoch.epoch->GetMjd())
      return false;
   epoch = *activeEpoch.epoch;
   return true;
}

bool Publisher::PublishWithSamples(GmatBase *provider, Integer id, Real *data,
   Integer count, const GmatTime &sourceEpoch,
   const std::vector<CartesianSample> &samples, Real propDir)
{
   if (!data || count <= 0 || !std::isfinite(data[0]) ||
       !std::isfinite(sourceEpoch.GetFracSec()) || data[0] != sourceEpoch.GetMjd())
      throw PublisherException("Sampled publication requires a matching source epoch.");
   std::set<const GmatBase *> objects;
   for (const auto &sample : samples)
   {
      if (!sample.object || !objects.insert(sample.object).second ||
          !std::isfinite(sample.epoch.GetFracSec()))
         throw PublisherException("Sampled publication has invalid object or epoch metadata.");
      for (Integer i = 0; i < 6; ++i)
         if (!std::isfinite(sample.state[i]))
            throw PublisherException("Sampled publication has a nonfinite Cartesian state.");
   }
   // Snapshot the caller's vector: callbacks cannot change later consumers'
   // values by modifying the producer's storage during synchronous dispatch.
   const std::vector<CartesianSample> frozen(samples);
   const GmatTime frozenEpoch(sourceEpoch);
   EpochRequest request(data, count, frozenEpoch, &frozen);
   return Publish(provider, id, data, count, propDir);
}

bool Publisher::GetPublishedCartesianState(const Real *data, Integer count,
   const GmatBase *object, GmatTime &epoch, Rvector6 &state)
{
   GmatTime source;
   if (!object || !activeEpoch.samples || !GetPublishedEpoch(data, count, source))
      return false;
   for (const auto &sample : *activeEpoch.samples)
      if (sample.object == object)
      {
         epoch = sample.epoch;
         state = sample.state;
         return true;
      }
   return false;
}

Publisher::ParameterStateScope::ParameterStateScope(const Real *data, Integer count) :
   previous(parameterToken)
{
   GmatTime source;
   parameterToken = Publisher::GetPublishedEpoch(data, count, source) ?
      activeEpoch.token : NULL;
}

Publisher::ParameterStateScope::~ParameterStateScope()
{
   parameterToken = previous;
}

bool Publisher::GetParameterCartesianState(const GmatBase *object,
   GmatTime &epoch, Rvector6 &state)
{
   return parameterToken && parameterToken == activeEpoch.token &&
      GetPublishedCartesianState(activeEpoch.data, activeEpoch.count, object, epoch, state);
}



namespace
{
   std::set<const Subscriber*> integrationStepConsumers;
   struct IntegrationPacket
   {
      const Real *data = NULL;
      Integer count = 0;
      const std::vector<Publisher::IntegrationStep> *steps = NULL;
      const ObjectArray *objects = NULL;
      bool initial = false;
   };
   thread_local IntegrationPacket requestedIntegration;
   thread_local IntegrationPacket activeIntegration;
   struct IntegrationRequest
   {
      IntegrationPacket previous;
      IntegrationRequest(const Real *data, Integer count,
         const std::vector<Publisher::IntegrationStep> &steps) : previous(requestedIntegration)
      {
         requestedIntegration = {data, count, &steps};
         if (previous.data == data && previous.count == count)
         {
            requestedIntegration.objects = previous.objects;
            requestedIntegration.initial = previous.initial;
         }
      }
      IntegrationRequest(const Real *data, Integer count, const ObjectArray &objects,
                         bool initial) : previous(requestedIntegration)
      { requestedIntegration = {data, count, NULL, &objects, initial}; }
      ~IntegrationRequest() { requestedIntegration = previous; }
   };
   struct IntegrationDispatch
   {
      IntegrationPacket previous;
      IntegrationDispatch(const Real *data, Integer count) : previous(activeIntegration)
      {
         activeIntegration = requestedIntegration.data == data && requestedIntegration.count == count
            ? requestedIntegration : IntegrationPacket{};
         requestedIntegration = IntegrationPacket{};
      }
      ~IntegrationDispatch() { activeIntegration = previous; }
   };
}

namespace { thread_local const PlotOriginScope *active = nullptr; }

PlotOriginScope::Packet::Packet(const Real *raw, Integer length, SpaceObject *origin) :
   Packet(raw, length, origin ? std::vector<SpaceObject*>{origin} : std::vector<SpaceObject*>{}) {}

PlotOriginScope::Packet::Packet(const Real *raw, Integer length,
      const std::vector<SpaceObject*> &dependencies) :
   data(raw), count(length), steps(Publisher::GetPublishedIntegrationSteps(raw, length))
{
   for (auto *object : dependencies)
      if (object && std::find(objects.begin(),objects.end(),object)==objects.end())
         objects.push_back(object);
   if (!steps) return;
   for (const auto &step : *steps)
      for (const auto &sample : step.states)
         for (auto *object : objects)
         {
            if (sample.object != object) continue;
            auto added = byObject[object].emplace(sample.epoch, &sample.state);
            auto &stored = added.first->second;
            if (!added.second && stored)
               for (Integer i=0; i<6; ++i)
                  if ((*stored)[i] != sample.state[i]) { stored = nullptr; break; }
         }
}

PlotOriginScope::PlotOriginScope(const Packet &source, const GmatTime &time) :
   previous(active), packet(source), epoch(time)
{
   if (packet.steps) for (auto *object : packet.objects)
   {
      Rvector6 state;
      bool valid=false, recorded=false;
      const auto index=packet.byObject.find(object);
      if (index!=packet.byObject.end())
      {
         const auto found=index->second.find(epoch);
         if (found!=index->second.end())
         {
            recorded=true;
            if (found->second) { state=*found->second; valid=true; }
         }
      }
      if (!recorded && (object->HasPrecisionTime() ? object->GetEpochGT()==epoch :
                        object->GetEpoch()==epoch.GetMjd()))
      {
         auto &live=object->GetState();
         if (live.GetSize()>=6)
         {
            for (Integer i=0; i<6; ++i) state[i]=live[i];
            valid=true;
         }
      }
      if (valid) for (Integer i=0; i<6; ++i)
         if (!std::isfinite(state[i]) || std::abs(state[i])>=(std::numeric_limits<Real>::max)())
            valid=false;
      if (valid) states.emplace(object,state);
   }
   active=this;
}

PlotOriginScope::~PlotOriginScope() { active=previous; }

SpaceObject *PlotOriginScope::MissingObject() const
{
   for (auto *object : packet.objects) if (!states.count(object)) return object;
   return nullptr;
}

bool PlotOriginScope::Matches(const SpaceObject *candidate) const
{
   return states.count(candidate) && packet.steps &&
      Publisher::GetPublishedIntegrationSteps(packet.data,packet.count)==packet.steps;
}

bool PlotOriginScope::Epoch(const SpaceObject *candidate, Real mjd, GmatTime &time)
{
   if (!active || !active->Matches(candidate) || active->epoch.GetMjd()!=mjd) return false;
   time=active->epoch;return true;
}

const Rvector6 *PlotOriginScope::Find(const SpaceObject *candidate, Real mjd)
{
   return active && active->Matches(candidate) && active->epoch.GetMjd()==mjd ? &active->states.at(candidate) : nullptr;
}

const Rvector6 *PlotOriginScope::Find(const SpaceObject *candidate, const GmatTime &time)
{
   return active && active->Matches(candidate) && active->epoch==time ? &active->states.at(candidate) : nullptr;
}

void Publisher::SetIntegrationStepConsumer(Subscriber *subscriber, bool enabled)
{
   if (!subscriber) return;
   if (enabled) integrationStepConsumers.insert(subscriber);
   else integrationStepConsumers.erase(subscriber);
}

bool Publisher::HasIntegrationStepConsumers() const
{
   for (auto *subscriber : subscriberList)
      if (integrationStepConsumers.count(subscriber)) return true;
   return false;
}

const std::vector<Publisher::IntegrationStep>* Publisher::GetPublishedIntegrationSteps(
   const Real *data, Integer count)
{
   return data && count > 0 && activeIntegration.data == data &&
      activeIntegration.count == count ? activeIntegration.steps : NULL;
}

const ObjectArray *Publisher::GetPublishedPropagationObjects(const Real *data,
   Integer count, bool &initial)
{
   initial = false;
   if (!data || count <= 0 || data != activeIntegration.data || count != activeIntegration.count)
      return NULL;
   initial = activeIntegration.initial;
   return activeIntegration.objects;
}

bool Publisher::PublishPropagatePacket(GmatBase *provider, Integer id, Real *data,
   Integer count, Real direction, const ObjectArray &objects, bool initial,
   const std::vector<IntegrationStep> *steps)
{
   IntegrationRequest request(data, count, objects, initial);
   if (steps) return PublishWithIntegrationSteps(provider, id, data, count, *steps, direction);
   return Publish(provider, id, data, count, direction);
}

bool Publisher::PublishWithIntegrationSteps(GmatBase *provider, Integer id,
   Real *data, Integer count, const std::vector<IntegrationStep> &steps, Real propDir)
{
   if (!data || count <= 0 || !std::isfinite(data[0]))
      throw PublisherException("Integration-step publication requires a finite source packet.");
   for (const auto &step : steps)
   {
      if (!std::isfinite(step.startSeconds) || !std::isfinite(step.endSeconds) ||
          !std::isfinite(step.epoch.GetMjd()) || !std::isfinite(step.epoch.GetFracSec()) ||
          (!step.initial && step.startSeconds == step.endSeconds))
         throw PublisherException("Integration-step publication requires a nonzero accepted advance.");
      std::set<const GmatBase*> objects;
      for (const auto &sample : step.states)
      {
         if (!sample.object || !objects.insert(sample.object).second || sample.epoch != step.epoch)
            throw PublisherException("Integration-step object/epoch bindings are invalid.");
         for (Integer i = 0; i < 6; ++i)
            if (!std::isfinite(sample.state[i]))
               throw PublisherException("Integration-step Cartesian state is nonfinite.");
      }
   }
   const std::vector<IntegrationStep> frozen(steps);
   IntegrationRequest request(data, count, frozen);
   return Publish(provider, id, data, count, propDir);
}

//#define DBGLVL_PUBLISHER_SUBSCRIBE 1
//#define DBGLVL_PUBLISHER_REGISTER 2
//#define DBGLVL_PUBLISHER_PUBLISH 2
//#define DBGLVL_PUBLISHER_CLEAR 1
//#define DEBUG_PUBLISHER_RUN_STATE
//#define DEBUG_PUBLISHER_BUFFERS


//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif

// Initialize the singleton
Publisher* Publisher::instance = NULL;

//------------------------------------------------------------------------------
// Publisher* Instance(void)
//------------------------------------------------------------------------------
Publisher* Publisher::Instance()
{
   if (instance == NULL)
      instance = new Publisher;
   return instance;
}

//------------------------------------------------------------------------------
// Publisher(void)
//------------------------------------------------------------------------------
Publisher::Publisher() :
   providerId          (-1),
   currProviderId      (-1),
   runState            (Gmat::IDLE),
   maneuvering         (false),
   internalCoordSystem (NULL),
   dataCoordSystem     (NULL),
   dataMJ2000EqOrigin  (NULL) //,
   // Buffers used to avoid publishing repeated data
   //   dataBuffer          (NULL),
   //   countBuffer         (0)
{
}

//------------------------------------------------------------------------------
// ~Publisher(void)
//------------------------------------------------------------------------------
Publisher::~Publisher()
{
   subscriberList.clear();
   coordSysMap.clear();
   
   // Clear registered pub data
   std::map<GmatBase*, std::vector<DataType>* >::iterator iter = providerMap.begin();
   while (iter != providerMap.end())
   {
      std::vector<DataType>* dataList = iter->second;
      if (dataList != NULL)
      {
         #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Remove
            (dataList, "dataList", "Publisher::UnregisterPublishedData()", "deleting dataList");
         #endif
         
         delete dataList;
      }
      iter++;
   }
   // Buffers used to avoid publishing repeated data
   //   if (dataBuffer)
   //      delete [] dataBuffer;
}

//------------------------------------------------------------------------------
// bool Subscribe(Subscriber *s)
//------------------------------------------------------------------------------
bool Publisher::Subscribe(Subscriber *s)
{
   #if DBGLVL_PUBLISHER_SUBSCRIBE
   MessageInterface::ShowMessage
      ("Publisher::Subscribe() sub = <%p><%s>'%s'\n", s, s->GetTypeName().c_str(),
       s->GetName().c_str());
   #endif
   
   if (!s)
      return false;
   
   if (s->GetType() != Gmat::SUBSCRIBER)
   {
      MessageInterface::ShowMessage
         ("**** ERROR **** Publisher::Subscribe() Cannot add non-Subscriber object "
          "'%s'. It is type of '%s'\n", s->GetName().c_str(), s->GetTypeName().c_str());
      return false;
   }
   
   // Add if subscriber is already not in the list (LOJ: 2009.04.08)
   if (find(subscriberList.begin(), subscriberList.end(), s) == subscriberList.end())
   {
      subscriberList.push_back(s);
      s->SetProviderId(currProviderId);
      #if DBGLVL_PUBLISHER_SUBSCRIBE
      MessageInterface::ShowMessage
         ("   Adding <%p>'%s' to subscriber list\n", s, s->GetName().c_str());
      ShowSubscribers();
      #endif
   }
   else
   {
      #if DBGLVL_PUBLISHER_SUBSCRIBE
      MessageInterface::ShowMessage
         ("   <%p>'%s' was already added\n", s, s->GetName().c_str());
      #endif      
   }
   
   return true;
}

//------------------------------------------------------------------------------
// bool Unsubscribe(Subscriber *s)
//------------------------------------------------------------------------------
bool Publisher::Unsubscribe(Subscriber *s)
{
   SetIntegrationStepConsumer(s, false);
   if (!s)
      return false;
   
   if (subscriberList.empty())
   {
      #if DBGLVL_PUBLISHER_SUBSCRIBE
      MessageInterface::ShowMessage
         ("Publisher::Unsubscribe() sub = <%p>'%s' returning false, "
          "the subscriber list is empty\n", s, s->GetName().c_str());
      #endif
      return false;
   }
   
   #if DBGLVL_PUBLISHER_SUBSCRIBE
   MessageInterface::ShowMessage
      ("Publisher::Unsubscribe() sub = <%p>'%s'\n", s, s->GetName().c_str());
   #endif
   
   #if DBGLVL_PUBLISHER_SUBSCRIBE
   MessageInterface::ShowMessage("   About to remove <%p> from the list\n", s);
   ShowSubscribers();
   #endif
   
   subscriberList.remove(s);
   
   #if DBGLVL_PUBLISHER_SUBSCRIBE
   MessageInterface::ShowMessage("Publisher::Unsubscribe() returning true\n");
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
// bool UnsubscribeAll()
//------------------------------------------------------------------------------
bool Publisher::UnsubscribeAll()
{
   #if DBGLVL_PUBLISHER_SUBSCRIBE
   MessageInterface::ShowMessage
      ("Publisher::UnsubscribeAll() entered. Clearing %d subscribers\n",
       subscriberList.size());
   #endif
   
   subscriberList.clear();
   integrationStepConsumers.clear();
   
   ClearPublishedData();
   
   // delete locally created coordinate systems
   std::map<std::string, CoordinateSystem*>::iterator iter;
   for (iter = coordSysMap.begin(); iter != coordSysMap.end(); ++iter)
   {
      if ((iter->first).find("Local-") != std::string::npos)
      {
         #if DBGLVL_PUBLISHER
         MessageInterface::ShowMessage
            ("Publisher::UnsubscribeAll() deleting %s\n", iter->first.c_str());
         #endif
         
         delete iter->second;
      }
   }
   
   coordSysMap.clear();
   return true;
}


//------------------------------------------------------------------------------
// bool Publish(GmatBase *provider, Integer id, Real *data, Integer count, ...)
//------------------------------------------------------------------------------
/**
 * This method is called from the propagation enabled command to publish data
 * so that subscribers can receive data.
 *
 * @param provider The command who is calling this method
 * @param id       The id given by the Publisher
 * @param data     The real type data to publish
 * @param count    Number of data points to publish
 * @param propDir  The direction of propagation (1.0 = forward, -1.0 = backward,
 *
 * Settings to only publish to ephems in step mode.  Remove these cases to
 * implement GMT-6110
 *                                               2.0 = forward, ephems only,
 *                                               -2.0 = backwards, ephems only)
 */
//------------------------------------------------------------------------------
bool Publisher::Publish(GmatBase *provider, Integer id, Real *data, Integer count,
                        Real propDir)
{
   EpochDispatch precision(data, count);
   IntegrationDispatch integration(data, count);
   #if DBGLVL_PUBLISHER_PUBLISH
   MessageInterface::ShowMessage
      ("Publisher::Publish(Real) entered, provider=<%p><%s>, id=%d, "
       "currProviderId=%d, count=%d, propDir=%s\n", provider, provider->GetTypeName().c_str(),
       id, currProviderId, count, propDir == 1.0 ? "forward" : "backward");
   MessageInterface::ShowMessage("   providerMap.size()=%d\n", providerMap.size());
   #endif

   //   // Trap and do not publish identical data twice in a row
   //   bool isRepeated = false;
   //
   // The commented code here ensures that repeated data is not published,  This
   // is turned off in the current implementation
   //   if (count != 0)
   //   {
   //      if (countBuffer != count)
   //      {
   //         if (dataBuffer)
   //            delete [] dataBuffer;
   //         dataBuffer = new Real[count];
   //         countBuffer = count;
   //         memcpy(dataBuffer, data, count*sizeof(Real));
   //      }
   //      else
   //      {
   //         if (memcmp(data, dataBuffer, count*sizeof(Real)) == 0)
   //            isRepeated = true;
   //         else
   //            memcpy(dataBuffer, data, count*sizeof(Real));
   //      }
   //   }
   //
   //   if (!isRepeated)
   //   {

      if ((count > 0) && (data[0] >= 6116.0) && (data[0] <= 58127.5))
         GmatGlobal::Instance()->SetCurrentEpoch(data[0]);

      // No subscribers
      if (subscriberList.empty())
      {
         #if DBGLVL_PUBLISHER_PUBLISH
         MessageInterface::ShowMessage
            ("*** WARNING *** Publisher::Publish() There are no subscribers, "
             "so just returning false\n");
         #endif
         return false;
      }

      // Check for no providers
      std::map<GmatBase*, std::vector<DataType>* >::iterator iter = providerMap.find(provider);
      if (iter == providerMap.end())
      {
         #if DBGLVL_PUBLISHER_PUBLISH
         MessageInterface::ShowMessage
            ("*** WARNING *** Publisher::Publish() There are no registered providers, "
             "so just returning false\n");
         #endif
         return false;
      }

      if (id != currProviderId)
      {
         currProviderId = id;
         UpdateProviderId(id);
      }


      // Get data labels
      std::vector<DataType>* dataList = iter->second;
   
      #if DBGLVL_PUBLISHER_PUBLISH > 1
      MessageInterface::ShowMessage("   dataList->size()=%d\n", dataList->size());
      #endif

      // Convert the data into a string for distribution
      Integer length = count*25 + 1;
      char *stream = new char[length];
   
      #ifdef DEBUG_PUBLISHER_BUFFERS
         MessageInterface::ShowMessage("Allocated %d chars at %p\n", length,
               stream);
      #endif
   
      stream[0] = '\0';    // Init to empty string

      for (Integer i = 0; i < count; ++i)
      {
         #ifdef DEBUG_PUBLISHER_BUFFERS
            MessageInterface::ShowMessage("   %d: %12lf\n", i, data[i]);
         #endif
         sprintf(stream, "%s%16le", stream, data[i]);
         if (i < count - 1)
            strcat(stream, ", ");
         else
            strcat(stream, "\n");
         #ifdef DEBUG_PUBLISHER_BUFFERS
               MessageInterface::ShowMessage("   used %d\n", strlen(stream));
         #endif
      }
   
      #ifdef DEBUG_PUBLISHER_BUFFERS
         MessageInterface::ShowMessage("   Data:  %s\n", stream);
      #endif
   
      #if DBGLVL_PUBLISHER_PUBLISH
      MessageInterface::ShowMessage
         ("Publisher::Publish() calling ReceiveData() number of subsbribers = %d\n",
          subscriberList.size());
      #endif
      
      // Code to only publish to ephems in step mode.  Remove this to
      // implement GMT-6110
      bool ephemsOnly = ((propDir == 2.0) || (propDir == -2.0));

      std::list<Subscriber*>::iterator current = subscriberList.begin();
      while (current != subscriberList.end())
      {
         #ifdef DEBUG_PUBLISHER_BUFFERS
            MessageInterface::ShowMessage("   Publishing to %s\n",
                  (*current)->GetName().c_str());
         #endif

         #if DBGLVL_PUBLISHER_PUBLISH > 1
         MessageInterface::ShowMessage
            ("Publisher::Publish() sub = <%p><%s>'%s'\n", (*current),
             (*current)->GetTypeName().c_str(), (*current)->GetName().c_str());
         #endif

         // Set labels
         #if DBGLVL_PUBLISHER_PUBLISH > 1
         MessageInterface::ShowMessage
            ("Publisher::Publish() Setting data labels:\n");
         StringArray dataLabels = (*dataList)[id].labels;
         for (unsigned int ii = 0; ii < dataLabels.size(); ii++)
            MessageInterface::ShowMessage("%s ", dataLabels[ii].c_str());
         MessageInterface::ShowMessage("\n");
         #endif

         // Code to only publish to ephems in step mode.  Remove this to
         // implement GMT-6110
         const bool integrationConsumer = integrationStepConsumers.count(*current) &&
               GetPublishedIntegrationSteps(data, count);
         if (ephemsOnly && !((*current)->IsOfType(Gmat::EPHEMERIS_FILE)) && !integrationConsumer)
         {
            ++current;
            continue;
         }

         (*current)->SetDataLabels((*dataList)[id].labels);

         // Set provider
         if (count > 0)
            (*current)->SetProvider(provider, data[0]);
         else
            (*current)->SetProvider(provider);

         // Set propagation direction
         (*current)->SetPropagationDirection((propDir > 0.0 ? 1.0 : -1.0));

         if (!(*current)->ReceiveData(stream))
            return false;
         if (!(*current)->ReceiveData(data, count))
            return false;
         current++;
      }

      #ifdef DEBUG_PUBLISHER_BUFFERS
         MessageInterface::ShowMessage("   Cleaning up\n");
      #endif
      delete [] stream;
      //   }  End of the repeated data check block

   #if DBGLVL_PUBLISHER_PUBLISH
   MessageInterface::ShowMessage("Publisher::Publish() returning true\n");
   #endif
   return true;
}


//------------------------------------------------------------------------------
// bool Publish(char *data, Integer count = 0)
//------------------------------------------------------------------------------
bool Publisher::Publish(Integer id, char *data, Integer count)
{
   EpochDispatch precision(NULL, 0);
   IntegrationDispatch integration(NULL, 0);
   Integer i;
    
   // No subscribers
   if (subscriberList.empty())
      return false;

   #if DBGLVL_PUBLISHER_PUBLISH > 1
   MessageInterface::ShowMessage("Publisher::Publish(char) id = %d\n", id);
   #endif
   
   if ((id < 0) || (id > providerId))
   {
      throw PublisherException
         ("Character data provider has not registered with the Publisher.");
   }
   
   if (id != currProviderId)
   {
      currProviderId = id;
      UpdateProviderId(id);
   }
   
   // Convert the data into a string for distribution
   Integer length;
   if (count)
        length = count + 1;
   else
        length = strlen(data) + 1;
        
   char *stream = new char[length];

   if (count)
   {
      for (i = 0; i < count; ++i)
         stream[i] = data[i];
      stream[i] = '\0';
   }
   else
   {
      strcpy(stream, data);
   }
   
   strcat(stream, "\n");

   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      if (!(*current)->ReceiveData(stream))
         return false;
      current++;
   }

   delete [] stream;
   return true;
}

//------------------------------------------------------------------------------
// bool Publish(Integer id, Integer *data, Integer count = false)
//------------------------------------------------------------------------------
bool Publisher::Publish(Integer id, Integer *data, Integer count)
{
   EpochDispatch precision(NULL, 0);
   IntegrationDispatch integration(NULL, 0);
   // No subscribers
   if (subscriberList.empty())
      return false;
   
   #if DBGLVL_PUBLISHER_PUBLISH > 1
   MessageInterface::ShowMessage("Publisher::Publish(Integer) id = %d\n", id);
   #endif
   
   if ((id < 0) || (id > providerId))
      throw PublisherException
         ("Integer data provider has not registered with the Publisher.");
   
   if (id != currProviderId)
   {
      currProviderId = id;
      UpdateProviderId(id);
   }
   
   // Convert the data into a string for distribution
   char *stream = new char[count*25 + 1];
   stream[0] = '\0';
   
   for(Integer i = 0; i < count; ++i)
   {
      sprintf(stream, "%s%d", stream, data[i]);
      if (i < count - 1)
         strcat(stream, ", ");
      else
         strcat(stream, "\n");
   }
   
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      if (!(*current)->ReceiveData(stream))
         return false;
      current++;
   }

   delete [] stream;
   return true;
}


//------------------------------------------------------------------------------
// void Ping()
//------------------------------------------------------------------------------
/**
 * Pings the system so that GUI overrides can hand control off for messages.
 */
//------------------------------------------------------------------------------
void Publisher::Ping()
{
}


//------------------------------------------------------------------------------
// bool FlushBuffers(bool endOfDataBlock = true)
//------------------------------------------------------------------------------
/**
 * Indicates end of data stream.
 */
//------------------------------------------------------------------------------
bool Publisher::FlushBuffers(bool endOfDataBlock)
{
   // No subscribers
   if (subscriberList.empty())
      return false;
   
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      if (!(*current)->FlushData(endOfDataBlock))
         return false;
      current++;
   }
   
   return true;
}


//------------------------------------------------------------------------------
// bool NotifyEndOfRun()
//------------------------------------------------------------------------------
bool Publisher::NotifyEndOfRun()
{
   // No subscribers
   if (subscriberList.empty())
      return false;
   
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      if (!(*current)->SetEndOfRun())
         return false;
      current++;
   }
   
   return true;
}


//------------------------------------------------------------------------------
// const std::list<Subscriber*> GetSubscriberList()
//------------------------------------------------------------------------------
const std::list<Subscriber*> Publisher::GetSubscriberList()
{
   return subscriberList;
}


//------------------------------------------------------------------------------
// void ClearPublishedData()
//------------------------------------------------------------------------------
void Publisher::ClearPublishedData()
{   
   #if DBGLVL_PUBLISHER_CLEAR
   MessageInterface::ShowMessage
      ("Publisher::ClearPublishedData() entered, clearing %d element owner objects\n",
       objectArray.size());
   MessageInterface::ShowMessage("   providerMap.size()=%u\n", providerMap.size());
   MessageInterface::ShowMessage("   subscriberList.size()=%u\n", subscriberList.size());
   #endif
   
   objectArray.clear();
   elementArray.clear();
   providerId = -1;
   currProviderId = -1;
   
   // Clear subscriber's data
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      SetIntegrationStepConsumer(*current, false);
      (*current)->ClearDataLabels();
      current++;
   }
   
   #if DBGLVL_PUBLISHER_CLEAR > 1
   MessageInterface::ShowMessage
      ("Publisher::ClearPublishedData() Using new Publisher code\n");
   #endif
   
   std::map<GmatBase*, std::vector<DataType>* >::iterator iter = providerMap.begin();
   while (iter != providerMap.end())
   {
      std::vector<DataType>* dataList = iter->second;
      if (dataList != NULL)
      {
         #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Remove
            (dataList, "dataList", "Publisher::ClearPublishedData()", "deleting dataList");
         #endif
         
         delete dataList;
      }
      iter++;
   }
   
   providerMap.clear();
   subscriberList.clear();
   
   #if DBGLVL_PUBLISHER_CLEAR
   MessageInterface::ShowMessage
      ("Publisher::ClearPublishedData() leaving, providerMap.size()=%u\n",
       providerMap.size());
   #endif
}


//------------------------------------------------------------------------------
// Integer RegisterPublishedData(GmatBase *provider, Integer id,
//         const StringArray& owners, const StringArray& elements)
//------------------------------------------------------------------------------
/*
 * Registers provider with data elements. This method passes elements as data
 * labels to subscribers by calling SetDataLabels(elements).
 *
 * @param  provider  Provider who calls publiser
 * @param  id        Provider id, if id is -1 it will assign new id
 * @param  owners    Object names, such as spacecraft name
 * @param  elements  Element names, such as Sat1.X, Sat1.Y
 *
 * @return  new provider id or existing id
 *
 * @note Currently only Propagate command registers and publishes orbit
 *       trajectory data. The owner is not currently used since elements has owner
 *       name, such as Sat1.X which Sat1 is the owner name.
 */
//------------------------------------------------------------------------------
Integer Publisher::RegisterPublishedData(GmatBase *provider, Integer id,
                                         const StringArray& owners, 
                                         const StringArray& elements)
{
   #if DBGLVL_PUBLISHER_REGISTER
   MessageInterface::ShowMessage
      ("Publisher::RegisterPublishedData() entered, <%p><%s> passed id: %d, %d "
       "owners and %d elements\n", provider, provider->GetTypeName().c_str(), id,
       owners.size(), elements.size());
   MessageInterface::ShowMessage("   providerMap.size()=%d\n", providerMap.size());
   #endif
   
   #if DBGLVL_PUBLISHER_REGISTER > 1
   for (unsigned int i=0; i<owners.size(); i++)
      MessageInterface::ShowMessage("   owner[%d]= <%s>  elements[%d] = <%s>\n", 
         i, owners[i].c_str(), i, elements[i].c_str());
   MessageInterface::ShowMessage
      ("   providerId=%d, subscriberList.size()=%d\n", providerId, subscriberList.size());
   #endif
   
   if (subscriberList.empty())
   {
      // Let's just show warning if debug is turned on
      //throw PublisherException("There are no registered subscribers.");
      #if DBGLVL_PUBLISHER_REGISTER
      MessageInterface::ShowMessage
         ("*** WARNING *** Publisher::RegisterPublishedData() There are no "
          "subscribers to register data\n");
      MessageInterface::ShowMessage
         ("Publisher::RegisterPublishedData() returning %d\n", providerId);
      #endif
      return providerId;
   }
   
   Integer actualId = -1;
   
   #if DBGLVL_PUBLISHER_REGISTER > 1
   MessageInterface::ShowMessage
      ("Publisher::RegisterPublishedData() Using new Publisher code\n");
   #endif
   
   if (id != -1)
   {
      #if DBGLVL_PUBLISHER_REGISTER
      MessageInterface::ShowMessage
         ("Publisher::RegisterPublishedData() returning %d, provider already has "
          "registered with data\n", id);
      #endif
      
      // Just set current run state
      std::list<Subscriber*>::iterator current = subscriberList.begin();
      while (current != subscriberList.end())
      {
         #if DBGLVL_PUBLISHER_REGISTER > 1
         MessageInterface::ShowMessage
            ("   calling <%s>->SetRunState()\n", (*current)->GetName().c_str());
         #endif
         
         (*current)->SetRunState(runState);
         current++;
      }
      return id;
   }
   
   // Add publishing data object names
   std::map<GmatBase*, std::vector<DataType>* >::iterator iter =
      providerMap.find(provider);
   if (!owners.empty())
   {
      //@tbd Should it be cleared first? (LOJ: 2009.11.04)
      objectArray.clear();
      objectArray.push_back(StringArray(owners));
   }
   
   // @note
   // New Publisher code doesn't use global providerId anymore. It keeps
   // track of provider's data labels and ids and copies corresponding
   // labels to current data labels before distributing the data in Publish().
   // There was an issue with provider id being kept incrementing if data is
   // registered and published inside a GmatFunction
   if (!elements.empty())
   {
      if (iter == providerMap.end())
      {
         #if DBGLVL_PUBLISHER_REGISTER
         MessageInterface::ShowMessage("==> provider not found\n");
         #endif
         
         // create new dataList
         actualId = 0;
         DataType newData(elements, actualId);
         
         std::vector<DataType>* dataList = new std::vector<DataType>;
         
         #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Add
            (dataList, "dataList", "Publisher::RegisterPublishedData()", "creating dataList");
         #endif
         
         dataList->push_back(newData);
         providerMap.insert(std::make_pair(provider, dataList));
      }
      else
      {
         #if DBGLVL_PUBLISHER_REGISTER
         MessageInterface::ShowMessage("==> provider found\n");
         #endif
         
         std::vector<DataType>* oldList = iter->second;
         actualId = oldList->size();
         DataType newData(elements, actualId);
         oldList->push_back(newData);
      }
   }
   
   #if DBGLVL_PUBLISHER_REGISTER > 1
   std::map<GmatBase*, std::vector<DataType>* >::iterator iter2 = providerMap.find(provider);
   std::vector<DataType>* tempList = iter2->second;
   MessageInterface::ShowMessage
      ("   actualId=%d, tempList->size() = %d\n", actualId, tempList->size());
   #endif
   
   providerId = actualId;
   
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      #if DBGLVL_PUBLISHER_REGISTER > 1
      MessageInterface::ShowMessage
         ("   calling <%s>->SetDataLabels()\n", (*current)->GetName().c_str());
      #endif
      
      (*current)->SetDataLabels(elements);
      (*current)->SetRunState(runState);
      current++;
   }
   
   #if DBGLVL_PUBLISHER_REGISTER
   MessageInterface::ShowMessage("   providerMap.size()=%d\n", providerMap.size());
   MessageInterface::ShowMessage
      ("Publisher::RegisterPublishedData() provider=<%p> returning %d\n",
       provider, actualId);
   #endif
   
   return actualId;
}


//------------------------------------------------------------------------------
// UnregisterPublishedData(GmatBase *provider)
//------------------------------------------------------------------------------
void Publisher::UnregisterPublishedData(GmatBase *provider)
{
   #if DBGLVL_PUBLISHER_REGISTER
   MessageInterface::ShowMessage
      ("Publisher::UnregisterPublishedData() entered, <%p><%s>, providerMap.size() = %d\n",
       provider, provider->GetTypeName().c_str(), providerMap.size());
   #endif
   
   std::map<GmatBase*, std::vector<DataType>* >::iterator iter = providerMap.find(provider);
   if (iter != providerMap.end())
   {
      std::vector<DataType>* dataList = iter->second;
      if (dataList != NULL)
      {
         #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Remove
            (dataList, "dataList", "Publisher::UnregisterPublishedData()", "deleting dataList");
         #endif
         
         delete dataList;
      }
      providerMap.erase(iter);
   }
   else
   {
      #if DBGLVL_PUBLISHER_REGISTER
      MessageInterface::ShowMessage
         ("*** WARNING *** Publisher::UnregisterPublishedData() provider <%p> "
          "not registered\n");
      #endif
   }
   
   #if DBGLVL_PUBLISHER_REGISTER
   MessageInterface::ShowMessage
      ("Publisher::UnregisterPublishedData() leaving, providerMap.size() = %d\n",
       providerMap.size());
   #endif
   
}


//------------------------------------------------------------------------------
// const StringArray& GetStringArrayParameter(const std::string& type)
//------------------------------------------------------------------------------
const StringArray& Publisher::GetStringArrayParameter(const std::string& type)
{
   if ((currProviderId < 0) || (currProviderId > providerId))
      throw PublisherException("Data provider id out of range.");
   
   if (type == "SpaceObjectMap")
      return objectArray[currProviderId];
   
   if (type == "PublishedDataMap")
      return elementArray[currProviderId];
   
   throw PublisherException("Unknown StringArray type requested.");
}


//------------------------------------------------------------------------------
// void SetInternalCoordSystem(CoordinateSystem *cs)
//------------------------------------------------------------------------------
/*
 * Sets the internal coordinate system.
 *
 * @parameter  cs  Internal coordinate system (usally EarthMJ2000Eq)
 */
//------------------------------------------------------------------------------
void Publisher::SetInternalCoordSystem(CoordinateSystem *cs)
{
   if (cs != NULL)
      internalCoordSystem = cs;
}


//------------------------------------------------------------------------------
// void SetDataCoordSystem(CoordinateSystem *cs)
//------------------------------------------------------------------------------
/*
 * Sets the coordinate system of data to subscribers. It also adds the coordinate
 * system to the map, if it is not found.
 *
 * @parameter  cs  Coordinate system which data represents
 */
//------------------------------------------------------------------------------
void Publisher::SetDataCoordSystem(CoordinateSystem *cs)
{
   if (cs == NULL)
      return;
   
   dataCoordSystem = cs;
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetDataCoordSystem(cs);
      current++;
   }
   
   // if coordinate system not found in the map, add
   std::string csName = cs->GetName();
   if (coordSysMap.find(csName) != coordSysMap.end())
      coordSysMap[csName] = cs;
}


//------------------------------------------------------------------------------
// void SetDataMJ2000EqOrigin(CelestialBody *cb)
//------------------------------------------------------------------------------
/*
 * Sets origin of MJ2000Eq coordinate system which data represents. If matching
 * coordinate system name found from the map, it sets to data coordinate system,
 * otherwise, it creates new coordinate system.
 *
 * @parameter  cb  Origin of MJ2000Eq coordinate system which data represents
 */
//------------------------------------------------------------------------------
void Publisher::SetDataMJ2000EqOrigin(CelestialBody *cb)
{
   if (cb == NULL)
      return;
   
   #if DBGLVL_PUBLISHER_DATA_REP
   MessageInterface::ShowMessage
      ("Publisher::SetDataMJ2000EqOrigin() cb=%s<%p>\n", cb->GetName().c_str(),
       cb);
   #endif
   
   dataMJ2000EqOrigin = cb;
   std::string originName = cb->GetName();
   std::string csName = originName + "MJ2000Eq";
      
   std::map<std::string, CoordinateSystem*>::iterator csIter =
      coordSysMap.find(csName);
   
   if (coordSysMap.find(csName) != coordSysMap.end())
   {
      // set coordinate system from the map
      dataCoordSystem = coordSysMap.find(csName)->second;
   }
   else
   {
      // check as local name
      csName = "Local-" + csName;
      if (coordSysMap.find(csName) != coordSysMap.end())
         dataCoordSystem = coordSysMap.find(csName)->second;
      else
      {
         // Create coordinate system if not exist
         CoordinateSystem *newCs = (CoordinateSystem*)internalCoordSystem->Clone();
         newCs->SetName(csName);
         newCs->SetStringParameter("Origin", originName);
         newCs->SetRefObject(cb, Gmat::CELESTIAL_BODY, originName);
         newCs->Initialize();
         coordSysMap[csName] = newCs;
         dataCoordSystem = newCs;
         
         #if DBGLVL_PUBLISHER_DATA_REP
         MessageInterface::ShowMessage
            ("   ===> %s not found in the map, so created <%p>\n", csName.c_str(),
             newCs);
         #endif
      }
   }
   
   // set to subscribers
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetDataMJ2000EqOrigin(cb);
      (*current)->SetDataCoordSystem(dataCoordSystem);
      current++;
   }
}


//------------------------------------------------------------------------------
// void SetRunState(const Gmat::RunState state)
//------------------------------------------------------------------------------
void Publisher::SetRunState(const Gmat::RunState state)
{
   #ifdef DEBUG_PUBLISHER_RUN_STATE
   MessageInterface::ShowMessage
      ("Publisher::SetRunState() entered, setting run state %d to "
       "%d subscribers\n", state, subscriberList.size());
   #endif
   
   runState = state;
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      #ifdef DEBUG_PUBLISHER_RUN_STATE
      MessageInterface::ShowMessage("   current=<%p>\n", *current);
      #endif
      
      (*current)->SetRunState(runState);
      current++;
   }
   
   #ifdef DEBUG_PUBLISHER_RUN_STATE
   MessageInterface::ShowMessage("Publisher::SetRunState() exiting\n");
   #endif
}


//------------------------------------------------------------------------------
// void SetManeuvering(GmatBase *originator, bool flag, Real epoch,
//                     const std::string &satName, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Sets single spacecraft maneuvering flag.
 *
 * @param originator  The maneuver command pointer who is maneuvering
 * @param flag  Set to true if maneuvering
 * @param epoch  Epoch of maneuver
 * @param satName  Name of the maneuvering spacecraft
 * @param desc  Description of maneuver (e.g. impulsive or finite)
 */
//------------------------------------------------------------------------------
void Publisher::SetManeuvering(GmatBase *originator, bool flag, Real epoch,
                               const std::string &satName,
                               const std::string &desc)
{
   maneuvering = flag;
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetManeuvering(originator, flag, epoch, satName, desc);
      current++;
   }
}


//------------------------------------------------------------------------------
// void SetManeuvering(GmatBase *originator, bool flag, Real epoch,
//                     const StringArray &satNames, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Sets multiple spacecrafts maneuvering flag.
 * 
 * @param originator  The maneuver command pointer who is maneuvering
 * @param flag  Set to true if maneuvering
 * @param epoch  Epoch of maneuver
 * @param satNames  Names of the maneuvering spacecraft
 * @param desc  Description of maneuver (e.g. impulsive or finite)
 */
//------------------------------------------------------------------------------
void Publisher::SetManeuvering(GmatBase *originator, bool flag, Real epoch,
                               const StringArray &satNames,
                               const std::string &desc)
{
   maneuvering = flag;
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetManeuvering(originator, flag, epoch, satNames, desc);
      current++;
   }
}


//------------------------------------------------------------------------------
// bool GetManeuvering()
//------------------------------------------------------------------------------
bool Publisher::GetManeuvering()
{
   return maneuvering;
}


//------------------------------------------------------------------------------
// void SetSpacecraftPropertyChanged(GmatBase *originator, Real epoch,
//                           const std::string &satName, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Sets spacecraft property change so that subscribers can handle appropriately.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param epoch  Epoch of spacecraft at property change
 * @param satName  Name of the spacecraft
 * @param desc  Description of property change
 */
//------------------------------------------------------------------------------
void Publisher::SetSpacecraftPropertyChanged(GmatBase *originator, Real epoch,
                                             const std::string &satName,
                                             const std::string &desc)
{
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetSpacecraftPropertyChanged(originator, epoch, satName, desc);
      current++;
   }
}


//------------------------------------------------------------------------------
// void SetOrbitColorChanged(GmatBase *originator, const std::string &newColor, ...)
//------------------------------------------------------------------------------
/**
 * Sets object orbit color change so that subscribers can handle appropriately.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param newColor  New color to be applied to the object
 * @param objName  Name of the object
 * @param desc  Description of property change
 */
//------------------------------------------------------------------------------
void Publisher::SetOrbitColorChanged(GmatBase *originator,
                                     const std::string &newColor,
                                     const std::string &objName,
                                     const std::string &desc)
{
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetOrbitColorChanged(originator, newColor, objName, desc);
      current++;
   }
}


//------------------------------------------------------------------------------
// void SetTargetColorChanged(GmatBase *originator, const std::string &newColor, ...)
//------------------------------------------------------------------------------
/**
 * Sets object target color change so that subscribers can handle appropriately.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param newColor  New color to be applied to the object
 * @param objName  Name of the object
 * @param desc  Description of property change
 */
//------------------------------------------------------------------------------
void Publisher::SetTargetColorChanged(GmatBase *originator,
                                      const std::string &newColor,
                                      const std::string &objName,
                                      const std::string &desc)
{
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetTargetColorChanged(originator, newColor, objName, desc);
      current++;
   }
}

//------------------------------------------------------------------------------
// void SetSegmentOrbitColor(GmatBase *originator, bool overrideColor,
//                           UnsignedInt orbitColor)
//------------------------------------------------------------------------------
/**
 * Sets propagation segment orbit color so that subscribers can handle appropriately.
 * 
 * @param originator  The Propagate command pointer who is setting
 * @param overrideColor  The flag indicating whether or not to override orbit color
 * @param orbitColor  New orbit color to be applied to the space object
 */
//------------------------------------------------------------------------------
void Publisher::SetSegmentOrbitColor(GmatBase *originator, bool overrideColor,
                                     UnsignedInt orbitColor, const StringArray &objNames)
{
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetSegmentOrbitColor(originator, overrideColor, orbitColor, objNames);
      current++;
   }
}

//------------------------------------------------------------------------------
// void UpdateProviderId(Integer newId)
//------------------------------------------------------------------------------
void Publisher::UpdateProviderId(Integer newId)
{
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      (*current)->SetProviderId(newId);
      current++;
   }
}


//------------------------------------------------------------------------------
// void ShowSubscribers()
//------------------------------------------------------------------------------
void Publisher::ShowSubscribers()
{
   if (subscriberList.empty())
   {
      MessageInterface::ShowMessage("   ===== There are no subscribers\n");
      return;
   }
   
   MessageInterface::ShowMessage
      ("   ===== There are %d subscriber(s)\n", subscriberList.size());
   
   std::list<Subscriber*>::iterator current = subscriberList.begin();
   while (current != subscriberList.end())
   {
      MessageInterface::ShowMessage
         ("   <%p>'%s'\n", *current, (*current)->GetName().c_str());
      current++;
   }
}

