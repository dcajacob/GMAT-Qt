#include "CSFixed.hpp"
#include "Spinner.hpp"
#include "BaseException.hpp"
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
int assertions = 0;
void require(bool condition, const char *message)
{
   ++assertions;
   if (!condition) throw std::runtime_error(message);
}
Rvector vector(std::initializer_list<double> values)
{
   Rvector result(static_cast<Integer>(values.size()));
   Integer i = 0;
   for (double value : values) result[i++] = value;
   return result;
}
bool equal(const Rvector &a, const Rvector &b)
{
   if (a.GetSize() != b.GetSize()) return false;
   for (Integer i = 0; i < a.GetSize(); ++i) if (a[i] != b[i]) return false;
   return true;
}
void rejects(CSFixed &model, const std::function<void()> &action)
{
   const auto before = model.GetRvectorParameter("OffsetQuaternion");
   const auto source = model.GetGeneratingString(Gmat::OWNED_OBJECT, "Sat.");
   bool rejected = false;
   try { action(); } catch (BaseException &) { rejected = true; }
   require(rejected, "Invalid native offset input was accepted");
   require(equal(before, model.GetRvectorParameter("OffsetQuaternion")),
           "Rejected native offset changed the quaternion");
   require(source == model.GetGeneratingString(Gmat::OWNED_OBJECT, "Sat."),
           "Rejected native offset changed serialization");
}
}

int main()
{
   try {
      CSFixed model;
      Spinner inherited;
      const Integer id = model.GetParameterID("OffsetQuaternion");
      require(id == inherited.GetParameterCount(), "Offset ID does not append to inherited IDs");
      require(model.GetParameterCount() == inherited.GetParameterCount() + 1,
              "Fixed-offset parameter count differs");
      for (Integer i = 0; i < inherited.GetParameterCount(); ++i) {
         require(model.GetParameterText(i) == inherited.GetParameterText(i),
                 "Existing attitude parameter text shifted");
         require(model.GetParameterType(i) == inherited.GetParameterType(i),
                 "Existing attitude parameter type changed");
      }
      require(model.GetParameterType(id) == Gmat::RVECTOR_TYPE && !model.IsParameterReadOnly(id),
              "Fixed offset has incorrect native metadata");
      require(!model.SetInitialAttitudeAllowed(), "Original inactive initial-state contract changed");
      const auto identity = vector({0,0,0,1});
      require(equal(model.GetRvectorParameter(id), identity), "Default offset is not identity");
      const auto original = model.GetGeneratingString(Gmat::OWNED_OBJECT, "Sat.");
      require(original.find("OffsetQuaternion") == std::string::npos,
              "Default offset changed the original script");
      model.SetRvectorParameter("OffsetQuaternion", vector({1,2,3,4}));
      auto normalized = model.GetRvectorParameter(id);
      for (Integer i = 0; i < 4; ++i)
         require(std::abs(normalized[i] - (i+1)/std::sqrt(30.0)) < 1e-15,
                 "Offset normalization differs from independent Euclidean norm");
      require(model.Validate(), "Valid offset failed validation");
      require(model.GetGeneratingString(Gmat::OWNED_OBJECT, "Sat.").find("Sat.OffsetQuaternion = [") != std::string::npos,
              "Nonzero offset did not serialize as a native vector");
      const double infinity = std::numeric_limits<double>::infinity();
      const double nan = std::numeric_limits<double>::quiet_NaN();
      for (const auto &invalid : {vector({1,2,3}), vector({1,2,3,4,5}),
              vector({0,0,0,0}), vector({0,0,0,1e-12}),
              vector({nan,0,0,1}), vector({infinity,0,0,1}), vector({-infinity,0,0,1})})
         rejects(model, [&] { model.SetRvectorParameter(id, invalid); });
      rejects(model, [&] { model.SetRealParameter(id, nan, 0); });
      rejects(model, [&] { model.SetRealParameter(id, 0.1, -1); });
      rejects(model, [&] { model.SetRealParameter(id, 0.1, 4); });
      for (Integer index : {-1,4})
         rejects(model, [&] { model.GetRealParameter(id, index); });
      model.SetRvectorParameter(id, vector({1e308,1e308,1e308,1e308}));
      require(equal(model.GetRvectorParameter(id), vector({.5,.5,.5,.5})),
              "Finite large quaternion overflowed normalization");
      const auto saved = model.GetRvectorParameter(id);
      std::unique_ptr<GmatBase> clone(model.Clone());
      require(equal(clone->GetRvectorParameter("OffsetQuaternion"), saved), "Clone lost offset");
      clone->SetRvectorParameter("OffsetQuaternion", vector({0,0,1,0}));
      require(equal(model.GetRvectorParameter(id), saved), "Clone offset aliases original storage");
      CSFixed assigned;
      assigned = model;
      require(equal(assigned.GetRvectorParameter(id), saved), "Assignment lost offset");
      assigned.Copy(&model);
      require(equal(assigned.GetRvectorParameter(id), saved), "Polymorphic Copy lost offset");
      assigned.Copy(&assigned);
      require(equal(assigned.GetRvectorParameter(id), saved), "Self Copy lost offset");
      rejects(model, [&] { model.Copy(&inherited); });
      model.SetRvectorParameter(id, vector({0,0,0,-4}));
      require(equal(model.GetRvectorParameter(id), vector({0,0,0,-1})),
              "Negative quaternion representative was discarded");
      model.SetRvectorParameter(id, identity);
      require(model.GetGeneratingString(Gmat::OWNED_OBJECT, "Sat.") == original,
              "Resetting the offset did not restore the original script");
      GmatBase &polymorphic = model;
      polymorphic.SetRealParameter("OffsetQuaternion", .125, 0);
      require(polymorphic.GetRealParameter("OffsetQuaternion", 0) == .125,
              "Indexed native property dispatch differs");
      model.SetRvectorParameter(id, model.GetRvectorParameter(id));
      require(std::abs(model.GetRvectorParameter(id).GetMagnitude()-1) < 8*std::numeric_limits<double>::epsilon(),
              "Indexed quaternion could not be normalized as a complete vector");
      std::cout << "{\"status\":\"passed\",\"assertions\":" << assertions
                << ",\"sizeof_CSFixed\":" << sizeof(CSFixed)
                << ",\"scope\":\"native metadata, normalization, atomic guards, copy and serialization; not complete frame qualification\"}\n";
      if (const char *hold=std::getenv("GMAT_QT_TEST_PROCESS_HOLD_MS")) { const int ms=std::atoi(hold); if(ms>0&&ms<=1000) std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
     catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
