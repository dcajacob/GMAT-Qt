#include "ScriptCompatibility.hpp"
#include "RgbColor.hpp"
#include "BaseException.hpp"
#include "StringUtil.hpp"
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <array>
#include <cmath>
#include <limits>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>

namespace {
QString codePart(const QString &line)
{
   bool quoted=false; int end=line.size();
   for (int i=0;i<line.size();++i) {
      if (line[i]=='\'') {
         if (quoted && i+1<line.size() && line[i+1]=='\'') { ++i; continue; }
         quoted=!quoted;
      } else if (line[i]=='%' && !quoted) { end=i; break; }
   }
   auto code=line.left(end).trimmed();
   // Empty trailing statements (for example the shipped IntegratedFlyby Add
   // list ending in ;;) are harmless. Normalize only this temporary parse view;
   // original output lines, quoted text and interior compound statements stay.
   if (!quoted) while (code.endsWith(';')) { code.chop(1); code=code.trimmed(); }
   return code;
}
bool vectorValue(QString value,std::array<double,3> &result)
{
   if (!value.startsWith('[') || !value.endsWith(']')) return false;
   const auto fields=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
   if (fields.size()!=3) return false;
   for (int i=0;i<3;++i) { bool ok=false; result[i]=fields[i].toDouble(&ok); if (!ok || !std::isfinite(result[i])) return false; }
   return true;
}
QMap<QString,bool> objectFlags(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Per-object display flags must be objects of named boolean values");
   const auto values=value.toObject(); QMap<QString,bool> result;
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      if (!identifier.match(it.key()).hasMatch() || !it.value().isBool()) throw std::runtime_error("Per-object display flags need valid names and booleans");
      result.insert(it.key(),it.value().toBool());
   }
   return result;
}
QJsonObject objectFlagsJson(const QMap<QString,bool> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),it.value()); return result;
}
QMap<QString,quint32> objectSizes(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Marker sizes must be objects of named nonnegative integers");
   QMap<QString,quint32> result; const auto values=value.toObject();
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      const double size=it.value().toDouble(-1);
      if (!identifier.match(it.key()).hasMatch() || !it.value().isDouble() || size<0 || size>4294967295.0 || std::floor(size)!=size)
         throw std::runtime_error("Marker sizes need valid names and unsigned integer pixel sizes");
      result.insert(it.key(),static_cast<quint32>(size));
   }
   return result;
}
QJsonObject objectSizesJson(const QMap<QString,quint32> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),double(it.value())); return result;
}
QMap<QString,double> objectWidths(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Line widths must be objects of named finite values >= 1");
   QMap<QString,double> result; const auto values=value.toObject();
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      const double width=it.value().toDouble(-1);
      if (!identifier.match(it.key()).hasMatch() || !it.value().isDouble() || !std::isfinite(width) || width<1 || width>std::numeric_limits<float>::max())
         throw std::runtime_error("Line widths need valid names and finite pixel values >= 1 within the renderer's range");
      result.insert(it.key(),width);
   }
   return result;
}
QJsonObject objectWidthsJson(const QMap<QString,double> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),it.value()); return result;
}
QMap<QString,quint32> objectFontSizes(const QJsonValue &value)
{
   auto sizes=objectSizes(value);
   for (auto size:sizes) if (size>10000) throw std::runtime_error("Label font sizes must be from 0 to 10000 pixels for Qt rendering");
   return sizes;
}
QMap<QString,QString> objectFontPositions(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Label positions must be named strings");
   QMap<QString,QString> result; const auto values=value.toObject();
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      if (!identifier.match(it.key()).hasMatch() || !it.value().isString() || !QStringList{"Top-Right","Top-Left","Bottom-Right","Bottom-Left"}.contains(it.value().toString()))
         throw std::runtime_error("Label positions need valid names and Top-Right, Top-Left, Bottom-Right or Bottom-Left");
      result[it.key()]=it.value().toString();
   }
   return result;
}
QJsonObject objectFontPositionsJson(const QMap<QString,QString> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),it.value()); return result;
}
QMap<QString,QString> objectModelFiles(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Per-object model files must be objects of named path strings");
   QMap<QString,QString> result; const auto values=value.toObject();
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      if (!identifier.match(it.key()).hasMatch() || !it.value().isString())
         throw std::runtime_error("Per-object model files need valid object names and path strings");
      // A blank file restores the resource model, including its transform.
      if (!it.value().toString().isEmpty()) result[it.key()]=it.value().toString();
   }
   return result;
}
QJsonObject objectModelFilesJson(const QMap<QString,QString> &values)
{
   QJsonObject result;
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.cbegin();it!=values.cend();++it) {
      if (!identifier.match(it.key()).hasMatch()) throw std::runtime_error("Per-object model files need valid object names");
      if (!it.value().isEmpty()) result.insert(it.key(),it.value());
   }
   return result;
}
QStringList legacyStringArrayValues(const QString &value)
{
   StringArray entries;
   const bool array=value.startsWith('{') && value.endsWith('}');
   if (!array && (value.startsWith('{') || value.endsWith('}')))
      throw std::runtime_error("expected a string or braced string list");
   if (array) {
      // Match Interpreter::SetProperty's shared STRINGARRAY/OBJECTARRAY
      // tokenization. Each item reaches a scalar/indexed setter rather than
      // a whole-list setter. Quoted commas/spaces and backslashes survive.
      if (value.contains('\'')) entries=GmatStringUtil::SeparateByComma(GmatStringUtil::RemoveOuterString(value.toStdString(),"{","}"));
      else entries=GmatStringUtil::SeparateBrackets(value.toStdString(),"{}"," ,");
   } else entries.push_back(value.toStdString());
   QStringList result;
   for (const auto &entry:entries) {
      auto path=GmatStringUtil::Trim(GmatStringUtil::RemoveEnclosingString(entry,"{}"));
      if (path.empty()) continue; // {} and empty unquoted tokens are no-ops.
      if ((path.front()=='\'' || path.back()=='\'') &&
          (path.size()<2 || path.front()!='\'' || path.back()!='\''))
         throw std::runtime_error("a quoted string needs both enclosing quotes");
      // The legacy interpreter removes one enclosing quote pair only. It
      // does not decode doubled apostrophes or backslash escape sequences.
      result.append(QString::fromStdString(GmatStringUtil::RemoveEnclosingString(path,"'")));
   }
   return result;
}
QString formatVector(const std::array<double,3> &value)
{
   return QString("[%1 %2 %3]").arg(QString::number(value[0],'g',17),QString::number(value[1],'g',17),QString::number(value[2],'g',17));
}
}

QMap<QString,bool> qtObjectFlags(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid per-object display flags: expected named boolean values");
   return objectFlags(document.object());
}
QString qtObjectFlagsJson(const QMap<QString,bool> &flags)
{
   return QString::fromUtf8(QJsonDocument(objectFlagsJson(flags)).toJson(QJsonDocument::Compact));
}

QMap<QString,QString> qtObjectModelFiles(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid per-object model files: expected named path strings");
   return objectModelFiles(document.object());
}
QString qtObjectModelFilesJson(const QMap<QString,QString> &files)
{
   return QString::fromUtf8(QJsonDocument(objectModelFilesJson(files)).toJson(QJsonDocument::Compact));
}

QMap<QString,quint32> qtObjectSizes(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid marker sizes: expected named unsigned integers");
   return objectSizes(document.object());
}
QString qtObjectSizesJson(const QMap<QString,quint32> &sizes)
{
   return QString::fromUtf8(QJsonDocument(objectSizesJson(sizes)).toJson(QJsonDocument::Compact));
}

QMap<QString,double> qtObjectWidths(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid line widths: expected named finite pixel values >= 1");
   return objectWidths(document.object());
}
QString qtObjectWidthsJson(const QMap<QString,double> &widths)
{
   return QString::fromUtf8(QJsonDocument(objectWidthsJson(widths)).toJson(QJsonDocument::Compact));
}

QMap<QString,quint32> qtObjectFontSizes(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid label sizes: expected named integers");
   return objectFontSizes(document.object());
}
QMap<QString,QString> qtObjectFontPositions(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid label positions: expected named strings");
   return objectFontPositions(document.object());
}
QString qtObjectFontPositionsJson(const QMap<QString,QString> &positions)
{
   return QString::fromUtf8(QJsonDocument(objectFontPositionsJson(positions)).toJson(QJsonDocument::Compact));
}

