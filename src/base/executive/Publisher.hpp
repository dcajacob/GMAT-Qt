//$Id$
//------------------------------------------------------------------------------
//                                  Publisher
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
// Created: 2003/10/21
/**
 * Definition for the Publisher class.
 */
//------------------------------------------------------------------------------

#ifndef Publisher_hpp
#define Publisher_hpp

#include "Subscriber.hpp"
#include "GmatTime.hpp"
#include "Rvector6.hpp"
#include <list>
#include <vector>
#include <map>
#include <array>
#include <memory>


class GMAT_API Publisher
{
public:
   static Publisher*    Instance();
   
   // destructor
   virtual ~Publisher();
   
   bool Subscribe(Subscriber *s);
   bool Unsubscribe(Subscriber *s);
   bool UnsubscribeAll();
   
   virtual bool Publish(GmatBase *provider, Integer id, Real *data, Integer count, Real propDir = 1.0);
   // Optional precise timestamp for this synchronous Real packet only. Existing
   // Publish overloads and subscriber ABI remain unchanged.
   bool PublishWithEpoch(GmatBase *provider, Integer id, Real *data, Integer count,
                         const GmatTime &epoch, Real propDir = 1.0);
   static bool GetPublishedEpoch(const Real *data, Integer count, GmatTime &epoch);
   struct CartesianSample
   {
      const GmatBase *object;
      GmatTime epoch;
      Rvector6 state;
   };
   bool PublishWithSamples(GmatBase *provider, Integer id, Real *data,
                           Integer count, const GmatTime &sourceEpoch,
                           const std::vector<CartesianSample> &samples,
                           Real propDir = 1.0);
   static bool GetPublishedCartesianState(const Real *data, Integer count,
      const GmatBase *object, GmatTime &epoch, Rvector6 &state);
   // Optional accepted integration endpoints for this synchronous publication.
   // Existing packets, virtual dispatch and Subscriber ABI remain unchanged.
   struct IntegrationStep
   {
      bool initial = false;
      // True for a committed ephemeris-provider advance (no integration).
      bool ephemeris = false;
      Real startSeconds = 0.0;
      Real endSeconds = 0.0;
      std::string method;
      GmatTime epoch;
      std::vector<CartesianSample> states;
      // Optional native command-start equivalents of numerical endpoints.
      // Used only to recognize continuous Real-clock restarts; display and
      // scientific consumers retain the precise Cartesian samples above.
      std::vector<CartesianSample> restartStates;
   };
   static void SetIntegrationStepConsumer(Subscriber *subscriber, bool enabled);
   bool HasIntegrationStepConsumers() const;
   bool PublishWithIntegrationSteps(GmatBase *provider, Integer id, Real *data,
      Integer count, const std::vector<IntegrationStep> &steps, Real propDir = 1.0);
   static const std::vector<IntegrationStep>* GetPublishedIntegrationSteps(
      const Real *data, Integer count);

   // Identity and command-start flag for the current ordinary Propagate packet.
   // This lightweight context does not subscribe the caller to integration steps.
   struct CartesianColumns
   {
      const GmatBase *object;
      std::array<Integer, 6> indices;
      // Zero means the optional field is absent from this packet.
      std::array<Integer, 21> covariance{};
      std::array<Integer, 3> acceleration{};
      std::array<Integer, 4> quaternion{};
      Integer propagationGroup = -1;
      std::string propagatorName;
      std::weak_ptr<const void> propagatorIdentity;
      bool hasEpoch = false;
      GmatTime epoch;
      CoordinateSystem *frame = NULL;
      std::weak_ptr<const void> frameIdentity;
   };
   bool PublishPropagatePacket(GmatBase *provider, Integer id, Real *data,
      Integer count, Real direction, const ObjectArray &objects, bool initial,
      const std::vector<IntegrationStep> *steps = NULL,
      const std::vector<CartesianColumns> *columns = NULL);
   static const ObjectArray *GetPublishedPropagationObjects(const Real *data,
      Integer count, bool &initial);

