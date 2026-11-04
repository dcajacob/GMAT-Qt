#include "SpacecraftOrbit.hpp"
#include "ResourceProperties.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "CoordinateConverter.hpp"
#include "CelestialBody.hpp"
#include "Moderator.hpp"
#include "StateConversionUtil.hpp"
#include "GmatConstants.hpp"
#include <cmath>
#include <stdexcept>
#include <memory>
#include <QRegularExpression>

CoordinateSystem *spacecraftOrbitFrame(const QString &name)
{
   auto *frame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject(name.toStdString()));
   if (!frame || !frame->GetOrigin()) throw std::runtime_error("Select an available, initialized coordinate system.");
   return frame;
}

QStringList spacecraftOrbitRepresentations(CoordinateSystem &frame)
{
   QStringList choices;
   const auto *types=StateConversionUtil::GetStateTypeList();
   for (int i=0;i<StateConversionUtil::GetTypeCount();++i) {
      if (StateConversionUtil::RequiresCelestialBodyOrigin(types[i]) && !frame.HasCelestialBodyOrigin()) continue;
      if (StateConversionUtil::RequiresFixedCoordinateSystem(types[i]) && !frame.AreAxesOfType("BodyFixedAxes")) continue;
      // Brouwer's fixed Earth zonal model is not available for other origins,
      // even if a user assigns that body an Earth-like gravitational constant.
      if ((types[i]=="BrouwerMeanShort" || types[i]=="BrouwerMeanLong") &&
          (!frame.GetOrigin() || frame.GetOrigin()->GetName()!="Earth")) continue;
      choices.append(QString::fromStdString(types[i]));
   }
   return choices;
}

QStringList spacecraftOrbitElementNames(GmatBase &object)
{
   QStringList names;
   for (int i=1;i<=6;++i) names.append(QString::fromStdString(object.GetParameterText(object.GetParameterID("Element"+std::to_string(i)))));
   const auto representation=object.GetStringParameter("DisplayStateType");
   if (representation=="Keplerian" || representation=="ModifiedKeplerian") names[5]=QString::fromStdString(static_cast<Spacecraft &>(object).GetAnomalyType());
   return names;
}

QStringList spacecraftOrbitAnomalies(GmatBase &object)
{
   const auto representation=object.GetStringParameter("DisplayStateType");
   if (representation!="Keplerian" && representation!="ModifiedKeplerian") return {"TA","MA","EA","HA"};
   const auto eccentricity=static_cast<Spacecraft &>(object).GetState("Keplerian")[1];
   return eccentricity<1 ? QStringList{"TA","MA","EA"} : QStringList{"TA","MA","HA"};
}