namespace {
QJsonObject resetJson(const QtCameraReset &reset)
{
   auto vector=[](const auto &value) { return QJsonArray{value[0],value[1],value[2]}; };
   return {{"reference",reset.reference},{"target",reset.target},{"automaticTrajectory",reset.automaticTrajectory},
      {"automaticBody",reset.automaticBody},{"segmentFrame",reset.segmentFrame},{"eye",vector(reset.eye)},
      {"center",vector(reset.center)},{"up",vector(reset.up)},{"bodyRelative",reset.bodyRelative},
      {"lookAtRotation",reset.lookAtRotation},{"shortestAngle",reset.shortestAngle}};
}
QtCameraReset resetSetting(const QJsonValue &input)
{
   if (!input.isObject()) throw std::runtime_error("Camera reset must be an object");
   const auto value=input.toObject(); QtCameraReset reset;
   const QSet<QString> fields={"reference","target","automaticTrajectory","automaticBody","segmentFrame","eye","center","up","bodyRelative","lookAtRotation","shortestAngle"};
   for (auto it=value.begin();it!=value.end();++it) if (!fields.contains(it.key())) throw std::runtime_error("Unsupported camera reset field");
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto item:{qMakePair(QString("reference"),&reset.reference),qMakePair(QString("target"),&reset.target),
      qMakePair(QString("automaticTrajectory"),&reset.automaticTrajectory),qMakePair(QString("automaticBody"),&reset.automaticBody)}) {
      if (!value.value(item.first).isString()) throw std::runtime_error("Camera reset references must be strings");
      *item.second=value.value(item.first).toString();
      if (!item.second->isEmpty() && !identifier.match(*item.second).hasMatch()) throw std::runtime_error("Invalid camera reset reference");
   }
   if (!value.value("segmentFrame").isString()) throw std::runtime_error("Camera reset segment must be a string");
   reset.segmentFrame=value.value("segmentFrame").toString();
   if (!reset.segmentFrame.isEmpty() && (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(reset.segmentFrame).hasMatch() ||
       reset.reference!=reset.segmentFrame.section('.',0,0) || !reset.automaticTrajectory.isEmpty())) throw std::runtime_error("Invalid camera reset segment");
   for (auto item:{qMakePair(QString("eye"),&reset.eye),qMakePair(QString("center"),&reset.center),qMakePair(QString("up"),&reset.up)}) {
      const auto array=value.value(item.first).toArray();
      if (array.size()!=3) throw std::runtime_error("Camera reset pose needs three numbers");
      for (int axis=0;axis<3;++axis) {
         if (!array[axis].isDouble() || !std::isfinite(array[axis].toDouble())) throw std::runtime_error("Camera reset pose must be finite");
         (*item.second)[axis]=array[axis].toDouble();
      }
   }
   for (auto item:{qMakePair(QString("bodyRelative"),&reset.bodyRelative),qMakePair(QString("lookAtRotation"),&reset.lookAtRotation),qMakePair(QString("shortestAngle"),&reset.shortestAngle)}) {
      if (!value.value(item.first).isBool()) throw std::runtime_error("Camera reset modes must be booleans");
      *item.second=value.value(item.first).toBool();
   }
   if (!reset.automaticBody.isEmpty() && (reset.automaticBody!=reset.reference || reset.automaticBody=="CoordinateSystem" || !reset.automaticTrajectory.isEmpty())) throw std::runtime_error("Invalid automatic reset body");
   if (!reset.automaticTrajectory.isEmpty() && (reset.bodyRelative || !reset.reference.isEmpty() || (!reset.target.isEmpty() && !reset.lookAtRotation))) throw std::runtime_error("Invalid automatic reset trajectory");
   if ((reset.bodyRelative && (reset.reference.isEmpty() || reset.reference=="CoordinateSystem")) || (reset.lookAtRotation && reset.target.isEmpty())) throw std::runtime_error("Camera reset needs its body or LookAt reference");
   const double up=std::hypot(reset.up[0],reset.up[1],reset.up[2]),distance=std::hypot(reset.eye[0]-reset.center[0],reset.eye[1]-reset.center[1],reset.eye[2]-reset.center[2]);
   if (!std::isfinite(up) || up<1e-12 || !std::isfinite(distance) || (reset.target.isEmpty() && distance<1e-9)) throw std::runtime_error("Camera reset needs a nonzero up and distinct eye/center");
   return reset;
}
QtCameraReset legacyReset(const QMap<QString,QString> &values,const QString &name)
{
   QtCameraReset reset;
   const auto frame=values.value("ViewFrame","CoordinateSystem");
   const bool segment=frame.contains('.'),trajectory=values.value("ViewTrajectory")=="On";
   const bool body=frame!="CoordinateSystem" && (!trajectory || segment);
   if (segment) reset.segmentFrame=frame;
   if (body) reset.reference=frame.section('.',0,0);
   reset.target=values.value("LookAtFrame"); reset.lookAtRotation=!reset.target.isEmpty();
   reset.shortestAngle=values.value("ShortestAngle","Off")=="On";
   reset.bodyRelative=body && values.value("InertialFrame","Off")=="Off";
   if (values.value("SetDefaultLocation")=="On") {
      for (auto component:{qMakePair(QString("Eye"),&reset.eye),qMakePair(QString("Center"),&reset.center),qMakePair(QString("Up"),&reset.up)})
         if (values.contains("Default"+component.first) && !vectorValue(values.value("Default"+component.first),*component.second))
            throw std::runtime_error((name+": invalid default reset camera "+component.first+" vector").toStdString());
   } else {
      reset.eye={0,-30000,0};
      if (body) reset.automaticBody=frame.section('.',0,0);
      else reset.automaticTrajectory=frame;
   }
   return resetSetting(resetJson(reset));
}
}

