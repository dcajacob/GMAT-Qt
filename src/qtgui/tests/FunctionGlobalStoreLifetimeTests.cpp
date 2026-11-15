#include "FormationAcceptance.hpp"
#include "ObjectManagedFunction.hpp"
#include "ObjectStoreLifetime.hpp"
#include "Sandbox.hpp"
#include "Variable.hpp"
#include "CallFunction.hpp"
#include "SolarSystem.hpp"
#include <new>

using namespace formaccept;

namespace {
class ViewProbe : public ObjectManagedFunction {
public:
   explicit ViewProbe(const std::string &name = "View") : ObjectManagedFunction("GlobalStoreViewProbe", name) {}
   GmatBase *Clone() const override { return new ViewProbe(*this); }
   SolarSystem *BoundSolarSystem() const { return solarSys; }
};
class CallViewProbe : public CallFunction {
public:
   CallViewProbe() : CallFunction("CallGmatFunction") {}
   Function *BoundFunction() const { return mFunction; }
};
void bind(ViewProbe &function, ObjectMap &local, ObjectMap &global) {
   function.SetFunctionObjectMap(&local);
   function.SetGlobalObjectMap(&global);
}
QJsonArray unitViews() {
   Variable first("First"), second("Second");
   ObjectMap local, innerLocal, global{{"Shared", &first}}, innerGlobal{{"Shared", &second}};
   ObjectStoreLifetime localOwner, innerLocalOwner, owner, innerOwner;
   localOwner.Bind(&local); innerLocalOwner.Bind(&innerLocal);
   owner.Bind(&global); innerOwner.Bind(&innerGlobal);
   ViewProbe function;
   bind(function, local, global);
   function.RetireFunctionObjectStore(&local);
   require(!function.GetFunctionObjectStore() && !function.HasActiveInvocation() &&
         function.GetGlobalObjectStore() == &global && global.at("Shared") == &first,
         "Retired function lost its actual live global view or retained its invocation");
   QJsonArray result{QJsonObject{{"retired_invocation_preserves_owner_tracked_identity", true}}};

   std::unique_ptr<GmatBase> clone(function.Clone());
   auto *copy = dynamic_cast<ObjectManagedFunction*>(clone.get());
   require(copy && !copy->GetFunctionObjectStore() && !copy->GetGlobalObjectStore(),
         "Cold definition clone inherited a caller's observed global scope");
   ViewProbe assigned;
   bind(assigned, innerLocal, innerGlobal); assigned.RetireFunctionObjectStore(&innerLocal);
   assigned = function;
   require(!assigned.GetGlobalObjectStore(), "Definition assignment retained an unrelated observed scope");
   result.append(QJsonObject{{"clone_and_assignment_are_cold", true}});

   auto outer = function.EnterInvocation();
   bind(function, local, global);
   auto inner = function.EnterInvocation();
   require(!function.GetGlobalObjectStore(), "Unbound nested invocation borrowed the enclosing observation");
   bind(function, innerLocal, innerGlobal);
   function.RetireFunctionObjectStore(&innerLocal);
   require(!function.GetGlobalObjectStore(), "Active unbound nested invocation exposed its retired observation");
   require(inner->Restore() && function.GetFunctionObjectStore() == &local &&
         function.GetGlobalObjectStore() == &global, "Nested invocation did not restore its actual enclosing binding");
   function.RetireFunctionObjectStore(&local);
   require(outer->Restore() && function.GetGlobalObjectStore() == &global,
         "Final nested cleanup retained the inner caller's global scope");
   result.append(QJsonObject{{"nested_active_and_observed_scopes_stay_distinct", true}});

   ObjectStoreLifetime duplicate;
   bool rejected = false;
   try { duplicate.Bind(&global); }
   catch (BaseException &error) { rejected = error.GetFullMessage().find("already has a lifetime owner") != std::string::npos; }
   require(rejected && function.GetGlobalObjectStore() == &global, "Duplicate registration replaced the actual map owner");
   owner.Reset();
   require(!function.GetGlobalObjectStore(), "Retired map remained visible");
   global["Shared"] = &second;
   owner.Bind(&global);
   require(!function.GetGlobalObjectStore(), "Same-address map registration revived an old scope view");
   bind(function, local, global); function.RetireFunctionObjectStore(&local);
   require(function.GetGlobalObjectStore() == &global && function.GetGlobalObjectStore()->at("Shared") == &second,
         "Explicit new binding did not select the new actual map generation");
   result.append(QJsonObject{{"owner_retirement_address_reuse_and_explicit_rebinding", true}});

   {
      ObjectMap temporary;
      ObjectStoreLifetime temporaryOwner;
      temporaryOwner.Bind(&temporary);
      bind(assigned, local, temporary); assigned.RetireFunctionObjectStore(&local);
      require(assigned.GetGlobalObjectStore() == &temporary, "Temporary owner view not installed");
   }
   require(!assigned.GetGlobalObjectStore(), "Owner destruction retained a dangling map");
   ObjectMap unregistered;
   bind(assigned, local, unregistered);
   require(assigned.GetGlobalObjectStore() == &unregistered, "Active caller-managed map contract changed");
   assigned.RetireFunctionObjectStore(&local);
   require(!assigned.GetGlobalObjectStore(), "Unregistered caller map was retained after invocation retirement");
   result.append(QJsonObject{{"owner_destruction_and_unregistered_borrowing", true}});

   alignas(ViewProbe) unsigned char storage[sizeof(ViewProbe)];
   auto *retired = new (storage) ViewProbe("OldDefinition");
   bind(*retired, local, global); retired->RetireFunctionObjectStore(&local);
   retired->~ViewProbe();
   auto *replacement = new (storage) ViewProbe("NewDefinition");
   const bool cold = !replacement->GetGlobalObjectStore();
   replacement->~ViewProbe();
   require(cold, "Same-address function construction inherited an old definition's scope");
   result.append(QJsonObject{{"function_address_reuse_is_cold", true}});

   auto active = function.EnterInvocation();
   bind(function, local, global);
   auto nested = function.EnterInvocation();
   bind(function, innerLocal, innerGlobal);
   owner.Reset();
   require(!nested->Restore() && !function.GetGlobalObjectStore(),
         "Restoration revived an enclosing map after its actual owner retired");
   require(active->Restore() && !function.GetGlobalObjectStore(), "Failed restoration left a stale global view");
   function.RetireFunctionObjectStore(&local);
   result.append(QJsonObject{{"retired_enclosing_owner_prevents_resume", true}});
   return result;
}
QJsonArray serviceViews() {
   ObjectMap local, global, innerLocal, innerGlobal;
   ObjectStoreLifetime owner, innerOwner;
   owner.Bind(&global); innerOwner.Bind(&innerGlobal);
   SolarSystem outerSystem("OuterService"), innerSystem("InnerService");
   ViewProbe function;
   bind(function, local, global); function.SetSolarSystem(&outerSystem);
   function.RetireFunctionObjectStore(&local);
   require(!function.BoundSolarSystem() && function.GetSolarSystem() == &outerSystem,
         "Completed observation retained a bound service or lost the actual live system");
   function.SetSolarSystem(nullptr);
   require(!function.GetSolarSystem() && function.GetGlobalObjectStore() == &global,
         "Explicit solar-system detach retained an observation or erased the global scope");
   bind(function, local, global); function.SetSolarSystem(&outerSystem);
   function.RetireFunctionObjectStore(&local);
   std::unique_ptr<GmatBase> clone(function.Clone());
   require(!dynamic_cast<Function*>(clone.get())->GetSolarSystem(),
         "Cold clone inherited a completed solar-system observation");
   ViewProbe assigned;
   bind(assigned, local, global); assigned.SetSolarSystem(&innerSystem);
   assigned.RetireFunctionObjectStore(&local); assigned = function;
   require(!assigned.GetSolarSystem(), "Assignment retained another function's completed service");
   QJsonArray result{QJsonObject{{"actual_completed_service_and_cold_copies", true}}};

   auto outer = function.EnterInvocation();
   bind(function, local, global); function.SetSolarSystem(&outerSystem);
   auto inner = function.EnterInvocation();
   bind(function, innerLocal, innerGlobal); function.SetSolarSystem(&innerSystem);
   function.RetireFunctionObjectStore(&innerLocal);
   require(!function.GetSolarSystem(), "Retired active inner invocation borrowed an observed service");
   require(inner->Restore() && function.GetSolarSystem() == &outerSystem,
         "Nested service restoration selected a different system");
   function.RetireFunctionObjectStore(&local);
   require(outer->Restore() && function.GetSolarSystem() == &outerSystem && !function.BoundSolarSystem(),
         "Outer cleanup lost its observed system or revived an active service");
   owner.Reset();
   require(!function.GetSolarSystem(), "Retired scope owner retained a system observation");
   owner.Bind(&global);
   require(!function.GetSolarSystem(), "Reused scope address revived a service observation");
   result.append(QJsonObject{{"nested_service_identity_and_scope_generations", true}});

   alignas(SolarSystem) unsigned char systemStorage[sizeof(SolarSystem)];
   auto *first = new (systemStorage) SolarSystem("OldService");
   bind(function, local, global); function.SetSolarSystem(first);
   function.RetireFunctionObjectStore(&local);
   require(function.GetSolarSystem() == first, "Tracked system observation absent");
   first->~SolarSystem();
   require(!function.GetSolarSystem(), "Destroyed solar system remained observable");
   auto *next = new (systemStorage) SolarSystem("NewService");
   require(!function.GetSolarSystem(), "Same-address solar system revived an old observation");
   bind(function, local, global); function.SetSolarSystem(next);
   function.RetireFunctionObjectStore(&local);
   const bool rebound = function.GetSolarSystem() == next;
   next->~SolarSystem();
   require(rebound && !function.GetSolarSystem(), "Explicit system rebind or destruction failed");
   result.append(QJsonObject{{"solar_system_deletion_address_reuse_and_rebinding", true}});

   auto setCallee = [&](CallViewProbe &call, ViewProbe &callee) {
      call.SetObjectMap(&local);
      call.SetStringParameter(call.GetParameterID("FunctionName"), callee.GetName());
      require(call.SetRefObject(&callee, Gmat::FUNCTION, callee.GetName()), "Callee binding rejected");
   };
   auto callee = [](CallFunction &call) { return call.GetRefObject(Gmat::FUNCTION, ""); };
   CallViewProbe call;
   setCallee(call, function);
   call.RetireFunctionBindings(&local);
   require(!call.BoundFunction() && !call.GetObjectMap() && callee(call) == &function,
         "Completed call kept active bindings or lost its actual live callee");
   std::unique_ptr<GmatBase> callClone(call.Clone());
   require(!dynamic_cast<CallFunction*>(callClone.get())->GetRefObject(Gmat::FUNCTION, ""),
         "Cold command clone inherited a completed callee observation");
   CallViewProbe other;
   setCallee(other, assigned); other.RetireFunctionBindings(&local); other = call;
   require(!callee(other), "Command assignment retained an unrelated callee observation");
   call.SetStringParameter(call.GetParameterID("FunctionName"), "Different");
   require(!callee(call), "Function rename retained the previous callee");
   setCallee(call, function); call.RetireFunctionBindings(&local);
   call.SetObjectMap(&innerLocal);
   require(!callee(call), "Rebinding a command scope retained its previous callee");
   result.append(QJsonObject{{"completed_callee_cold_copies_rename_and_scope_rebind", true}});

   alignas(ViewProbe) unsigned char functionStorage[sizeof(ViewProbe)];
   auto *oldFunction = new (functionStorage) ViewProbe("SameName");
   setCallee(call, *oldFunction); call.RetireFunctionBindings(&local);
   require(callee(call) == oldFunction, "Retired call observation missing");
   oldFunction->~ViewProbe();
   require(!callee(call), "Deleted callee remained observable");
   auto *newFunction = new (functionStorage) ViewProbe("SameName");
   require(!callee(call), "Same-address function revived a previous callee observation");
   setCallee(call, *newFunction); call.RetireFunctionBindings(&local);
   const bool newBinding = callee(call) == newFunction;
   newFunction->~ViewProbe();
   require(newBinding && !callee(call), "Explicit callee rebind or deletion failed");
   result.append(QJsonObject{{"callee_deletion_address_reuse_and_rebinding", true}});
   return result;
}
void build(Rig &rig, const QString &source) {
   rig.source->setPlainText(source);
   require(rig.window.buildScript(), "Native global-store mission did not Build");
}
const ObjectMap *actualGlobals() {
   for (auto *command = Moderator::Instance()->GetFirstCommand(); command; command = command->GetNext()) {
      if (!command->IsOfType("CallFunction")) continue;
      auto *function = dynamic_cast<ObjectManagedFunction*>(command->GetRefObject(Gmat::FUNCTION, ""));
      require(function && !function->GetFunctionObjectStore() && !function->HasActiveInvocation(),
            "Native completed function retained an active invocation");
      return function->GetGlobalObjectStore();
   }
   return nullptr;
}
QJsonObject sandboxViews(Rig &rig) {
   const auto file = rig.files.filePath("ReadGlobal.gmf");
   write(file, "function [out] = ReadGlobal()\nCreate Variable out;\nBeginMissionSequence;\nGlobal Shared;\nout = Shared+1;\n");
   const auto source = "Create Variable Shared Result;\nShared = 11;\nCreate GmatFunction ReadGlobal;\n"
         "ReadGlobal.FunctionPath = '" + file + "';\nBeginMissionSequence;\nGlobal Shared;\n[Result] = ReadGlobal();\n";
   build(rig, source);
   require(rig.window.runMission() == MainWindow::RunResult::Completed, "Native global-store mission failed");
   events();
   const auto *first = actualGlobals();
   auto *shared = Moderator::Instance()->GetInternalObject("Shared");
   require(first && first->at("Shared") == shared && shared->GetRealParameter("Value") == 11 &&
         Moderator::Instance()->GetInternalObject("Result")->GetRealParameter("Value") == 12,
         "Native view does not preserve the actual caller global and independent result");
   ObjectMap local;
   ViewProbe external;
   bind(external, local, *const_cast<ObjectMap*>(first)); external.RetireFunctionObjectStore(&local);
   require(external.GetGlobalObjectStore() == first, "External native observation did not capture the actual Sandbox owner");
   Moderator::Instance()->GetSandbox()->Clear();
   require(!external.GetGlobalObjectStore(), "Sandbox Clear left an external view dangling");
   build(rig, QString(source).replace("Shared = 11", "Shared = 21"));
   require(rig.window.runMission() == MainWindow::RunResult::Completed, "Next Sandbox generation failed");
   events();
   const auto *next = actualGlobals();
   require(next && next->at("Shared") == Moderator::Instance()->GetInternalObject("Shared") &&
         next->at("Shared")->GetRealParameter("Value") == 21 &&
         Moderator::Instance()->GetInternalObject("Result")->GetRealParameter("Value") == 22 &&
         !external.GetGlobalObjectStore(), "Rebuilt Sandbox revived the previous generation's observation");
   return QJsonObject{{"actual_sandbox_identity", true}, {"clear_invalidates_external_view", true},
         {"next_generation_stays_distinct", true}, {"same_map_address", first == next},
         {"independent_results", QJsonArray{12, 22}}};
}
}

int main(int argc, char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv); app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionGlobalStoreLifetime");
   try {
      require(argc == 2, "Supply native global-store startup");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto units = unitViews();
      const auto services = serviceViews();
      const auto native = sandboxViews(rig);
      const QJsonObject proof{{"checkpoint", "function-global-store-lifetime"}, {"outcome", "passed"},
            {"unit_observations", units}, {"service_observations", services}, {"native_sandbox", native}, {"new_requirement_credit", 0},
            {"scope", "Actual caller map observations, cold invocation/definition behavior, retirement/generation guards and real Sandbox Clear/rebuild. No complete eligible-resource/scoping/science/platform qualification."}};
      rig.save("proof.json", QJsonDocument(proof).toJson());
      std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
   catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
