#ifndef ObjectMatlabExportScope_hpp
#define ObjectMatlabExportScope_hpp

#include "gmatdefs.hpp"
#include "ObjectScriptExportScope.hpp"
#include <functional>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>

class GmatBase;

// A scoped data export keeps native field selection/prefixes while allowing
// the caller to stage typed leaf values outside the generated MATLAB source.
class GMAT_API ObjectMatlabExportScope
{
public:
   using ValueWriter = std::function<bool(GmatBase&, Integer, std::stringstream&)>;
   using RealWriter = std::function<void(Real, std::stringstream&)>;
   explicit ObjectMatlabExportScope(ValueWriter writer, RealWriter realWriter = {});
   ~ObjectMatlabExportScope() noexcept;
   ObjectMatlabExportScope(const ObjectMatlabExportScope &) = delete;
   ObjectMatlabExportScope &operator=(const ObjectMatlabExportScope &) = delete;
   static bool IsActive() noexcept;
   static bool WriteValue(GmatBase &object, Integer id, std::stringstream &value);
   static bool WriteReal(Real number, std::stringstream &value);
   static bool Applies(Gmat::WriteMode mode) noexcept
   { return IsActive() && (mode == Gmat::MATLAB_STRUCT || mode == Gmat::OWNED_OBJECT); }
   static Integer Precision(Integer usual) noexcept
   { return IsActive() ? std::numeric_limits<Real>::max_digits10 : usual; }
   static void Prepare(std::stringstream &stream)
   {
      ObjectScriptExportScope::Prepare(stream);
      if (IsActive())
      {
         stream.imbue(std::locale::classic());
         stream.precision(std::numeric_limits<Real>::max_digits10);
      }
   }
private:
   const ObjectMatlabExportScope *previous;
   ValueWriter writer;
   RealWriter realWriter;
};

// Native serializers assign their legacy mode first, then Apply supplies the
// enclosing data-export mode. Only this explicit export restores the old flag.
class ObjectMatlabModeScope
{
public:
   ObjectMatlabModeScope(bool &flag, Gmat::WriteMode mode) noexcept
      : flag(flag), previous(flag), active(ObjectMatlabExportScope::Applies(mode)) {}
   ~ObjectMatlabModeScope() noexcept { if (active) flag = previous; }
   void Apply() noexcept { if (active) flag = true; }
   ObjectMatlabModeScope(const ObjectMatlabModeScope &) = delete;
   ObjectMatlabModeScope &operator=(const ObjectMatlabModeScope &) = delete;
private:
   bool &flag;
   const bool previous, active;
};

#endif