QtScriptConversion convertOpenFramesViews(const QString &source)
{
   QtScriptConversion result; result.script=source;
   const auto lines=source.split('\n');
   const QRegularExpression declaration("^Create\\s+(OpenFramesInterface|OpenFramesView|OpenFramesVector|OpenFramesSensorMask)\\s+([A-Za-z][A-Za-z0-9_]*(?:\\s+[A-Za-z][A-Za-z0-9_]*)*)\\s*;?$");
   QMap<QString,QString> types;
   QMap<QString,QMap<QString,QString>> properties;
   for (int i=0;i<lines.size();++i) {
      const auto code=codePart(lines[i]);
      if (!QRegularExpression("^Create\\s+OpenFrames").match(code).hasMatch()) continue;
      const auto match=declaration.match(code);
      if (!match.hasMatch()) {
         result.error=QString("Line %1: unsupported OpenFrames declaration. Convert this definition manually.").arg(i+1);
         return result;
      }
      const auto names=match.captured(2).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
      for (const auto &name:names) {
         if (types.contains(name)) {
            result.error=QString("Line %1: duplicate OpenFrames declaration for %2. The original script is unchanged.").arg(i+1).arg(name); return result;
         }
         types[name]=match.captured(1);
      }
   }
   if (types.isEmpty()) return result;
   const QRegularExpression assignment("^(?:GMAT\\s+)?([A-Za-z][A-Za-z0-9_]*)\\.([A-Za-z][A-Za-z0-9_]*)\\s*=\\s*(.+?)\\s*;?$");
   QMap<int,QPair<QString,QString>> settings;
   const QMap<QString,QString> viewAliases={
      {"SetCameraLocation","SetDefaultLocation"}, {"CameraEye","DefaultEye"},
      {"CameraCenter","DefaultCenter"}, {"CameraUp","DefaultUp"}};
   bool mission=false;
   for (int i=0;i<lines.size();++i) {
      auto code=codePart(lines[i]);
      if (code.startsWith("BeginMissionSequence")) mission=true;
      if (declaration.match(code).hasMatch()) {
         if (mission) { result.error="OpenFrames declarations inside the mission require manual conversion."; return result; }
         continue;
      }
      const auto match=assignment.match(code);
      if (match.hasMatch() && types.contains(match.captured(1))) {
         if (mission || match.captured(3).contains(';')) {
            result.error=QString("Line %1: dynamic or compound OpenFrames settings require manual conversion.").arg(i+1); return result;
         }
         if (types.value(match.captured(1))=="OpenFramesInterface" &&
             (match.captured(2)=="DrawYZPlane" || match.captured(2)=="DrawXZPlane" || match.captured(2)=="DrawPlanes")) {
            result.error=match.captured(1)+"."+match.captured(2)+": not a valid OpenFrames guide field. Qt converts per-object DrawGrid and DrawXYPlane; the original script is unchanged."; return result;
         }
         auto value=match.captured(3).trimmed();
         // Legacy camera aliases share the Default storage in OpenFramesView.
         // Normalize while reading source order, so the last alias or canonical
         // assignment wins, while the original spelling remains in comments.
         const auto property=types.value(match.captured(1))=="OpenFramesView"
            ? viewAliases.value(match.captured(2),match.captured(2)) : match.captured(2);
         properties[match.captured(1)][property]=value;
         settings[i]={match.captured(1),match.captured(2)};
      } else {
         // Never remove a view object that is used by a calculation, command or
         // unrecognized continuation. Subscriber Toggle commands remain valid.
         for (auto it=types.cbegin();it!=types.cend();++it) {
            const QRegularExpression reference("\\b"+QRegularExpression::escape(it.key())+"\\b");
            if (reference.match(code).hasMatch() && (it.value()=="OpenFramesView" || it.value()=="OpenFramesVector" || it.value()=="OpenFramesSensorMask")) {
               result.error=QString("Line %1: %2 %3 is referenced outside a supported viewer setting.").arg(i+1).arg(it.value(),it.key()); return result;
            }
         }
      }
   }
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesView") {
      const auto values=properties.value(it.key());
      if (values.value("LookAtFrame").contains('.')) {
         result.error=it.key()+".LookAtFrame: segment-relative LookAt targets are not supported by the OpenFrames object lookup. The original script is unchanged."; return result;
      }
      if (values.value("ViewFrame").contains('.') &&
          !QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(values.value("ViewFrame")).hasMatch()) {
         result.error=it.key()+".ViewFrame: a segment frame must be Object.NamedPropagate. The original script is unchanged."; return result;
      }
      for (const auto *key:{"SetDefaultLocation","SetCurrentLocation","InertialFrame","ViewTrajectory","ShortestAngle"})
         if (values.contains(key) && values.value(key)!="On" && values.value(key)!="Off") {
            result.error=it.key()+"."+key+": expected On or Off."; return result;
         }
   }
   QMap<QString,QtVectorSetting> vectorSettings;
   const QRegularExpression vectorIdentifier("^[A-Za-z][A-Za-z0-9_]*$");
   auto stringValue=[](QString value) {
      if (value.startsWith('\'') && value.endsWith('\'') && value.size()>=2) {
         value=value.mid(1,value.size()-2); value.replace("''","'");
      }
      return value;
   };
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesVector") {
      const auto values=properties.value(it.key()); QtVectorSetting vector; vector.name=it.key();
      const QSet<QString> supported={"SourceObject","DestinationObject","VectorType","BFStartPoint","BFDirection","VectorColor","VectorLabel","VectorLengthType","VectorLength"};
      for (auto property=values.cbegin();property!=values.cend();++property) if (!supported.contains(property.key())) {
         result.error=it.key()+"."+property.key()+": unsupported OpenFramesVector field; original source retained."; return result;
      }
      vector.source=stringValue(values.value("SourceObject")); vector.destination=stringValue(values.value("DestinationObject"));
      vector.type=stringValue(values.value("VectorType","Body-Fixed")); vector.label=stringValue(values.value("VectorLabel","DefaultVector"));
      if (vector.type!="Relative Position" && vector.type!="Body-Fixed") {
         result.error=it.key()+".VectorType: Qt supports Relative Position and Body-Fixed; "+vector.type+" is not implemented. The original script is unchanged."; return result;
      }
      const auto lengthType=stringValue(values.value("VectorLengthType","Auto"));
      if (lengthType!="Auto" && lengthType!="Manual") { result.error=it.key()+".VectorLengthType: expected Auto or Manual."; return result; }
      vector.automaticLength=lengthType=="Auto";
      bool validLength=false; vector.length=values.value("VectorLength","1").toDouble(&validLength);
      if (!validLength || !std::isfinite(vector.length) || vector.length<=0 || vector.length>std::numeric_limits<float>::max()) {
         result.error=it.key()+".VectorLength: expected a finite positive length within the renderer's range."; return result;
      }
      for (auto field:{qMakePair(QString("BFStartPoint"),&vector.start),qMakePair(QString("BFDirection"),&vector.direction)})
         if (values.contains(field.first) && !vectorValue(values.value(field.first),*field.second)) {
            result.error=it.key()+"."+field.first+": expected three finite numbers."; return result;
         }
      if (vector.type=="Body-Fixed" && (!std::isfinite(std::hypot(vector.direction[0],vector.direction[1],vector.direction[2])) || std::hypot(vector.direction[0],vector.direction[1],vector.direction[2])<1e-12)) {
         result.error=it.key()+".BFDirection: expected a nonzero direction."; return result;
      }
      try { vector.color=RgbColor::ToIntColor(stringValue(values.value("VectorColor","Red")).toStdString()); }
      catch (BaseException &) { result.error=it.key()+".VectorColor: expected a GMAT color name or an RGB integer triplet from 0 to 255."; return result; }
      vectorSettings.insert(it.key(),vector);
   }
   QMap<QString,QtSensorMaskSetting> maskSettings;
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesSensorMask") {
      const auto values=properties.value(it.key()); QtSensorMaskSetting mask; mask.name=it.key();
      const QSet<QString> supported={"Source","Hardware","Label","LengthType","Length"};
      for (auto property=values.cbegin();property!=values.cend();++property) if (!supported.contains(property.key())) {
         result.error=it.key()+"."+property.key()+": unsupported OpenFramesSensorMask field; original source retained."; return result;
      }
      mask.source=stringValue(values.value("Source")); mask.hardware=stringValue(values.value("Hardware"));
      mask.label=stringValue(values.value("Label","DefaultSensorMask"));
      const auto lengthType=stringValue(values.value("LengthType","Auto"));
      if (lengthType!="Auto" && lengthType!="Manual") { result.error=it.key()+".LengthType: choose Auto or Manual."; return result; }
      mask.automaticLength=lengthType=="Auto";
      bool valid=false; mask.length=values.value("Length","1").toDouble(&valid);
      if (!valid || !std::isfinite(mask.length) || mask.length<=0 || mask.length>std::numeric_limits<float>::max()) {
         result.error=it.key()+".Length: expected a finite positive axial length within the renderer's range."; return result;
      }
      maskSettings.insert(it.key(),mask);
   }
   QMap<QString,QtCameraSetting> displaySettings;
   QMap<QString,QStringList> plottedNames;
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesInterface") {
      QStringList names; int fontPositionIndex=0,modelFileIndex=0; auto &display=displaySettings[it.key()];
      QString solverMode="All"; int solverCount=1;
      auto &stars=display.stars.emplace();
      auto &playback=display.playback.emplace(); playback.timeScale=3600; playback.showToolbar=true;
      for (auto line=settings.cbegin();line!=settings.cend();++line) {
         if (line->first!=it.key()) continue;
         const auto property=line->second,value=assignment.match(codePart(lines[line.key()])).captured(3).trimmed();
         if (property=="PlaybackTimeScale") {
            bool ok=false; const auto scale=value.toDouble(&ok);
            if (!ok || !std::isfinite(scale)) { result.error=it.key()+".PlaybackTimeScale: expected a finite simulated-seconds-per-wall-second scale."; return result; }
            playback.timeScale=scale;
         } else if (property=="TimeSyncParent") {
            playback.syncParent=stringValue(value);
            if ((!playback.syncParent.isEmpty() && !vectorIdentifier.match(playback.syncParent).hasMatch()) || playback.syncParent==it.key()) { result.error=it.key()+".TimeSyncParent: choose another plot name or an empty string for independent clock time."; return result; }
         } else if (property=="ShowToolbar") {
            const auto flag=value.toLower();
            if (flag!="true" && flag!="false") { result.error=it.key()+".ShowToolbar: expected true or false."; return result; }
            playback.showToolbar=flag=="true";
         } else if (property=="StarSettings") {
            stars.preset=stringValue(value);
            if (!QStringList{"Monitor","Printer","Virtual Reality","Custom"}.contains(stars.preset)) { result.error=it.key()+".StarSettings: choose Monitor, Printer, Virtual Reality or Custom."; return result; }
         } else if (property=="StarCatalog") {
            stars.catalog=stringValue(value);
            if (stars.catalog.isEmpty()) { result.error=it.key()+".StarCatalog: expected a nonempty catalog filename."; return result; }
         } else if (property=="StarCount") {
            bool ok=false; const auto count=value.toLongLong(&ok);
            if (!ok || count<=0 || count>std::numeric_limits<int>::max() || !QRegularExpression("^[+]?[0-9]+$").match(value).hasMatch()) { result.error=it.key()+".StarCount: expected a positive integer count within the renderer's range."; return result; }
            stars.customCount=static_cast<int>(count);
         } else if (property=="MinStarMag" || property=="MaxStarMag" || property=="MinStarPixels" || property=="MaxStarPixels" || property=="MinStarDimRatio") {
            bool ok=false; const double number=value.toDouble(&ok);
            if (!ok || !std::isfinite(number) || std::abs(number)>std::numeric_limits<float>::max()) { result.error=it.key()+"."+property+": expected a finite real value within the renderer's range."; return result; }
            if ((property=="MinStarMag" && number>20) || (property=="MaxStarMag" && number<stars.minMagnitude) ||
                (property=="MinStarPixels" && number<=0) || (property=="MaxStarPixels" && number<stars.minPixels) ||
                (property=="MinStarDimRatio" && (number<0 || number>1))) { result.error=it.key()+"."+property+": violates the legacy source-ordered star bounds."; return result; }
            if (property=="MinStarMag") stars.minMagnitude=number;
            else if (property=="MaxStarMag") stars.maxMagnitude=number;
            else if (property=="MinStarPixels") stars.minPixels=number;
            else if (property=="MaxStarPixels") stars.maxPixels=number;
            else stars.minDimRatio=number;
         } else if (property=="SolverIterations") {
            solverMode=stringValue(value);
            if (!QStringList{"All","Current","None","LastN"}.contains(solverMode)) { result.error=it.key()+".SolverIterations: choose All, Current, None or LastN."; return result; }
            if (solverMode=="All" || solverMode=="None") solverCount=0;
            else if (solverMode=="Current") solverCount=1;
         } else if (property=="SolverIterLastN") {
            bool ok=false; const int count=value.toInt(&ok);
            if (!ok || count<0 || !QRegularExpression("^[+]?[0-9]+$").match(value).hasMatch()) { result.error=it.key()+".SolverIterLastN: expected a nonnegative integer."; return result; }
            // Legacy setters apply N only while LastN is already selected.
            if (solverMode=="LastN") solverCount=count;
         } else if (property=="AxesLabels" || property=="FrameLabel") {
            const auto flag=stringValue(value);
            if (flag!="On" && flag!="Off") { result.error=it.key()+"."+property+": expected On or Off."; return result; }
            (property=="AxesLabels" ? display.axesLabels : display.frameLabel)=flag=="On";
         } else if (property=="Vector") {
            const bool array=value.startsWith('{') && value.endsWith('}');
            if (!array && !vectorIdentifier.match(value).hasMatch()) { result.error=it.key()+".Vector: expected a vector name or braced list."; return result; }
            if (array) display.vectors.clear();
            const auto selected=array ? value.mid(1,value.size()-2).split(',') : QStringList{value};
            for (auto vectorName:selected) {
               vectorName=vectorName.trimmed(); if (vectorName.isEmpty()) continue;
               if (!vectorIdentifier.match(vectorName).hasMatch() || !vectorSettings.contains(vectorName)) { result.error=it.key()+".Vector: unknown OpenFramesVector "+vectorName+"."; return result; }
               const auto vector=vectorSettings.value(vectorName);
               if (!vectorIdentifier.match(vector.source).hasMatch() || (vector.type=="Relative Position" && !vectorIdentifier.match(vector.destination).hasMatch())) {
                  result.error=vector.name+": selected vector requires a SourceObject and, for Relative Position, a DestinationObject."; return result;
               }
               bool duplicate=false; for (const auto &existing:display.vectors) if (existing.name==vectorName) duplicate=true;
               if (!duplicate) display.vectors.append(vector);
            }
         } else if (property=="SensorMask") {
            const bool array=value.startsWith('{') && value.endsWith('}');
            if (!array && !vectorIdentifier.match(value).hasMatch()) { result.error=it.key()+".SensorMask: expected a mask name or braced list."; return result; }
            if (array) display.sensorMasks.clear();
            const auto selected=array ? value.mid(1,value.size()-2).split(',') : QStringList{value};
            for (auto maskName:selected) {
               maskName=maskName.trimmed(); if (maskName.isEmpty()) continue;
               if (!vectorIdentifier.match(maskName).hasMatch() || !maskSettings.contains(maskName)) { result.error=it.key()+".SensorMask: unknown OpenFramesSensorMask "+maskName+"."; return result; }
               const auto mask=maskSettings.value(maskName);
               if (!vectorIdentifier.match(mask.source).hasMatch() || !vectorIdentifier.match(mask.hardware).hasMatch()) {
                  result.error=mask.name+": selected mask requires a bare Source object and Hardware name."; return result;
               }
               bool duplicate=false; for (const auto &existing:display.sensorMasks) if (existing.name==maskName) duplicate=true;
               if (!duplicate) display.sensorMasks.append(mask);
            }
         } else if (property=="Add") {
            QStringList additions;
            try { additions=legacyStringArrayValues(value); }
            catch (BaseException &error) { result.error=it.key()+".Add: "+QString::fromStdString(error.GetFullMessage()); return result; }
            catch (const std::exception &error) { result.error=it.key()+".Add: "+QString::fromUtf8(error.what()); return result; }
            // Interpreter sends each OBJECTARRAY item to the indexed setter.
            // The actual OF/OrbitView script route appends unique names. It
            // does not call the separate whole-list setters that clear state.
            for (const auto &name:additions) {
               if (name.isEmpty()) continue;
               if (!vectorIdentifier.match(name).hasMatch()) { result.error=it.key()+".Add: expected SpacePoint names."; return result; }
               if (names.contains(name)) continue;
               names.append(name);
               display.objectLabels[name]=true; display.objectTrajectories[name]=true; display.objectCenters[name]=true; display.objectEndpoints[name]=true; display.objectMarkerSizes[name]=10; display.objectLineWidths[name]=2; display.objectFontSizes[name]=14; display.objectFontPositions[name]="Top-Right"; display.objectAxes[name]=false; display.objectGrids[name]=false; display.objectXYPlanes[name]=false; display.objectVelocities[name]=false; display.objectSegmentLabels[name]=false;
            }
         } else if (property=="DrawModelFile") {
            QStringList files;
            try { files=legacyStringArrayValues(value); }
            catch (BaseException &error) { result.error=it.key()+".DrawModelFile: "+QString::fromStdString(error.GetFullMessage()); return result; }
            catch (const std::exception &error) { result.error=it.key()+".DrawModelFile: "+QString::fromUtf8(error.what()); return result; }
            for (const auto &file:files) {
               if (names.isEmpty() || modelFileIndex>=names.size()) {
                  result.error=it.key()+".DrawModelFile: no selected object at the legacy scalar model-file index; the original script is unchanged."; return result;
               }
               if (file.isEmpty()) display.objectModelFiles.remove(names[modelFileIndex]);
               else display.objectModelFiles[names[modelFileIndex]]=file;
               // This counter survives additional Add assignments.
               modelFileIndex=(modelFileIndex+1)%names.size();
            }
         } else if (property=="DrawLabel" || property=="DrawTrajectory" || property=="DrawCenterPoint" || property=="DrawEndPoints" || property=="DrawAxes" || property=="DrawGrid" || property=="DrawXYPlane" || property=="DrawVelocity" || property=="DrawUsePropLabel") {
            if (!value.startsWith('[') || !value.endsWith(']')) { result.error=it.key()+"."+property+": expected a bracketed true/false array."; return result; }
            const auto flags=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            auto &destination=property=="DrawLabel" ? display.objectLabels : property=="DrawTrajectory" ? display.objectTrajectories : property=="DrawCenterPoint" ? display.objectCenters : property=="DrawEndPoints" ? display.objectEndpoints : property=="DrawAxes" ? display.objectAxes : property=="DrawGrid" ? display.objectGrids : property=="DrawXYPlane" ? display.objectXYPlanes : property=="DrawVelocity" ? display.objectVelocities : display.objectSegmentLabels;
            for (int i=0;i<flags.size();++i) {
               if (flags[i]!="true" && flags[i]!="false") { result.error=it.key()+"."+property+": expected true or false."; return result; }
               // OF applies the available prefix and leaves omitted objects at defaults.
               if (i<names.size()) destination[names[i]]=flags[i]=="true";
            }
         } else if (property=="DrawLineWidth") {
            if (!value.startsWith('[') || !value.endsWith(']')) { result.error=it.key()+".DrawLineWidth: expected a bracketed real array."; return result; }
            const auto widths=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            for (int i=0;i<widths.size();++i) {
               bool ok=false; const double width=widths[i].toDouble(&ok);
               if (!ok || !std::isfinite(width) || (i<names.size() && (width<1 || width>std::numeric_limits<float>::max()))) { result.error=it.key()+".DrawLineWidth: selected objects need finite widths >= 1 within the renderer's range."; return result; }
               // OF's bulk real setter validates/applies only the selected prefix.
               if (i<names.size()) display.objectLineWidths[names[i]]=width;
            }
         } else if (property=="DrawMarkerSize" || property=="DrawFontSize") {
            if (!value.startsWith('[') || !value.endsWith(']')) { result.error=it.key()+"."+property+": expected a bracketed unsigned integer array."; return result; }
            const auto sizes=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            if (sizes.size()>names.size()) { result.error=it.key()+"."+property+": a size index is outside the selected objects."; return result; }
            for (int i=0;i<sizes.size();++i) {
               bool ok=false; const auto size=sizes[i].toULongLong(&ok);
               if (!ok || size>4294967295ULL || !QRegularExpression("^[0-9]+$").match(sizes[i]).hasMatch()) { result.error=it.key()+"."+property+": expected unsigned integer pixel sizes."; return result; }
               if (property=="DrawFontSize" && size>10000) { result.error=it.key()+".DrawFontSize: Qt supports label sizes from 0 to 10000 pixels; original source retained."; return result; }
               if (i<names.size()) (property=="DrawFontSize" ? display.objectFontSizes : display.objectMarkerSizes)[names[i]]=static_cast<quint32>(size);
            }
         } else if (property=="DrawFontPosition") {
            const bool array=value.startsWith('{') && value.endsWith('}');
            if (!array && (value.startsWith('{') || value.endsWith('}'))) { result.error=it.key()+".DrawFontPosition: malformed position array."; return result; }
            const auto inner=array ? value.mid(1,value.size()-2).trimmed() : value;
            const auto positions=inner.isEmpty() && array ? QStringList() : inner.split(',');
            if (!array && (positions.size()!=1 || names.isEmpty() || fontPositionIndex>=names.size())) { result.error=it.key()+".DrawFontPosition: no selected object for the scalar position."; return result; }
            for (int i=0;i<positions.size();++i) {
               // OF's string-array setter applies only the selected prefix.
               if (array && i>=names.size()) break;
               auto position=positions[i].trimmed();
               if (position.startsWith('\'') && position.endsWith('\'') && position.size()>=2) position=position.mid(1,position.size()-2);
               if (!QStringList{"Top-Right","Top-Left","Bottom-Right","Bottom-Left"}.contains(position)) { result.error=it.key()+".DrawFontPosition: choose Top-Right, Top-Left, Bottom-Right or Bottom-Left; original source retained."; return result; }
               const int index=array ? i : fontPositionIndex;
               if (index<names.size()) display.objectFontPositions[names[index]]=position;
            }
            if (!array) fontPositionIndex=(fontPositionIndex+1)%names.size();
         }
      }
      try { (void)qtStars(qtStarsJson(display.stars)); (void)qtPlayback(qtPlaybackJson(display.playback)); }
      catch (const std::exception &error) { result.error=it.key()+": "+QString::fromUtf8(error.what()); return result; }
      if (solverMode=="LastN") display.solverLastN=solverCount;
      plottedNames[it.key()]=names;
   }
   const QSet<QString> common={"Add","CoordinateSystem","DrawObject","SolverIterations","Maximized","ShowPlot","Size","UpperLeft",
      "RelativeZOrder","Axes","XYPlane","EclipticPlane","EnableStars","StarCount","EnableConstellations",
      "DataCollectFrequency","UpdatePlotFrequency","MaxPlotPoints"};
   QStringList output;
   for (int i=0;i<lines.size();++i) {
      const auto declared=declaration.match(codePart(lines[i]));
      if (declared.hasMatch()) {
         if (declared.captured(1)=="OpenFramesInterface") {
            // Keep an original inline declaration comment, including grouped names.
            if (lines[i].contains('%')) output.append("% Qt conversion: "+lines[i]);
            output.append("Create OrbitView "+declared.captured(2)+"; % Converted from OpenFramesInterface for Qt");
            result.plots+=declared.captured(2).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts).size();
         } else output.append("% Qt conversion: "+lines[i]);
         continue;
      }
      if (!settings.contains(i)) { output.append(lines[i]); continue; }
      const auto name=settings[i].first,key=settings[i].second;
      if (types[name]=="OpenFramesInterface" && key=="SolverIterations" && stringValue(assignment.match(codePart(lines[i])).captured(3).trimmed())=="LastN") {
         output.append("% Qt conversion: "+lines[i]);
         output.append(name+".SolverIterations = All; % LastN retention is applied by the Qt viewer");
         result.notes.append(name+".SolverIterations: LastN trajectory retention retained in Qt metadata.");
      } else if (types[name]=="OpenFramesInterface" && key=="SolverIterLastN") {
         output.append("% Qt conversion: "+lines[i]);
         result.notes.append(name+".SolverIterLastN: source-ordered count retained when LastN is selected; zero is unlimited.");
      } else if (types[name]=="OpenFramesInterface" && common.contains(key)) output.append(lines[i]);
      else if (types[name]=="OpenFramesInterface" && (key=="DrawLabel" || key=="DrawTrajectory" || key=="DrawCenterPoint" || key=="DrawEndPoints" || key=="DrawMarkerSize" || key=="DrawLineWidth" || key=="DrawFontSize" || key=="DrawFontPosition" || key=="DrawAxes" || key=="DrawGrid" || key=="DrawXYPlane" || key=="DrawVelocity" || key=="DrawUsePropLabel" || key=="DrawModelFile" || key=="AxesLabels" || key=="FrameLabel")) {
         output.append("% Qt conversion: "+lines[i]);
         result.notes.append(name+"."+key+((key=="AxesLabels" || key=="FrameLabel") ? ": coordinate label setting retained in Qt metadata." : ": independent per-object drawing retained in Qt metadata."));
      } else {
         output.append("% Qt conversion: "+lines[i]);
         if (types[name]=="OpenFramesInterface" && QStringList{"PlaybackTimeScale","TimeSyncParent","ShowToolbar"}.contains(key)) result.notes.append(name+"."+key+": clock synchronization and toolbar settings retained in Qt metadata.");
         else if (types[name]=="OpenFramesInterface" && QStringList{"StarSettings","StarCatalog","MinStarMag","MaxStarMag","MinStarPixels","MaxStarPixels","MinStarDimRatio"}.contains(key)) result.notes.append(name+"."+key+": preset/custom star options retained in Qt metadata.");
         else if (types[name]=="OpenFramesInterface" && key=="SensorMask") result.notes.append(name+".SensorMask: FOV-backed sensor masks retained in Qt metadata.");
         else if (types[name]=="OpenFramesInterface" && key=="Vector") result.notes.append(name+".Vector: source/destination pointing, body-fixed pose, color/label and Auto/Manual length retained in Qt metadata.");
         else if (types[name]=="OpenFramesInterface" && key!="View") result.notes.append(name+"."+key+": retained as a comment; no direct Qt equivalent.");
      }
   }
   QStringList cameras;
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesInterface") {
      const auto plot=it.key();
      QStringList viewNames;
      const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
      for (auto line=settings.cbegin();line!=settings.cend();++line) {
         if (line->first!=plot || line->second!="View") continue;
         const auto value=assignment.match(codePart(lines[line.key()])).captured(3).trimmed();
         const bool array=value.startsWith('{') && value.endsWith('}');
         if (!array && !identifier.match(value).hasMatch()) { result.error=plot+": invalid camera view list."; return result; }
         // OpenFramesInterface::SetStringParameter(VIEW) clears a braced list,
         // while scalar settings append. AddView ignores blanks and duplicates.
         if (array) viewNames.clear();
         const auto entries=array ? value.mid(1,value.size()-2).split(',') : QStringList{value};
         for (auto entry:entries) {
            entry=entry.trimmed(); if (entry.isEmpty()) continue;
            if (!identifier.match(entry).hasMatch()) { result.error=plot+": invalid camera view list."; return result; }
            if (!viewNames.contains(entry)) viewNames.append(entry);
         }
      }
      if (viewNames.isEmpty()) {
         result.notes.append(plot+": no selected view; review the default OrbitView camera.");
         cameras.append(qtCameraDirective(plot,displaySettings.value(plot)).trimmed()); continue;
      }
      for (const auto &viewName:viewNames) {
         if (types.value(viewName)!="OpenFramesView") {
            result.error=plot+": unknown camera view "+viewName+"."; return result;
         }
         const auto viewSettings=properties.value(viewName);
         if (viewSettings.value("ViewTrajectory")=="On" && viewSettings.value("ViewFrame","CoordinateSystem")!="CoordinateSystem") {
            if (!plottedNames.value(plot).contains(viewSettings.value("ViewFrame").section('.',0,0))) {
               result.error=viewName+": trajectory frame must name an object in "+plot+".Add or CoordinateSystem."; return result;
            }
         }
      }
      const auto view=viewNames.first(); const auto values=properties.value(view);
      bool validFov=false;
      const double fov=values.value("FOVy","45").toDouble(&validFov);
      if (!validFov || !std::isfinite(fov) || fov<1 || fov>150) {
         result.error=plot+": field of view must be a finite number from 1 to 150 degrees for Qt conversion."; return result;
      }
      QtCameraSetting cameraSetting{true,fov};
      cameraSetting.vectors=displaySettings.value(plot).vectors;
      cameraSetting.sensorMasks=displaySettings.value(plot).sensorMasks;
      cameraSetting.stars=displaySettings.value(plot).stars;
      cameraSetting.playback=displaySettings.value(plot).playback;
      cameraSetting.objectLabels=displaySettings.value(plot).objectLabels;
      cameraSetting.objectTrajectories=displaySettings.value(plot).objectTrajectories;
      cameraSetting.objectCenters=displaySettings.value(plot).objectCenters;
      cameraSetting.objectEndpoints=displaySettings.value(plot).objectEndpoints;
      cameraSetting.objectAxes=displaySettings.value(plot).objectAxes;
      cameraSetting.objectGrids=displaySettings.value(plot).objectGrids;
      cameraSetting.objectXYPlanes=displaySettings.value(plot).objectXYPlanes;
      cameraSetting.objectVelocities=displaySettings.value(plot).objectVelocities;
      cameraSetting.objectModelFiles=displaySettings.value(plot).objectModelFiles;
      cameraSetting.objectSegmentLabels=displaySettings.value(plot).objectSegmentLabels;
      cameraSetting.axesLabels=displaySettings.value(plot).axesLabels;
      cameraSetting.frameLabel=displaySettings.value(plot).frameLabel;
      cameraSetting.solverLastN=displaySettings.value(plot).solverLastN;
      cameraSetting.objectMarkerSizes=displaySettings.value(plot).objectMarkerSizes;
      cameraSetting.objectLineWidths=displaySettings.value(plot).objectLineWidths;
      cameraSetting.objectFontSizes=displaySettings.value(plot).objectFontSizes;
      cameraSetting.objectFontPositions=displaySettings.value(plot).objectFontPositions;
      const auto frame=values.value("ViewFrame","CoordinateSystem");
      const bool trajectory=values.value("ViewTrajectory")=="On";
      const bool segment=frame.contains('.');
      const auto frameObject=frame.section('.',0,0);
      const bool body=frame!="CoordinateSystem" && (!trajectory || segment);
      if (segment) cameraSetting.segmentFrame=frame;
      cameraSetting.bodyRelative=body && values.value("InertialFrame","Off")=="Off";
      auto set=[&](const QString &key,const QString &value) { cameras.append("GMAT "+plot+"."+key+" = "+value+";"); };
      set("ViewPointReference",body ? frameObject : "[0 0 0]");
      set("ViewDirection",body ? frameObject : "[0 0 0]");
      const auto prefix=values.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
      const bool stored=values.value("Set"+prefix+"Location")=="On";
      if (((!body && trajectory) || frame=="CoordinateSystem") && !stored) {
         cameraSetting.automaticTrajectory=frame;
         set("ViewPointVector","[0 -30000 0]"); cameraSetting.up=std::array<double,3>{0,0,1};
      }
      // OF returns the moving empty segment frame for either ViewTrajectory mode.
      if (body && !stored) {
         cameraSetting.automaticBody=frameObject;
         set("ViewPointVector","[0 -30000 0]"); cameraSetting.up=std::array<double,3>{0,0,1};
      }
      std::array<double,3> eye{0,-1,0},center{},up{0,0,1};
      if (stored) for (auto component:{qMakePair(QString("Eye"),&eye),qMakePair(QString("Center"),&center),qMakePair(QString("Up"),&up)}) {
         if (values.contains(prefix+component.first) && !vectorValue(values.value(prefix+component.first),*component.second)) {
            result.error=plot+": invalid stored camera "+component.first+" vector."; return result;
         }
      }
      if (stored) set("ViewPointVector",formatVector(eye));
      else if (!cameraSetting.automaticBody.isEmpty()) result.notes.append(plot+(segment ? ": automatic segment camera uses empty frame bounds." : ": automatic body camera uses the rendered body/model bounds."));
      if (stored) {
         if (!body) set("ViewDirection",formatVector(center));
         else cameraSetting.centerOffset=center;
      }
      if (values.contains("LookAtFrame")) {
         set("ViewDirection",values["LookAtFrame"]=="CoordinateSystem" ? "[0 0 0]" : values["LookAtFrame"]);
         cameraSetting.lookAtRotation=true; cameraSetting.shortestAngle=values.value("ShortestAngle","Off")=="On";
         cameraSetting.centerOffset=center;
      }
      if (stored) {
         int axis=0; for (int i=1;i<3;++i) if (std::abs(up[i])>std::abs(up[axis])) axis=i;
         QString signedAxis=up[axis]<0 ? "-" : ""; signedAxis+="XYZ"[axis]; set("ViewUpAxis",signedAxis);
         if (!std::isfinite(std::hypot(up[0],up[1],up[2])) || std::hypot(up[0],up[1],up[2])<1e-12) {
            result.error=plot+": camera up vector must be nonzero."; return result;
         }
         cameraSetting.up=up;
         result.notes.append(plot+": exact camera up vector retained in Qt metadata; the base viewer uses "+signedAxis+".");
      }
      cameraSetting.primaryName=view;
      try { cameraSetting.reset=legacyReset(values,view); }
      catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); return result; }
      for (int index=1;index<viewNames.size();++index) {
         QtCameraPreset preset; preset.name=viewNames[index];
         const auto extra=properties.value(preset.name);
         bool valid=false; preset.fieldOfView=extra.value("FOVy","45").toDouble(&valid);
         if (!valid || !std::isfinite(preset.fieldOfView) || preset.fieldOfView<1 || preset.fieldOfView>150) {
            result.error=preset.name+": field of view must be a finite number from 1 to 150 degrees."; return result;
         }
         const auto extraFrame=extra.value("ViewFrame","CoordinateSystem");
         const bool trajectory=extra.value("ViewTrajectory")=="On";
         const bool segment=extraFrame.contains('.');
         const auto extraObject=extraFrame.section('.',0,0);
         if (segment) preset.segmentFrame=extraFrame;
         if (extraFrame!="CoordinateSystem" && (!trajectory || segment)) preset.reference=extraObject;
         preset.target=extra.value("LookAtFrame");
         preset.lookAtRotation=!preset.target.isEmpty(); preset.shortestAngle=extra.value("ShortestAngle","Off")=="On";
         preset.bodyRelative=!preset.reference.isEmpty() && extra.value("InertialFrame","Off")=="Off";
         const auto location=extra.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
         if (extra.value("Set"+location+"Location")=="On") {
            // OF defaults for omitted stored components.
            preset.eye={0,-1,0};
            for (auto component:{qMakePair(QString("Eye"),&preset.eye),qMakePair(QString("Center"),&preset.center),qMakePair(QString("Up"),&preset.up)}) {
               if (extra.contains(location+component.first) && !vectorValue(extra.value(location+component.first),*component.second)) {
                  result.error=preset.name+": invalid stored camera "+component.first+" vector."; return result;
               }
            }
         } else if ((!segment && trajectory) || extraFrame=="CoordinateSystem") preset.automaticTrajectory=extraFrame;
         else { preset.automaticBody=extraObject; result.notes.append(preset.name+(segment ? ": automatic segment camera uses empty frame bounds." : ": automatic body camera uses the rendered body/model bounds.")); }
         try { preset.reset=legacyReset(extra,preset.name); }
         catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); return result; }
         cameraSetting.views.append(preset);
         if (segment) result.notes.append(preset.name+": segment camera follows the first retained named arc and clamps to its endpoint; automatic framing uses the empty segment-frame bounds.");
         if (preset.automaticTrajectory=="CoordinateSystem") result.notes.append(preset.name+": automatic origin framing retains the OpenFrames default radius and viewport-aware distance.");
         if (!preset.reference.isEmpty()) result.notes.append(preset.name+(preset.bodyRelative ? ": camera follows the object position and orientation." : ": camera follows the object position using plot-frame axes."));
         if (!preset.target.isEmpty()) result.notes.append(preset.name+": two-frame look-at orientation is retained, including ShortestAngle/AZEL rotation.");
         if (trajectory && !segment) result.notes.append(preset.name+(preset.automaticTrajectory.isEmpty() ? ": stored whole-trajectory camera retained in the plot frame; it does not follow the moving object." : ": automatic whole-trajectory camera uses retained trajectory bounds, refreshed as points arrive."));
      }
      if (!cameraSetting.segmentFrame.isEmpty()) result.notes.append(plot+": segment camera follows the first retained named arc and clamps to its endpoint; automatic framing uses the empty segment-frame bounds.");
      try { qtCameraSettings(qtCameraDirective(plot,cameraSetting)); }
      catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); return result; }
      cameras.append(qtCameraDirective(plot,cameraSetting).trimmed());
      if (cameraSetting.automaticTrajectory=="CoordinateSystem") result.notes.append(plot+": automatic origin framing retains the OpenFrames default radius and viewport-aware distance.");
      set("ViewUpCoordinateSystem",properties[plot].value("CoordinateSystem","EarthMJ2000Eq"));
      result.notes.append(plot+": camera selector retains "+QString::number(viewNames.size())+" named views.");
      if (trajectory && !segment) result.notes.append(plot+(stored ? ": stored whole-trajectory camera retained in the plot frame; it does not follow the moving object." : ": automatic whole-trajectory camera uses retained trajectory bounds, refreshed as points arrive."));
      if (body) result.notes.append(plot+(cameraSetting.bodyRelative ? ": camera follows the object position and orientation." : ": camera follows the object position using plot-frame axes."));
      if (values.contains("LookAtFrame")) result.notes.append(plot+": two-frame look-at orientation is retained, including ShortestAngle/AZEL rotation.");
   }
   int insertion=output.size();
   for (int i=0;i<output.size();++i) if (codePart(output[i]).startsWith("BeginMissionSequence")) { insertion=i; break; }
   for (int i=cameras.size()-1;i>=0;--i) output.insert(insertion,cameras[i]);
   result.notes.removeDuplicates();
   QStringList preamble={"% OpenFrames viewer conversion for Qt. Review these visual differences before saving:"};
   for (const auto &note:result.notes) preamble.append("% "+note);
   result.script=(preamble+output).join('\n');
   return result;
}

