#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "NativeValueProvider.hpp"
#include "Solver.hpp"
#include <memory>
using namespace formaccept;

namespace
{
class Fixture : public GmatBase
{
public:
   Fixture() : GmatBase(Gmat::UNKNOWN_OBJECT, "NativeValueFixture", "Fixture") {}
   GmatBase *Clone() const override { return new Fixture(*this); }
   bool RenameRefObject(const UnsignedInt, const std::string&, const std::string&) override { return false; }
   bool HasLocalClones() override { return false; }
   GmatBase *child = nullptr, *alternative = nullptr, *probe = nullptr;
   int value = 0;
};

class RestoreFixture : public NativeRestorationPlan
{
public:
   RestoreFixture(Fixture &target, int value) : target(target), value(value) {}
   void Apply() noexcept override { target.value = value; }
private:
   Fixture &target;
   int value;
};
}

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("NativeValueProviders");
   try
   {
      require(argc == 2, "Supply the native provider test startup");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      auto *factory = FactoryManager::Instance();
      std::unique_ptr<GmatBase> spacecraft(factory->CreateObject(
            Gmat::SPACECRAFT, "Spacecraft", "ProviderSpacecraft"));
      require(bool(spacecraft), "Native spacecraft factory absent");
      const auto owned = spacecraft->GetFunctionValueOwnedObjects();
      require(std::any_of(owned.begin(), owned.end(), [](const GmatBase *object)
            { return object && object->IsOfType(Gmat::ATTITUDE); }),
            "Native spacecraft provider omitted its actual owned attitude");

      QJsonArray solvers;
      for (const auto &type : {"DifferentialCorrector", "Yukon"})
      {
         std::unique_ptr<Solver> source(factory->CreateSolver(type, "ProviderSource"));
         std::unique_ptr<Solver> target(factory->CreateSolver(type, "ProviderTarget"));
         require(source && target && source->HasSolverValueReferenceProvider() &&
               target->HasSolverValueReferenceProvider(), "Native solver provider absent");
         const auto sourceId = source->GetParameterID("MaximumIterations");
         const auto targetId = target->GetParameterID("MaximumIterations");
         source->SetIntegerParameter(sourceId, 17);
         target->SetIntegerParameter(targetId, 3);
         target->PrepareClonedSolverValue(*source);
         require(target->GetIntegerParameter(targetId) == 17 &&
               source->GetIntegerParameter(sourceId) == 17 &&
               target->GetName() == "ProviderTarget", "Native solver value transfer lost value or identity");
         solvers.append(type);
      }

      std::unique_ptr<GmatBase> function(factory->CreateObject(Gmat::FUNCTION, "Sign", "ProviderSign"));
      require(bool(function), "Native Sign factory absent");
      std::unique_ptr<GmatBase> functionCopy(function->Clone());
      require(functionCopy && functionCopy->RebindFunctionValueReferences(*function,
            Moderator::Instance()->GetSolarSystemInUse(), [](GmatBase *object) { return object; }),
            "Native builtin reference provider was not dispatched");

      Fixture target, source, firstChild, secondChild;
      target.child = &firstChild;
      target.alternative = &secondChild;
      target.probe = spacecraft.get();
      source.value = 41;
      GmatBase &base = target;
      require(base.GetFunctionValueOwnedObjects().empty(), "Unknown resource acquired a provider");
      NativeValueProvider provider;
      provider.depth = 1;
      provider.matches = [](const GmatBase *object) { return dynamic_cast<const Fixture*>(object) != nullptr; };
      provider.ownedObjects = [](const GmatBase *object) -> ObjectArray
      {
         const auto *fixture = dynamic_cast<const Fixture*>(object);
         // A callback may inspect another resource; holding the registry lock
         // during invocation would deadlock this independent native query.
         require(!fixture->probe->GetFunctionValueOwnedObjects().empty(), "Reentrant provider query lost native ownership");
         return {fixture->child};
      };
      provider.restoration = [](GmatBase *object, const GmatBase &saved) -> std::unique_ptr<NativeRestorationPlan>
      {
         return std::make_unique<RestoreFixture>(dynamic_cast<Fixture&>(*object),
               dynamic_cast<const Fixture&>(saved).value);
      };
      {
         NativeValueProviderRegistration registration(provider);
         require(base.GetFunctionValueOwnedObjects() == ObjectArray{&firstChild}, "Registered ownership callback was not used");
         auto restoration = base.PrepareRestoration(source);
         require(restoration && target.value == 0, "Restoration modified its destination during preparation");
         restoration->Apply();
         require(target.value == 41, "Prepared restoration callback was not applied");
         auto derived = provider;
         derived.depth = 2;
         derived.ownedObjects = [](const GmatBase *object) -> ObjectArray
               { return {dynamic_cast<const Fixture*>(object)->alternative}; };
         {
            NativeValueProviderRegistration nested(derived);
            require(base.GetFunctionValueOwnedObjects() == ObjectArray{&secondChild}, "More specific provider was not selected");
         }
         require(base.GetFunctionValueOwnedObjects() == ObjectArray{&firstChild}, "Retired provider did not restore the enclosing callback");
      }
      require(base.GetFunctionValueOwnedObjects().empty() && !base.PrepareRestoration(source),
            "Retired provider left callable ownership or restoration hooks");

      const QJsonObject proof{{"checkpoint", "native-value-provider-dispatch"},
         {"outcome", "passed"}, {"native_solver_types", solvers},
         {"native_spacecraft_attitude_owned", true}, {"native_builtin_rebound", true},
         {"scoped_provider_selection_retirement_and_reentrancy", true},
         {"staged_fixture_restoration", true},
         {"scope", "Actual native spacecraft ownership, DC/Yukon dormant solver value transfer and Sign reference rebinding through nonvirtual base facades; scoped extension selection, retirement, reentrancy and staged fixture restoration. No initialized model, full function/global lifetime, proprietary solver algorithm or NASA row qualification."}};
      rig.save("proof.json", QJsonDocument(proof).toJson());
      std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
      return 0;
   }
   catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << std::endl; }
   catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << std::endl; }
   return 1;
}
