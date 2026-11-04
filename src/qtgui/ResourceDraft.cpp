#include "ResourceDraft.hpp"
#include "FactoryManager.hpp"
#include "FileManager.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "SolarSystem.hpp"
#include "Array.hpp"
#include "CelestialBody.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "ODEModel.hpp"
#include "CalculatedPoint.hpp"
#include <algorithm>
#include <stdexcept>

QString defaultPropagatorForceModelName(const QString &name)
{
   const auto base=name+"_ForceModel";
   auto candidate=base; int suffix=1;
   while (Moderator::Instance()->GetConfiguredObject(candidate.toStdString()))
      candidate=base+QString::number(suffix++);
   return candidate;
}

bool propagatorUsesForceModel(const QString &type)
{
   const auto selected=type.trimmed().toStdString();
   const auto &types=Moderator::Instance()->GetListOfFactoryItems(Gmat::PROPAGATOR);
   if (std::find(types.begin(),types.end(),selected)==types.end())
      throw std::runtime_error("Select a registered propagator type.");
   QtResourcePreview draft(FactoryManager::Instance()->CreatePropagator(selected,selected));
   auto *propagator=dynamic_cast<Propagator *>(draft.get());
   if (!propagator) throw std::runtime_error("The selected propagator could not be prepared.");
   return propagator->UsesODEModel();
}

QtResourcePreview resourceDraft(const QString &type,const QString &name)
{
   auto *factory=FactoryManager::Instance();
   QtResourcePreview draft;
   auto *files=FileManager::Instance();
   const auto paths=type=="CustomFOV" ? files->CaptureState() : nullptr;
   try {
      draft.reset(factory->CreateObject(factory->GetBaseTypeOf(type.toStdString()),type.toStdString(),name.toStdString()));
   } catch (...) {
      if (paths) files->RestoreState(*paths);
      throw;
   }
   // Selecting a pending FOV must not reset configured search paths, including
   // their absence; registration/defaults belong to the accepted source build.
   if (paths) files->RestoreState(*paths);
   if (!draft) throw std::runtime_error("The runtime cannot prepare this resource type.");
   auto *moderator=Moderator::Instance();
   if (auto *setup=dynamic_cast<PropSetup *>(draft.get())) {
      // Like wx CreateDefaultPropSetup, preview the ordinary Earth/JGM2
      // force model. An empty Moderator name is unregistered; Cancel cannot
      // add a configured object. SetODEModel owns a clone of this preview.
      QtResourcePreview model(moderator->CreateODEModel("ForceModel",""));
      auto *force=dynamic_cast<ODEModel *>(model.get());
      if (!force) throw std::runtime_error("The runtime cannot prepare a default force model.");
      force->SetName(defaultPropagatorForceModelName(name).toStdString());
      setup->SetODEModel(force);
   }
   if (auto *body=dynamic_cast<CelestialBody *>(draft.get())) {
      body->SetUserDefined(true); body->SetSolarSystem(moderator->GetSolarSystemInUse()); body->SetUpBody();
   }
   if (auto *frame=dynamic_cast<CoordinateSystem *>(draft.get())) {
      // Factory drafts are unregistered and do not receive Moderator's default
      // axes. Supply the same default so dependent settings can be edited before
      // the outer Create transaction registers the coordinate system.
      std::unique_ptr<AxisSystem> axes(moderator->CreateAxisSystem("MJ2000Eq","",0));
      if (!axes || !frame->SetRefObject(axes.get(),Gmat::AXIS_SYSTEM,""))
         throw std::runtime_error("The runtime cannot prepare coordinate system axes.");
   }
   if (auto *spacecraft=dynamic_cast<Spacecraft *>(draft.get())) {
      spacecraft->SetSolarSystem(moderator->GetSolarSystemInUse());
      auto *frame=dynamic_cast<CoordinateSystem *>(moderator->GetConfiguredObject("EarthMJ2000Eq"));
      if (frame) { spacecraft->SetInternalCoordSystem(frame); spacecraft->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,"EarthMJ2000Eq"); }
   }
   if (auto *array=dynamic_cast<Array *>(draft.get())) array->SetSize(1,1);
   // Match Moderator::CreateCalculatedPoint without registering a pending
   // resource. Factory constructors alone leave these defaults empty.
   if (type=="LibrationPoint") {
      draft->SetStringParameter("Primary","Sun");
      draft->SetStringParameter("Secondary","Earth");
      draft->SetStringParameter("Point","L1");
   } else if (type=="Barycenter") {
      auto *point=dynamic_cast<CalculatedPoint *>(draft.get());
      point->SetDefaultBody("Earth"); point->SetDefaultBody("Luna");
   }
   const auto spacecraft=moderator->GetListOfObjects(Gmat::SPACECRAFT);
   if (type=="EphemerisFile" && !spacecraft.empty()) draft->SetStringParameter("Spacecraft",spacecraft.front());
   if (draft->IsOfType("EventLocator") && !spacecraft.empty()) {
      // Match wx's CreateEventLocator(createDefault=true) without registering
      // another spacecraft when the creator is merely opened or canceled.
      const auto *field=draft->IsOfType("ContactLocator") ? "Target" : "Spacecraft";
      if (draft->GetStringParameter(field).empty()) draft->SetStringParameter(field,spacecraft.front());
   }
   if ((type=="OrbitView" || type=="GroundTrack" || type=="GroundTrackPlot") && !spacecraft.empty()) {
      draft->SetStringParameter("Add",spacecraft.front()); draft->SetStringParameter("Add","Earth");
   }
   return draft;
}