QString qtCameraDirective(const QString &plot,const QtCameraSetting &setting)
{
   QJsonObject object{{"plot",plot},{"perspective",setting.perspective},{"fieldOfView",setting.fieldOfView}};
   if (setting.axesLabels) object.insert("axesLabels",*setting.axesLabels);
   if (setting.frameLabel) object.insert("frameLabel",*setting.frameLabel);
   if (setting.solverLastN) object.insert("solverLastN",*setting.solverLastN);
   if (setting.stars) object.insert("stars",QJsonDocument::fromJson(qtStarsJson(setting.stars).toUtf8()).object());
   if (setting.playback) {
      const auto playback=qtPlaybackJson(setting.playback);
      if (playback!="null") object.insert("playback",QJsonDocument::fromJson(playback.toUtf8()).object());
   }
   if (setting.reset) object.insert("reset",resetJson(*setting.reset));
   if (setting.up) object.insert("up",QJsonArray{(*setting.up)[0],(*setting.up)[1],(*setting.up)[2]});
   if (setting.bodyRelative) object.insert("bodyRelative",true);
   if (setting.lookAtRotation) { object.insert("lookAtRotation",true); object.insert("shortestAngle",setting.shortestAngle); }
   if (setting.centerOffset) object.insert("centerOffset",QJsonArray{(*setting.centerOffset)[0],(*setting.centerOffset)[1],(*setting.centerOffset)[2]});
   if (!setting.primaryName.isEmpty()) object.insert("primaryName",setting.primaryName);
   if (!setting.automaticTrajectory.isEmpty()) object.insert("automaticTrajectory",setting.automaticTrajectory);
   if (!setting.automaticBody.isEmpty()) object.insert("automaticBody",setting.automaticBody);
   if (!setting.segmentFrame.isEmpty()) object.insert("segmentFrame",setting.segmentFrame);
   if (!setting.objectLabels.isEmpty()) object.insert("objectLabels",objectFlagsJson(setting.objectLabels));
   if (!setting.objectTrajectories.isEmpty()) object.insert("objectTrajectories",objectFlagsJson(setting.objectTrajectories));
   if (!setting.objectCenters.isEmpty()) object.insert("objectCenters",objectFlagsJson(setting.objectCenters));
   if (!setting.objectEndpoints.isEmpty()) object.insert("objectEndpoints",objectFlagsJson(setting.objectEndpoints));
   if (!setting.objectAxes.isEmpty()) object.insert("objectAxes",objectFlagsJson(setting.objectAxes));
   if (!setting.objectGrids.isEmpty()) object.insert("objectGrids",objectFlagsJson(setting.objectGrids));
   if (!setting.objectXYPlanes.isEmpty()) object.insert("objectXYPlanes",objectFlagsJson(setting.objectXYPlanes));
   if (!setting.objectSegmentLabels.isEmpty()) object.insert("objectSegmentLabels",objectFlagsJson(setting.objectSegmentLabels));
   if (!setting.objectVelocities.isEmpty()) object.insert("objectVelocities",objectFlagsJson(setting.objectVelocities));
   if (!setting.objectMarkerSizes.isEmpty()) object.insert("objectMarkerSizes",objectSizesJson(setting.objectMarkerSizes));
   if (!setting.objectLineWidths.isEmpty()) object.insert("objectLineWidths",objectWidthsJson(setting.objectLineWidths));
   if (!setting.objectFontSizes.isEmpty()) object.insert("objectFontSizes",objectSizesJson(setting.objectFontSizes));
   if (!setting.objectFontPositions.isEmpty()) object.insert("objectFontPositions",objectFontPositionsJson(setting.objectFontPositions));
   const auto modelFiles=objectModelFilesJson(setting.objectModelFiles);
   if (!modelFiles.isEmpty()) object.insert("objectModelFiles",modelFiles);
   if (!setting.sensorMasks.isEmpty()) object.insert("sensorMasks",QJsonDocument::fromJson(qtSensorMasksJson(setting.sensorMasks).toUtf8()).array());
   if (!setting.vectors.isEmpty()) {
      QJsonArray vectors;
      for (const auto &vector:setting.vectors) vectors.append(QJsonObject{
         {"name",vector.name},{"source",vector.source},{"destination",vector.destination},{"type",vector.type},
         {"label",vector.label},{"color",double(vector.color)},{"automaticLength",vector.automaticLength},{"length",vector.length},
         {"start",QJsonArray{vector.start[0],vector.start[1],vector.start[2]}},
         {"direction",QJsonArray{vector.direction[0],vector.direction[1],vector.direction[2]}}});
      object.insert("vectors",vectors);
   }
   if (!setting.views.isEmpty()) {
      QJsonArray views;
      auto vector=[](const std::array<double,3> &v) { return QJsonArray{v[0],v[1],v[2]}; };
      for (const auto &view:setting.views) {
         QJsonObject named{{"name",view.name},{"reference",view.reference},{"target",view.target},
            {"eye",vector(view.eye)},{"center",vector(view.center)},{"up",vector(view.up)},{"automaticTrajectory",view.automaticTrajectory},{"automaticBody",view.automaticBody},
            {"perspective",view.perspective},{"fieldOfView",view.fieldOfView},{"bodyRelative",view.bodyRelative},{"lookAtRotation",view.lookAtRotation},{"shortestAngle",view.shortestAngle}};
         if (view.reset) named.insert("reset",resetJson(*view.reset));
         if (!view.segmentFrame.isEmpty()) named.insert("segmentFrame",view.segmentFrame);
         views.append(named);
      }
      object.insert("views",views);
   }
   return "% GMAT-Qt-Camera "+QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact))+"\n";
}
QMap<QString,QtCameraSetting> qtCameraSettings(const QString &source)
{
   QMap<QString,QtCameraSetting> result;
   for (const auto &line:source.split('\n')) {
      const auto text=line.trimmed(); const QString prefix="% GMAT-Qt-Camera ";
      if (!text.startsWith(prefix)) continue;
      QJsonParseError error;
      const auto document=QJsonDocument::fromJson(text.mid(prefix.size()).toUtf8(),&error);
      const auto object=document.object(); const auto name=object.value("plot").toString();
      const auto fov=object.value("fieldOfView").toDouble(-1);
      if (error.error!=QJsonParseError::NoError || !document.isObject() ||
          !QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch() ||
          !object.value("perspective").isBool() || !std::isfinite(fov) || fov<1 || fov>150 || result.contains(name))
         throw std::runtime_error("Invalid or duplicate GMAT-Qt-Camera comment: expected a plot name, perspective boolean and fieldOfView from 1 to 150 degrees");
      QtCameraSetting setting{object.value("perspective").toBool(),fov};
      for (const auto &key:QStringList{"axesLabels","frameLabel"}) if (object.contains(key)) {
         if (!object.value(key).isBool()) throw std::runtime_error("Coordinate label settings must be booleans");
         (key=="axesLabels" ? setting.axesLabels : setting.frameLabel)=object.value(key).toBool();
      }
      if (object.contains("playback")) {
         if (object.value("playback").isNull()) setting.playback.reset();
         else {
            if (!object.value("playback").isObject()) throw std::runtime_error("Plot playback must be an object or null");
            setting.playback=qtPlayback(QString::fromUtf8(QJsonDocument(object.value("playback").toObject()).toJson(QJsonDocument::Compact)));
            if (setting.playback && setting.playback->syncParent==name) throw std::runtime_error("A plot cannot synchronize playback with itself");
         }
      }
      if (object.contains("stars")) {
         if (object.value("stars").isNull()) setting.stars.reset();
         else {
            if (!object.value("stars").isObject()) throw std::runtime_error("Plot stars must be an object or null");
            setting.stars=qtStars(QString::fromUtf8(QJsonDocument(object.value("stars").toObject()).toJson(QJsonDocument::Compact)));
         }
      }
      if (object.contains("solverLastN")) {
         const auto count=object.value("solverLastN"); const double n=count.toDouble(-1);
         if (!count.isDouble() || n<0 || n>std::numeric_limits<int>::max() || std::floor(n)!=n)
            throw std::runtime_error("LastN solver retention requires a nonnegative integer count");
         setting.solverLastN=static_cast<int>(n);
      }
      if (object.contains("reset")) setting.reset=resetSetting(object.value("reset"));
      if (object.contains("up")) {
         const auto array=object.value("up").toArray();
         std::array<double,3> up{};
         bool valid=array.size()==3;
         for (int i=0;valid && i<3;++i) {
            up[i]=array[i].toDouble(); valid=array[i].isDouble() && std::isfinite(up[i]);
         }
         if (!valid || !std::isfinite(std::hypot(up[0],up[1],up[2])) || std::hypot(up[0],up[1],up[2])<1e-12)
            throw std::runtime_error("Invalid GMAT-Qt-Camera up vector: expected three finite numbers and a nonzero length");
         setting.up=up;
      }
      if (object.contains("bodyRelative") && !object.value("bodyRelative").isBool()) throw std::runtime_error("Camera bodyRelative must be a boolean");
      setting.bodyRelative=object.value("bodyRelative").toBool();
      for (const auto *key:{"lookAtRotation","shortestAngle"})
         if (object.contains(key) && !object.value(key).isBool()) throw std::runtime_error("Camera alignment modes must be booleans");
      setting.lookAtRotation=object.value("lookAtRotation").toBool(); setting.shortestAngle=object.value("shortestAngle").toBool();
      if (object.contains("centerOffset")) {
         const auto array=object.value("centerOffset").toArray(); std::array<double,3> center{};
         if (array.size()!=3) throw std::runtime_error("Camera centerOffset must have three numbers");
         for (int i=0;i<3;++i) {
            if (!array[i].isDouble() || !std::isfinite(array[i].toDouble())) throw std::runtime_error("Camera centerOffset must have finite numbers");
            center[i]=array[i].toDouble();
         }
         setting.centerOffset=center;
      }
      const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
      if (object.contains("sensorMasks")) {
         if (!object.value("sensorMasks").isArray()) throw std::runtime_error("Plot sensorMasks must be an array");
         setting.sensorMasks=qtSensorMasks(QString::fromUtf8(QJsonDocument(object.value("sensorMasks").toArray()).toJson(QJsonDocument::Compact)));
      }
      if (object.contains("vectors")) {
         if (!object.value("vectors").isArray()) throw std::runtime_error("Plot vectors must be an array");
         QSet<QString> names;
         auto finiteVector=[](const QJsonValue &value) {
            const auto array=value.toArray(); std::array<double,3> result{};
            if (array.size()!=3) throw std::runtime_error("Plot vector pose needs three numbers");
            for (int axis=0;axis<3;++axis) {
               if (!array[axis].isDouble() || !std::isfinite(array[axis].toDouble())) throw std::runtime_error("Plot vector pose must be finite");
               result[axis]=array[axis].toDouble();
            }
            return result;
         };
         for (const auto &item:object.value("vectors").toArray()) {
            if (!item.isObject()) throw std::runtime_error("Plot vector definition must be an object");
            const auto value=item.toObject(); QtVectorSetting vector;
            const QSet<QString> fields={"name","source","destination","type","label","color","automaticLength","length","start","direction"};
            for (auto field=value.begin();field!=value.end();++field) if (!fields.contains(field.key())) throw std::runtime_error("Unsupported plot vector metadata field");
            vector.name=value.value("name").toString(); vector.source=value.value("source").toString();
            vector.destination=value.value("destination").toString(); vector.type=value.value("type").toString();
            if (!identifier.match(vector.name).hasMatch() || names.contains(vector.name) || !identifier.match(vector.source).hasMatch() ||
                !value.value("destination").isString() || (!vector.destination.isEmpty() && !identifier.match(vector.destination).hasMatch()) ||
                (vector.type!="Relative Position" && vector.type!="Body-Fixed") || (vector.type=="Relative Position" && vector.destination.isEmpty()))
               throw std::runtime_error("Invalid or duplicate plot vector: check names, source/destination and Relative Position or Body-Fixed mode");
            const auto color=value.value("color").toDouble(-1);
            vector.length=value.value("length").toDouble(-1);
            if (!value.value("color").isDouble() || color<0 || color>0xffffff || std::floor(color)!=color ||
                !value.value("length").isDouble() || !std::isfinite(vector.length) || vector.length<=0 || vector.length>std::numeric_limits<float>::max() ||
                !value.value("automaticLength").isBool() || !value.value("label").isString())
               throw std::runtime_error("Plot vector needs an RGB color, label, automaticLength boolean and finite positive length");
            vector.color=static_cast<quint32>(color); vector.label=value.value("label").toString(); vector.automaticLength=value.value("automaticLength").toBool();
            vector.start=finiteVector(value.value("start")); vector.direction=finiteVector(value.value("direction"));
            if (vector.type=="Body-Fixed" && (!std::isfinite(std::hypot(vector.direction[0],vector.direction[1],vector.direction[2])) || std::hypot(vector.direction[0],vector.direction[1],vector.direction[2])<1e-12))
               throw std::runtime_error("Body-Fixed plot vector direction must be nonzero");
            names.insert(vector.name); setting.vectors.append(vector);
         }
      }
      if (object.contains("objectLabels")) setting.objectLabels=objectFlags(object.value("objectLabels"));
      if (object.contains("objectTrajectories")) setting.objectTrajectories=objectFlags(object.value("objectTrajectories"));
      if (object.contains("objectCenters")) setting.objectCenters=objectFlags(object.value("objectCenters"));
      if (object.contains("objectEndpoints")) setting.objectEndpoints=objectFlags(object.value("objectEndpoints"));
      if (object.contains("objectAxes")) setting.objectAxes=objectFlags(object.value("objectAxes"));
      if (object.contains("objectGrids")) setting.objectGrids=objectFlags(object.value("objectGrids"));
      if (object.contains("objectXYPlanes")) setting.objectXYPlanes=objectFlags(object.value("objectXYPlanes"));
      if (object.contains("objectSegmentLabels")) setting.objectSegmentLabels=objectFlags(object.value("objectSegmentLabels"));
      if (object.contains("objectVelocities")) setting.objectVelocities=objectFlags(object.value("objectVelocities"));
      if (object.contains("objectMarkerSizes")) setting.objectMarkerSizes=objectSizes(object.value("objectMarkerSizes"));
      if (object.contains("objectLineWidths")) setting.objectLineWidths=objectWidths(object.value("objectLineWidths"));
      if (object.contains("objectFontSizes")) setting.objectFontSizes=objectFontSizes(object.value("objectFontSizes"));
      if (object.contains("objectFontPositions")) setting.objectFontPositions=objectFontPositions(object.value("objectFontPositions"));
      if (object.contains("objectModelFiles")) setting.objectModelFiles=objectModelFiles(object.value("objectModelFiles"));
      if (object.contains("segmentFrame")) {
         setting.segmentFrame=object.value("segmentFrame").toString();
         if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(setting.segmentFrame).hasMatch() || object.contains("automaticTrajectory"))
            throw std::runtime_error("Segment camera frame must be Object.NamedPropagate and cannot frame a whole trajectory");
      }
      if (object.contains("automaticBody")) {
         setting.automaticBody=object.value("automaticBody").toString();
         if (!identifier.match(setting.automaticBody).hasMatch() || setting.automaticBody=="CoordinateSystem" || object.contains("automaticTrajectory"))
            throw std::runtime_error("Automatic body camera needs one object name and cannot also frame a trajectory");
      }
      if (object.contains("automaticTrajectory")) {
         setting.automaticTrajectory=object.value("automaticTrajectory").toString();
         if (!identifier.match(setting.automaticTrajectory).hasMatch() || setting.bodyRelative)
            throw std::runtime_error("Automatic trajectory camera needs an object name without body-relative rotation");
      }
      if (object.contains("primaryName")) {
         setting.primaryName=object.value("primaryName").toString();
         if (!identifier.match(setting.primaryName).hasMatch()) throw std::runtime_error("Invalid primary camera name");
      }
      if (object.contains("views")) {
         if (!object.value("views").isArray()) throw std::runtime_error("Camera views must be an array");
         QSet<QString> names; names.insert(setting.primaryName);
         auto vector=[](const QJsonValue &value) {
            const auto array=value.toArray(); std::array<double,3> result{};
            if (array.size()!=3) throw std::runtime_error("Camera vector must have three numbers");
            for (int i=0;i<3;++i) {
               if (!array[i].isDouble() || !std::isfinite(array[i].toDouble())) throw std::runtime_error("Camera vector must have finite numbers");
               result[i]=array[i].toDouble();
            }
            return result;
         };
         for (const auto &item:object.value("views").toArray()) {
            const auto value=item.toObject(); QtCameraPreset view;
            if (value.contains("reset")) view.reset=resetSetting(value.value("reset"));
            view.name=value.value("name").toString(); view.reference=value.value("reference").toString(); view.target=value.value("target").toString();
            view.automaticTrajectory=value.value("automaticTrajectory").toString();
            view.automaticBody=value.value("automaticBody").toString();
            if (value.contains("segmentFrame")) {
               view.segmentFrame=value.value("segmentFrame").toString();
               if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(view.segmentFrame).hasMatch() ||
                   view.reference!=view.segmentFrame.section('.',0,0) || !view.automaticTrajectory.isEmpty())
                  throw std::runtime_error("Named segment camera must reference Object.NamedPropagate and cannot frame a whole trajectory");
            }
            if (value.contains("automaticBody") && (!value.value("automaticBody").isString() || (!view.automaticBody.isEmpty() &&
                (!identifier.match(view.automaticBody).hasMatch() || view.automaticBody=="CoordinateSystem" || !view.automaticTrajectory.isEmpty() || view.automaticBody!=view.reference))))
               throw std::runtime_error("Automatic body camera must match its object reference and cannot also frame a trajectory");
            if (value.contains("automaticTrajectory") && (!value.value("automaticTrajectory").isString() ||
                (!view.automaticTrajectory.isEmpty() && !identifier.match(view.automaticTrajectory).hasMatch())))
               throw std::runtime_error("Invalid automatic trajectory object name");
            view.fieldOfView=value.value("fieldOfView").toDouble(-1); view.perspective=value.value("perspective").toBool();
            if (!identifier.match(view.name).hasMatch() || names.contains(view.name) ||
                !value.value("reference").isString() || (!view.reference.isEmpty() && !identifier.match(view.reference).hasMatch()) ||
                !value.value("target").isString() || (!view.target.isEmpty() && !identifier.match(view.target).hasMatch()) ||
                !value.value("perspective").isBool() || !std::isfinite(view.fieldOfView) || view.fieldOfView<1 || view.fieldOfView>150)
               throw std::runtime_error("Invalid or duplicate named camera: check names, reference objects, projection and field of view");
            if (value.contains("bodyRelative") && !value.value("bodyRelative").isBool()) throw std::runtime_error("Named camera bodyRelative must be a boolean");
            view.bodyRelative=value.value("bodyRelative").toBool();
            for (const auto *key:{"lookAtRotation","shortestAngle"})
               if (value.contains(key) && !value.value(key).isBool()) throw std::runtime_error("Named camera alignment modes must be booleans");
            view.lookAtRotation=value.value("lookAtRotation").toBool(); view.shortestAngle=value.value("shortestAngle").toBool();
            if (!view.automaticTrajectory.isEmpty() && (view.bodyRelative || !view.reference.isEmpty() || (!view.target.isEmpty() && !view.lookAtRotation)))
               throw std::runtime_error("Automatic trajectory camera needs a plot-frame reference and LookAt rotation for any target");
            if (view.lookAtRotation && view.target.isEmpty()) throw std::runtime_error("Aligned camera needs a target");
            if (view.bodyRelative && (view.reference.isEmpty() || view.reference=="CoordinateSystem")) throw std::runtime_error("Body-relative camera needs an object reference");
            view.eye=vector(value.value("eye")); view.center=vector(value.value("center")); view.up=vector(value.value("up"));
            const auto upLength=std::hypot(view.up[0],view.up[1],view.up[2]);
            const auto distance=std::hypot(view.eye[0]-view.center[0],view.eye[1]-view.center[1],view.eye[2]-view.center[2]);
            if (!std::isfinite(upLength) || upLength<1e-12 || !std::isfinite(distance) || (view.target.isEmpty() && distance<1e-9))
               throw std::runtime_error("Named camera needs a nonzero up vector and distinct eye and center");
            names.insert(view.name); setting.views.append(view);
         }
      }
      result.insert(name,setting);
   }
   return result;
}
QString retainQtCameraSettings(const QString &original,const QString &candidate,const QString &removedPlot)
{
   const auto before=qtCameraSettings(original),after=qtCameraSettings(candidate);
   QString result=candidate;
   for (auto it=before.cbegin();it!=before.cend();++it)
      if (it.key()!=removedPlot && !after.contains(it.key())) result.prepend(qtCameraDirective(it.key(),it.value()));
   return result;
}

