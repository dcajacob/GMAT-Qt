#include "ParameterReferences.hpp"
#include "Moderator.hpp"
#include "ParameterInfo.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "Parameter.hpp"
#include "FactoryManager.hpp"
#include "ResourceProperties.hpp"
#include <QRegularExpression>
#include <memory>
#include <cmath>

ParameterDependency parameterDependency(const QString &owner,const QString &property)
{
   ParameterDependency result;
   auto *info=ParameterInfo::Instance(); auto *moderator=Moderator::Instance();
   const auto type=property.toStdString();
   const auto dep=info->GetDepObjectType(type);
   UnsignedInt objectType=Gmat::UNKNOWN_OBJECT;
   if (dep==GmatParam::COORD_SYS) { objectType=Gmat::COORDINATE_SYSTEM; result.label="Coordinate system"; }
   else if (dep==GmatParam::ORIGIN) { objectType=Gmat::CELESTIAL_BODY; result.label="Central body"; }
   else if (dep==GmatParam::ODE_MODEL) { objectType=Gmat::ODE_MODEL; result.label="Force model"; }
   const bool attached=dep==GmatParam::ATTACHED_OBJ;
   QStringList attachedNames;
   if (attached) {
      objectType=info->GetOwnedObjectType(type); result.label="Attached hardware";
      if (auto *object=moderator->GetConfiguredObject(owner.toStdString())) {
         // Only direct references belong to this owner. A thruster's tank does
         // not make the tank attached to an unrelated spacecraft.
         for (int id=0;id<object->GetParameterCount();++id) {
            try {
               const auto kind=object->GetParameterType(id);
               if (kind==Gmat::OBJECT_TYPE) attachedNames.append(QString::fromStdString(object->GetStringParameter(id)));
               else if (kind==Gmat::OBJECTARRAY_TYPE)
                  for (const auto &name:object->GetStringArrayParameter(id)) attachedNames.append(QString::fromStdString(name));
            } catch (BaseException &) {} // Plugin properties may lack a string getter.
         }
      }
   }
   result.needed=attached || objectType!=Gmat::UNKNOWN_OBJECT;
   if (result.needed && objectType!=Gmat::UNKNOWN_OBJECT) for (const auto &name:moderator->GetListOfObjects(objectType)) {
      auto *object=moderator->GetConfiguredObject(name);
      if (!object || (attached && !attachedNames.contains(QString::fromStdString(name)))) continue;
      if (dep==GmatParam::COORD_SYS) {
         try {
            if (info->RequiresBodyFixedCS(type) && object->GetStringParameter("Axes")!="BodyFixed") continue;
            if (info->RequiresCelestialBodyCSOrigin(type)) {
               auto *origin=moderator->GetConfiguredObject(object->GetStringParameter("Origin"));
               if (!origin || !origin->IsOfType(Gmat::CELESTIAL_BODY)) continue;
            }
         } catch (BaseException &) { continue; }
      }
      result.choices.append(QString::fromStdString(name));
   }
   if (property=="HA") {
      result.label="Central body or coordinate system";
      for (const auto &name:moderator->GetListOfObjects(Gmat::COORDINATE_SYSTEM)) {
         auto *frame=moderator->GetConfiguredObject(name);
         if (!frame) continue;
         try {
            auto *origin=moderator->GetConfiguredObject(frame->GetStringParameter("Origin"));
            if (origin && origin->IsOfType(Gmat::CELESTIAL_BODY)) result.choices.append(QString::fromStdString(name));
         } catch (BaseException &) {}
      }
   }
   result.choices.removeDuplicates(); result.choices.sort();
   result.preferred=dep==GmatParam::COORD_SYS ? "EarthMJ2000Eq" : "Earth";
   return result;
}

QString parameterReference(const QString &owner,const QString &property,const QString &dependency)
{
   QStringList parts{owner};
   if (!dependency.isEmpty()) parts.append(dependency);
   parts.append(property); return parts.join('.');
}

QString parameterUnitDescription(const QString &reference)
{
   const auto name=reference.trimmed();
   if (name.isEmpty()) return "select a quantity for units";
   bool number=false;const auto value=name.toDouble(&number);
   if (number && std::isfinite(value)) return "user-defined units";
   const auto canonical=[](const std::string &raw) {
      auto unit=QString::fromStdString(raw).trimmed();
      const QMap<QString,QString> aliases={{"km","km"},{"kg","kg"},{"deg","deg"},{"degrees","deg"},
         {"sec","s"},{"secs","s"},{"second","s"},{"seconds","s"},{"day","days"},{"days","days"},
         {"km/sec","km/s"},{"deg/sec","deg/s"}};
      if (aliases.contains(unit.toLower())) return aliases.value(unit.toLower());
      if (unit.isEmpty() || unit.compare("None",Qt::CaseInsensitive)==0 || unit=="N/A") return QString();
      return unit;
   };
   auto *moderator=Moderator::Instance();
   if (!moderator->IsInitialized()) return "units available after Build";
   try {
      if (auto *configured=moderator->GetConfiguredObject(name.toStdString())) {
         if (configured->IsOfType("Variable") || configured->IsOfType("Array")) return "user-defined units";
         if (configured->IsOfType("String")) return "text (no physical units)";
         if (auto *parameter=dynamic_cast<Parameter *>(configured)) {
            const auto unit=canonical(parameter->GetStringParameter("Unit"));
            if (!unit.isEmpty()) return unit;
         }
      }
      const auto array=QRegularExpression(R"(^([A-Za-z][A-Za-z0-9_]*)\s*\([^()]+\)$)").match(name);
      if (array.hasMatch()) if (auto *object=moderator->GetConfiguredObject(array.captured(1).toStdString()))
         if (object->IsOfType("Array")) return "user-defined units";
      if (!QRegularExpression(R"(^[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z][A-Za-z0-9_]*)+$)").match(name).hasMatch())
         return "expression-defined units";
      auto *owner=moderator->GetConfiguredObject(name.section('.',0,0).toStdString());
      const auto field=name.section('.',-1);
      if (!owner) return "unresolved units";
      if (owner->IsOfType("Spacecraft") && (field=="Apoapsis" || field=="Periapsis")) return "dimensionless";
      const auto dimensionless=dimensionlessResourceUnit(*owner,field);
      if (!dimensionless.isEmpty()) return dimensionless;
      // Empty names keep temporary metadata prototypes out of ParameterInfo's
      // instance-name registry. No references are installed and no value read.
      std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(field.toStdString(),"",owner));
      if (prototype) {
         const auto unit=canonical(prototype->GetStringParameter("Unit"));
         if (!unit.isEmpty()) return unit;
      }
      for (const auto &property:resourceProperties(*owner)) if (property.name==field && !property.unit.isEmpty()) return property.unit;
   } catch (BaseException &) {
      // Partial references remain editable while the user is typing.
   } catch (const std::exception &) {}
   return "parameter-defined units";
}