   // Bind packet columns to the actual propagated object. Function arguments
   // may rename spacecraft, and distinct retained objects may share a name.
   static const std::vector<CartesianColumns> *GetPublishedCartesianColumns(
      const Real *data, Integer count);
   static const std::vector<CartesianColumns> *GetPublishedCartesianColumns(
      const GmatBase *provider);
   static bool GetPublishedSpacecraftEpoch(const Real *data, Integer count,
      const GmatBase *object, GmatTime &epoch);
   static CoordinateSystem *GetPublishedSpacecraftFrame(const Real *data, Integer count,
      const GmatBase *object);
   struct EpochColumns
   {
      GmatTime epoch;
      std::vector<CartesianColumns> columns;
   };
   static std::vector<EpochColumns> GetPublishedEpochGroups(const Real *data, Integer count,
      const ObjectArray &objects);
   bool PublishWithSpacecraftColumns(GmatBase *provider, Integer id, Real *data,
      Integer count, const std::vector<CartesianColumns> &columns, Real direction = 1.0,
      const GmatTime *epoch = NULL, const std::vector<CartesianSample> *samples = NULL);
   // A plot's compact accepted-step packet gets its own column layout while
   // preserving the original integration/origin metadata for frame conversion.
   class GMAT_API CartesianColumnScope
   {
   public:
      CartesianColumnScope(const Real *data, Integer count,
         const std::vector<CartesianColumns> &columns);
      ~CartesianColumnScope();
      CartesianColumnScope(const CartesianColumnScope &) = delete;
      CartesianColumnScope &operator=(const CartesianColumnScope &) = delete;
   private:
      const Real *previousData;
      Integer previousCount;
      const std::vector<CartesianColumns> *previousColumns;
      const GmatBase *previousProvider;
      const Real *previousRootData;
      Integer previousRootCount;
      const std::vector<CartesianColumns> *previousRootColumns;
      const std::vector<CartesianColumns> frozen;
   };

   // Only a report's wrapper evaluation opts into sampled Cartesian/time
   // values. Other subscribers and the live spacecraft retain source data.
   class GMAT_API ParameterStateScope
   {
   public:
      ParameterStateScope(const Real *data, Integer count);
      ~ParameterStateScope();
      ParameterStateScope(const ParameterStateScope &) = delete;
      ParameterStateScope &operator=(const ParameterStateScope &) = delete;
   private:
      const void *previous;
   };
   static bool GetParameterCartesianState(const GmatBase *object,
      GmatTime &epoch, Rvector6 &state);
   virtual bool Publish(Integer id, char *data, Integer count = 0);
   virtual bool Publish(Integer id, Integer *data, Integer count);
   virtual void Ping();
   
   bool FlushBuffers(bool endOfDataBlock = true);
   bool NotifyEndOfRun();
   
   const std::list<Subscriber*> GetSubscriberList();
   
   // Interface methods used to identify the data sent to the publisher and
   // subscribers
   Integer              RegisterPublishedData(GmatBase *provider, Integer id,
                                              const StringArray& owners,
                                              const StringArray& elements);
   void                 UnregisterPublishedData(GmatBase *provider);
   void                 ClearPublishedData();
   const StringArray&   GetStringArrayParameter(const std::string& type);
   void                 SetInternalCoordSystem(CoordinateSystem *cs);
   void                 SetDataCoordSystem(CoordinateSystem *cs);
   void                 SetDataMJ2000EqOrigin(CelestialBody *cb);
   
   // Interfaces used to update the state of the running system
   void                 SetRunState(const Gmat::RunState state);
   
   void                 SetManeuvering(GmatBase *originator,
                                       bool flag, Real epoch,
                                       const std::string &satNames,
                                       const std::string &desc);
   void                 SetManeuvering(GmatBase *originator,
                                       bool flag, Real epoch,
                                       const StringArray &satNames,
                                       const std::string &desc);
   bool                 GetManeuvering();
   struct ManeuveringState
   {
   private:
      friend class Publisher;
      struct Entry
      {
         Subscriber *object;
         std::weak_ptr<const void> identity;
         bool active;
      };
      const Publisher *owner = nullptr;
      bool active = false;
      std::vector<Entry> subscribers;
   };
   ManeuveringState CaptureManeuveringState() const;
   std::unique_ptr<NativeRestorationPlan> PrepareManeuveringRestoration(
         const ManeuveringState &source);
   