QString removeQtCameraSetting(const QString &source,const QString &plot)
{
   qtCameraSettings(source); // Retain the existing ambiguity/format validation.
   const QRegularExpression line("^[ \\t]*% GMAT-Qt-Camera [^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption);
   auto matches=line.globalMatch(source);
   while (matches.hasNext()) {
      const auto match=matches.next();
      if (qtCameraSettings(match.captured()).contains(plot)) {
         auto result=source; result.remove(match.capturedStart(),match.capturedLength()); return result;
      }
   }
   return source;
}

QString setQtCameraSetting(const QString &source,const QString &plot,const QtCameraSetting &setting)
{
   qtCameraSettings(source); // Reject ambiguity rather than overwriting malformed settings.
   const auto directive=qtCameraDirective(plot,setting);
   qtCameraSettings(directive);
   const QRegularExpression line("^[ \t]*% GMAT-Qt-Camera [^\n]*(?:\n|$)",QRegularExpression::MultilineOption);
   auto matches=line.globalMatch(source);
   while (matches.hasNext()) {
      const auto match=matches.next();
      const auto existing=qtCameraSettings(match.captured());
      if (existing.contains(plot)) {
         // Preserve the authored bytes of a semantic no-op, including explicit
         // empty model defaults that normalize out of canonical metadata.
         if (qtCameraDirective(plot,existing.value(plot))==directive) return source;
         auto result=source; result.replace(match.capturedStart(),match.capturedLength(),directive); return result;
      }
   }
   return directive+source;
}

namespace {
QPair<qsizetype,qsizetype> arrayBlock(const QString &source,const QString &name)
{
   const QString prefix="^[ \t]*% GMAT-Qt-Array-Expressions "+QRegularExpression::escape(name);
   const QRegularExpression beginPattern(prefix+" begin[ \t]*(?:\n|$)",QRegularExpression::MultilineOption);
   const QRegularExpression endPattern(prefix+" end[ \t]*(?:\n|$)",QRegularExpression::MultilineOption);
   const auto begin=beginPattern.match(source),end=endPattern.match(source);
   if (!begin.hasMatch() && !end.hasMatch()) return {-1,0};
   if (!begin.hasMatch() || !end.hasMatch() || end.capturedStart()<begin.capturedEnd() ||
       beginPattern.match(source,begin.capturedEnd()).hasMatch() || endPattern.match(source,end.capturedEnd()).hasMatch())
      throw std::runtime_error("Array expression block is incomplete or duplicated; repair its begin/end comments before editing");
   return {begin.capturedStart(),end.capturedEnd()-begin.capturedStart()};
}
}
QString arrayExpressions(const QString &source,const QString &name)
{
   const auto block=arrayBlock(source,name); QJsonArray cells;
   if (block.first<0) return "[]";
   const auto lines=source.mid(block.first,block.second).split('\n');
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*\\)\\s*=\\s*([^;\\n]+);\\s*$");
   for (int i=1;i<lines.size();++i) {
      if (lines[i].trimmed()=="% GMAT-Qt-Array-Expressions "+name+" end") break;
      const auto match=assignment.match(lines[i]);
      if (!match.hasMatch()) throw std::runtime_error("Array expression block contains an unsupported statement; edit it in the script");
      cells.append(QJsonObject{{"row",match.captured(1).toInt()},{"column",match.captured(2).toInt()},{"expression",match.captured(3).trimmed()}});
   }
   return QString::fromUtf8(QJsonDocument(cells).toJson(QJsonDocument::Compact));
}
QString setArrayExpressions(const QString &source,const QString &name,const QString &cells,int rows,int columns)
{
   const auto block=arrayBlock(source,name);
   if (block.first>=0) arrayExpressions(source,name); // Never erase unrecognized commands.
   QJsonParseError error; const auto document=QJsonDocument::fromJson(cells.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isArray()) throw std::runtime_error("Invalid array expression grid");
   QMap<QPair<int,int>,QString> expressions;
   for (const auto &value:document.array()) {
      const auto cell=value.toObject(); const auto row=cell.value("row").toInt(-1),col=cell.value("column").toInt(-1);
      const auto expression=cell.value("expression").toString().trimmed();
      if (row<1 || row>rows || col<1 || col>columns || expression.isEmpty() ||
          expression.contains(QRegularExpression("[;\r\n%]")) || expressions.contains({row,col}))
         throw std::runtime_error("Each expression needs a unique in-range cell and a single formula without semicolons or comments");
      expressions.insert({row,col},expression);
   }
   QString replacement;
   if (!expressions.isEmpty()) {
      replacement="% GMAT-Qt-Array-Expressions "+name+" begin\n";
      for (auto it=expressions.cbegin();it!=expressions.cend();++it)
         replacement+=QString("GMAT %1(%2,%3) = %4;\n").arg(name).arg(it.key().first).arg(it.key().second).arg(it.value());
      replacement+="% GMAT-Qt-Array-Expressions "+name+" end\n";
   }
   auto result=source;
   if (block.first>=0) { result.replace(block.first,block.second,replacement); return result; }
   if (replacement.isEmpty()) return result;
   const auto mission=QRegularExpression("^[ \t]*BeginMissionSequence[ \t]*;?[^\n]*(?:\n|$)",QRegularExpression::MultilineOption).match(source);
   if (!mission.hasMatch()) throw std::runtime_error("Build a mission sequence before adding array expressions");
   const auto position=mission.capturedEnd();
   result.insert(position,(position && source[position-1]!='\n' ? "\n" : "")+replacement);
   return result;
}