QSet<QString> applySpacecraftOrbitProperties(GmatBase &object,const QMap<QString,QString> &values)
{
   QSet<QString> consumed;
   auto *spacecraft=dynamic_cast<Spacecraft *>(&object);
   if (!spacecraft) return consumed;
   // Epoch format/epoch and frame binding precede dependent elements. A string
   // frame name alone does not replace the clone's coordinate-system pointer.
   for (const auto &name:QStringList{"DateFormat","Epoch","CoordinateSystem","DisplayStateType","AnomalyType"}) {
      if (!values.contains(name)) continue;
      setResourceProperty(object,name,values.value(name)); consumed.insert(name);
      if (name=="CoordinateSystem") {
         const auto frameName=values.value(name);
         auto *frame=spacecraftOrbitFrame(frameName);
         if (!spacecraft->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,frameName.toStdString())) throw std::runtime_error("The spacecraft rejected this coordinate system.");
      }
      if (name=="AnomalyType") {
         // The string setter refreshes the default labels (TA). The wx orbit
         // panel also calls SetAnomaly to select the displayed sixth-element
         // label; use that public API so MA/EA/HA values cannot be labeled TA.
         spacecraft->SetAnomaly(values.value(name).toStdString(),0);
      }
   }
   const auto names=spacecraftOrbitElementNames(object);
   bool changed=false; for (const auto &name:names) changed=changed || values.contains(name);
   if (consumed.isEmpty() && !changed && !values.contains("@OrbitCartesianState")) return consumed;
   const auto representation=QString::fromStdString(spacecraft->GetStringParameter("DisplayStateType"));
   auto *frame=spacecraftOrbitFrame(QString::fromStdString(spacecraft->GetStringParameter("CoordinateSystem")));
   if (!spacecraftOrbitRepresentations(*frame).contains(representation)) throw std::runtime_error("This orbit representation is unavailable in the selected coordinate system.");
   auto validateAnomaly=[&] {
      if (!spacecraftOrbitAnomalies(object).contains(QString::fromStdString(spacecraft->GetAnomalyType())))
         throw std::runtime_error("Select an anomaly type appropriate for this orbit's eccentricity.");
   };
   if (values.contains("@OrbitCartesianState")) {
      const auto values6=values.value("@OrbitCartesianState").split(',');
      if (values6.size()!=6) throw std::runtime_error("The pending orbit state requires six Cartesian elements.");
      Rvector6 internal;
      for (int i=0;i<6;++i) {
         bool valid=false; internal[i]=values6[i].toDouble(&valid);
         if (!valid || !std::isfinite(internal[i])) throw std::runtime_error("The pending orbit state must contain finite numbers.");
      }
      spacecraft->SetState(internal); validateAnomaly(); consumed.insert("@OrbitCartesianState");
      for (const auto &name:names) if (values.contains(name)) consumed.insert(name);
      return consumed;
   }
   if (!changed) { if (values.contains("AnomalyType")) validateAnomaly(); return consumed; }
   Rvector6 current;
   bool partial=false; for (const auto &name:names) partial=partial || !values.contains(name);
   if (partial) current=spacecraft->GetState(representation.toStdString());
   Rvector6 input;
   for (int i=0;i<6;++i) {
      const auto &name=names[i];
      if (!values.contains(name)) input[i]=current[i];
      else {
         bool valid=false; input[i]=values.value(name).toDouble(&valid);
         if (!valid || !std::isfinite(input[i])) throw std::runtime_error((name+" must be a finite number.").toStdString());
         consumed.insert(name);
      }
   }
   // Match the wx orbit panel's input checks before conversion: converters may
   // normalize invalid inputs (negative eccentricity or spherical magnitude).
   // Validate coupled values against this complete pending state, not the old
   // configured state, so valid simultaneous edits remain atomic.
   const auto errorFormat=spacecraft->GetErrorMessageFormat();
   for (int i=0;i<6;++i) {
      int paired=-1;
      if ((representation=="Keplerian" || representation=="ModifiedKeplerian") && i<2) paired=1-i;
      if (representation=="Equinoctial" && (i==1 || i==2)) paired=3-i;
      StateConversionUtil::ValidateValue(names[i].toStdString(),input[i],errorFormat,17,
         paired>=0 ? names[paired].toStdString() : "",paired>=0 ? input[paired] : 0);
   }
   // Submit the six elements together. Changing eccentricity or inclination one
   // scalar at a time can make otherwise valid paired state edits fail midway.
   auto *body=dynamic_cast<CelestialBody *>(frame->GetOrigin());
   const auto cartesian=StateConversionUtil::Convert(input,representation.toStdString(),"Cartesian",
      body ? body->GetGravitationalConstant() : 0,body ? body->GetFlattening() : 0,body ? body->GetEquatorialRadius() : 0,
      spacecraft->GetStringParameter("AnomalyType"));
   auto *internal=spacecraft->GetInternalCoordSystem();
   if (!internal) throw std::runtime_error("The spacecraft has no internal coordinate system. Rebuild the mission.");
   Rvector6 converted;
   CoordinateConverter converter;
   if (spacecraft->HasPrecisionTime()) converter.Convert(spacecraft->GetEpochGT(),cartesian,frame,converted,internal);
   else converter.Convert(spacecraft->GetEpoch(),cartesian,frame,converted,internal);
   spacecraft->SetState(converted);
   validateAnomaly();
   if (representation=="Keplerian" || representation=="ModifiedKeplerian") {
      const auto keplerian=spacecraft->GetState("Keplerian");
      spacecraft->SetAnomaly(spacecraft->GetAnomalyType(),StateConversionUtil::ConvertToTrueAnomaly(spacecraft->GetAnomalyType(),
         keplerian[5]*GmatMathConstants::RAD_PER_DEG,keplerian[1])*GmatMathConstants::DEG_PER_RAD);
   }
   return consumed;
}