   void                 SetSpacecraftPropertyChanged(GmatBase *originator, Real epoch,
                                                     const std::string &satName,
                                                     const std::string &desc);
   void                 SetOrbitColorChanged(GmatBase *originator,
                                             const std::string &newColor,
                                             const std::string &objName,
                                             const std::string &desc);
   void                 SetTargetColorChanged(GmatBase *originator,
                                              const std::string &newColor,
                                              const std::string &objName,
                                              const std::string &desc);
   void                 SetSegmentOrbitColor(GmatBase *originator,
                                             bool overrideColor,
                                             UnsignedInt orbitColor,
                                             const StringArray &objNames);

   // Identity-bearing overloads preserve the existing subscriber callbacks and
   // exported name-only entry points for external publishers/plugins.
   enum class ObjectEvent { Maneuver, SpacecraftProperty, OrbitColor, TargetColor, SegmentColor };
   static bool IsPublishedEventSubject(ObjectEvent event, const GmatBase *originator,
      const GmatBase *subject, bool &identified);
   void SetManeuvering(GmatBase *originator, bool flag, Real epoch,
      const std::string &name, const std::string &desc, GmatBase *subject);
   void SetManeuvering(GmatBase *originator, bool flag, Real epoch,
      const StringArray &names, const std::string &desc, const ObjectArray &subjects);
   void SetSpacecraftPropertyChanged(GmatBase *originator, Real epoch,
      const std::string &name, const std::string &desc, GmatBase *subject);
   void SetOrbitColorChanged(GmatBase *originator, const std::string &color,
      const std::string &name, const std::string &desc, GmatBase *subject);
   void SetTargetColorChanged(GmatBase *originator, const std::string &color,
      const std::string &name, const std::string &desc, GmatBase *subject);
   void SetSegmentOrbitColor(GmatBase *originator, bool overrideColor,
      UnsignedInt color, const StringArray &names, const ObjectArray &subjects);
   
   CoordinateSystem* GetInternalCoordSystem() { return internalCoordSystem; }
   CoordinateSystem* GetDataCoordSystem() { return dataCoordSystem; }
   CelestialBody* GetDataMJ2000EqOrigin() { return dataMJ2000EqOrigin; }
   inline Gmat::RunState GetRunState();
   
protected:
   /// The singleton
   static Publisher         *instance;
   /// List of the subscribers
   std::list<Subscriber*>   subscriberList;
   /// Index used to identify number of registered data providers
   Integer                  providerId;
   /// ID for the current data provider
   Integer                  currProviderId;
   /// Arrays used to track objects for published data
   std::vector<StringArray> objectArray;
   /// Arrays used to track elements for published data
   std::vector<StringArray> elementArray;
   
   /// State of the system (used to track data for display or suppression)
   Gmat::RunState           runState;
   /// Flag indicating maneuvering
   bool                     maneuvering;
   /// Internal coordinate system
   CoordinateSystem         *internalCoordSystem;
   /// Coordinate system of data
   CoordinateSystem         *dataCoordSystem;
   /// Origin of MJ2000Eq of data
   CelestialBody            *dataMJ2000EqOrigin;
   /// Map of coordinate system of data
   std::map<std::string, CoordinateSystem*> coordSysMap;
   
   //   // Data buffers used to avoid repeated points
   //   /// The data last published
   //   Real *dataBuffer;
   //   /// Count for the last published data
   //   Integer countBuffer;

   /// published data info
   struct DataType
   {
      StringArray    labels;
      Integer        id;
      DataType(StringArray labs, Integer pos)
      {
         labels = labs;
         id = pos;
      };
   };
   
   /// published data map
   std::map<GmatBase*, std::vector<DataType>* > providerMap;
   
   void                 UpdateProviderId(Integer newId);
   
   // for debug
   void                 ShowSubscribers();
   
   // default constructor
   Publisher();
   // assignment operator
   Publisher& operator=(const Publisher &right);
};



inline Gmat::RunState Publisher::GetRunState()
{
   return runState; 
}

#endif // Publisher_hpp