std::optional<bool> qtDrawingFlag(const QString &value)
{
   if (value=="Default") return std::nullopt;
   if (value=="On") return true;
   if (value=="Off") return false;
   throw std::runtime_error("Choose Default, On or Off for coordinate labels");
}
QString qtDrawingFlagText(std::optional<bool> value)
{
   return value ? (*value ? "On" : "Off") : "Default";
}

QVector<QtSensorMaskSetting> qtSensorMasks(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isArray()) throw std::runtime_error("Sensor masks must be a JSON array");
   QVector<QtSensorMaskSetting> result; QSet<QString> names;
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   const QSet<QString> fields={"name","source","hardware","label","automaticLength","length"};
   for (const auto &item:document.array()) {
      if (!item.isObject()) throw std::runtime_error("A sensor mask must be an object");
      const auto value=item.toObject();
      for (auto field=value.constBegin();field!=value.constEnd();++field) if (!fields.contains(field.key())) throw std::runtime_error("Unsupported sensor mask metadata field");
      QtSensorMaskSetting mask;
      for (auto pair:{qMakePair(QString("name"),&mask.name),qMakePair(QString("source"),&mask.source),qMakePair(QString("hardware"),&mask.hardware),qMakePair(QString("label"),&mask.label)}) {
         if (!value.value(pair.first).isString()) throw std::runtime_error("Sensor mask name, source, hardware and label must be strings");
         *pair.second=value.value(pair.first).toString();
      }
      if (!identifier.match(mask.name).hasMatch() || names.contains(mask.name) || !identifier.match(mask.source).hasMatch() || !identifier.match(mask.hardware).hasMatch())
         throw std::runtime_error("Sensor masks need unique valid names and bare source/hardware names");
      mask.length=value.value("length").toDouble(-1);
      if (!value.value("automaticLength").isBool() || !value.value("length").isDouble() || !std::isfinite(mask.length) || mask.length<=0 || mask.length>std::numeric_limits<float>::max())
         throw std::runtime_error("Sensor mask length must be finite and positive, with a boolean automaticLength mode");
      mask.automaticLength=value.value("automaticLength").toBool();
      names.insert(mask.name); result.append(mask);
   }
   return result;
}
QString qtSensorMasksJson(const QVector<QtSensorMaskSetting> &masks)
{
   QJsonArray result;
   for (const auto &mask:masks) result.append(QJsonObject{{"name",mask.name},{"source",mask.source},{"hardware",mask.hardware},{"label",mask.label},{"automaticLength",mask.automaticLength},{"length",mask.length}});
   return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
}