QString spacecraftOrbitScript(GmatBase &object)
{
   auto *spacecraft=dynamic_cast<Spacecraft *>(&object);
   if (!spacecraft)
      return QString::fromStdString(object.GetGeneratingString(Gmat::SCRIPTING));
   // Display elements are not a lossless serialization of the physical state.
   // Near-circular Keplerian angle round trips can move the orbit by centimetres;
   // Planetodetic conversion is approximate and non-TA anomalies are omitted by
   // the engine writer. Preserve Cartesian input with the independent display
   // and anomaly selectors for every non-Cartesian GUI orbit.
   std::unique_ptr<GmatBase> copy(spacecraft->Clone());
   copy->SetStringParameter("DisplayStateType","Cartesian");
   auto block=QString::fromStdString(copy->GetGeneratingString(Gmat::SCRIPTING));
   // The engine writer's general display precision is 16 significant digits.
   // State serialization needs 17 for an exact double round trip; even one ULP
   // can change the accepted pass of a sensitive target sequence.
   for (const auto &element:QStringList{"X","Y","Z","VX","VY","VZ"}) {
      const QRegularExpression field("(^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(QString::fromStdString(object.GetName()))+
         "\\."+element+"[ \\t]*=[ \\t]*)[^;\\n]+([ \\t]*;)",QRegularExpression::MultilineOption);
      const auto match=field.match(block);
      if (!match.hasMatch()) throw std::runtime_error("A Cartesian orbit component could not be serialized.");
      block.replace(match.capturedStart(0),match.capturedLength(0),match.captured(1)+
         QString::number(copy->GetRealParameter(element.toStdString()),'g',17)+match.captured(2));
   }
   const QRegularExpression display("(^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(QString::fromStdString(object.GetName()))+"\\.DisplayStateType[ \\t]*=[ \\t]*)Cartesian([ \\t]*;)",QRegularExpression::MultilineOption);
   if (!display.match(block).hasMatch()) throw std::runtime_error("The spacecraft display selector could not be serialized.");
   block.replace(display,"\\1"+QString::fromStdString(spacecraft->GetStringParameter("DisplayStateType"))+"\\2");
   if (spacecraft->GetAnomalyType()!="TA") {
      // Insert after the display selector, before the Cartesian inputs. Those
      // inputs do not override the independently selected anomaly type.
      const QRegularExpression selector("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(QString::fromStdString(object.GetName()))+"\\.DisplayStateType[^\\n]*\\n",QRegularExpression::MultilineOption);
      const auto selected=selector.match(block);
      if (!selected.hasMatch()) throw std::runtime_error("The spacecraft display selector could not be serialized.");
      block.insert(selected.capturedEnd(),QString::fromStdString(object.GetName())+".AnomalyType = "+QString::fromStdString(spacecraft->GetAnomalyType())+";\n");
   }
   return block;
}

QString preserveSpacecraftOrbits(QString script,GmatBase *exclude)
{
   auto *moderator=Moderator::Instance();
   for (const auto &name:moderator->GetListOfObjects(Gmat::SPACECRAFT)) {
      auto *spacecraft=moderator->GetConfiguredObject(name);
      if (!spacecraft || spacecraft==exclude) continue;
      const auto existing=QString::fromStdString(spacecraft->GetGeneratingString(Gmat::SCRIPTING));
      const auto preserved=spacecraftOrbitScript(*spacecraft);
      if (existing!=preserved && !existing.isEmpty() && script.count(existing)==1) script.replace(script.indexOf(existing),existing.size(),preserved);
   }
   return script;
}