std::optional<QtStarSetting> qtStars(const QString &json)
{
   if (json.trimmed()=="null") return {};
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Star options must be a JSON object or null");
   const auto value=document.object();
   const QSet<QString> fields={"preset","catalog","customCount","minMagnitude","maxMagnitude","minPixels","maxPixels","minDimRatio"};
   for (auto field=value.constBegin();field!=value.constEnd();++field) if (!fields.contains(field.key())) throw std::runtime_error("Unsupported star options field");
   QtStarSetting setting;
   if (!value.value("preset").isString() || !QStringList{"Monitor","Printer","Virtual Reality","Custom"}.contains(value.value("preset").toString()) ||
       !value.value("catalog").isString() || value.value("catalog").toString().isEmpty()) throw std::runtime_error("Star options need a valid preset and nonempty raw catalog filename");
   setting.preset=value.value("preset").toString(); setting.catalog=value.value("catalog").toString();
   const double count=value.value("customCount").toDouble(-1);
   if (!value.value("customCount").isDouble() || count<=0 || count>std::numeric_limits<int>::max() || std::floor(count)!=count) throw std::runtime_error("Custom star count must be a positive integer within the renderer's range");
   setting.customCount=static_cast<int>(count);
   for (auto field:{qMakePair(QString("minMagnitude"),&setting.minMagnitude),qMakePair(QString("maxMagnitude"),&setting.maxMagnitude),qMakePair(QString("minPixels"),&setting.minPixels),qMakePair(QString("maxPixels"),&setting.maxPixels),qMakePair(QString("minDimRatio"),&setting.minDimRatio)}) {
      const auto number=value.value(field.first);
      if (!number.isDouble() || !std::isfinite(number.toDouble()) || std::abs(number.toDouble())>std::numeric_limits<float>::max()) throw std::runtime_error("Star bounds must be finite real values within the renderer's range");
      *field.second=number.toDouble();
   }
   if (setting.minMagnitude>20 || setting.maxMagnitude<setting.minMagnitude || setting.minPixels<=0 || setting.maxPixels<setting.minPixels || setting.minDimRatio<0 || setting.minDimRatio>1)
      throw std::runtime_error("Star options require min magnitude <= max, positive min pixels <= max, and dim ratio from 0 to 1");
   return setting;
}
QString qtStarsJson(const std::optional<QtStarSetting> &setting)
{
   if (!setting) return "null";
   return QString::fromUtf8(QJsonDocument(QJsonObject{{"preset",setting->preset},{"catalog",setting->catalog},{"customCount",setting->customCount},
      {"minMagnitude",setting->minMagnitude},{"maxMagnitude",setting->maxMagnitude},{"minPixels",setting->minPixels},{"maxPixels",setting->maxPixels},{"minDimRatio",setting->minDimRatio}}).toJson(QJsonDocument::Compact));
}

std::optional<QtPlaybackSetting> qtPlayback(const QString &json)
{
   if (json.trimmed()=="null") return {};
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Playback options must be a JSON object or null");
   const auto value=document.object(); const QSet<QString> fields={"timeScale","syncParent","showToolbar"};
   for (auto field=value.constBegin();field!=value.constEnd();++field) if (!fields.contains(field.key())) throw std::runtime_error("Unsupported playback options field");
   if (!value.contains("timeScale") || !value.contains("syncParent") || !value.contains("showToolbar") || !value.value("syncParent").isString())
      throw std::runtime_error("Playback options require a scale or null, a parent string, and a toolbar boolean or null");
   QtPlaybackSetting setting;
   if (!value.value("timeScale").isNull()) {
      if (!value.value("timeScale").isDouble() || !std::isfinite(value.value("timeScale").toDouble())) throw std::runtime_error("Playback scale must be a finite signed number or null");
      setting.timeScale=value.value("timeScale").toDouble();
   }
   if (!value.value("showToolbar").isNull()) {
      if (!value.value("showToolbar").isBool()) throw std::runtime_error("Playback toolbar setting must be boolean or null");
      setting.showToolbar=value.value("showToolbar").toBool();
   }
   setting.syncParent=value.value("syncParent").toString();
   if (!setting.syncParent.isEmpty() && (!setting.timeScale || !QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(setting.syncParent).hasMatch()))
      throw std::runtime_error("A playback parent requires a clock scale and a valid plot name");
   if (!setting.timeScale && setting.syncParent.isEmpty() && !setting.showToolbar) return {};
   return setting;
}
QString qtPlaybackJson(const std::optional<QtPlaybackSetting> &setting)
{
   if (!setting) return "null";
   if (setting->timeScale && !std::isfinite(*setting->timeScale)) throw std::runtime_error("Playback scale must be finite");
   if (!setting->timeScale && setting->syncParent.isEmpty() && !setting->showToolbar) return "null";
   return QString::fromUtf8(QJsonDocument(QJsonObject{{"timeScale",setting->timeScale ? QJsonValue(*setting->timeScale) : QJsonValue(QJsonValue::Null)},
      {"syncParent",setting->syncParent},{"showToolbar",setting->showToolbar ? QJsonValue(*setting->showToolbar) : QJsonValue(QJsonValue::Null)}}).toJson(QJsonDocument::Compact));
}
