#include "CustomFovProperties.hpp"
#include "ResourcePreview.hpp"
#include "ScriptStatements.hpp"
#include "CustomFOV.hpp"
#include "FileManager.hpp"
#include "UtilityException.hpp"
#include "Rvector.hpp"
#include <QFileInfo>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>
#include <optional>

namespace {
const QSet<QString> geometryFields{"@CustomFovSource","@CustomFovCoordinates","FOVFileName","ConeAngles","ClockAngles","Azimuth","Elevation","InterpolationStepSize"};
class FovPathsGuard
{
public:
   FovPathsGuard(): files(FileManager::Instance()),state(files->CaptureState()) { }
   ~FovPathsGuard() { files->RestoreState(*state); }
   FovPathsGuard(const FovPathsGuard &)=delete;
   FovPathsGuard &operator=(const FovPathsGuard &)=delete;
private:
   FileManager *files;
   std::shared_ptr<const FileManager::State> state;
};
QString vectorText(GmatBase &object,const QString &name)
{
   const Rvector values=object.GetRvectorParameter(name.toStdString()); QStringList cells;
   for (int i=0;i<values.GetSize();++i) cells.append(QString::number(values[i],'g',17));
   return cells.join(' ');
}
QVector<double> angles(const QString &text)
{
   auto input=text.trimmed();
   if (input.startsWith('[') && input.endsWith(']')) input=input.mid(1,input.size()-2).trimmed();
   const auto cells=input.split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
   if (cells.size()<3 || cells.size()>100000) throw std::runtime_error("Enter between 3 and 100000 paired FOV vertices.");
   QVector<double> result;
   for (const auto &cell:cells) {
      bool valid=false; const auto value=cell.toDouble(&valid);
      if (!valid || !std::isfinite(value)) throw std::runtime_error("Every FOV vertex needs two finite angles.");
      result.append(value);
   }
   return result;
}
}

QString customFovCoordinates(const GmatBase &object,const QString &script)
{
   QString result="ConeClock";
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(QString::fromStdString(object.GetName()))+
      "\\.(ConeAngles|ClockAngles|Azimuth|Elevation)\\s*=");
   for (const auto &statement:scriptStatements(script)) {
      if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch()) break;
      const auto match=assignment.match(statement.code);
      if (match.hasMatch()) result=(match.captured(1)=="Azimuth" || match.captured(1)=="Elevation") ? "AzimuthElevation" : "ConeClock";
   }
   return result;
}

QString customFovEditedCoordinates(const QMap<QString,QString> &values,const QString &fallback)
{
   if (values.contains("@CustomFovCoordinates")) return values.value("@CustomFovCoordinates");
   if (!values.value("Azimuth").trimmed().isEmpty() || !values.value("Elevation").trimmed().isEmpty()) return "AzimuthElevation";
   if (!values.value("ConeAngles").trimmed().isEmpty() || !values.value("ClockAngles").trimmed().isEmpty()) return "ConeClock";
   return fallback;
}
QSet<QString> applyCustomFovProperties(GmatBase &object,const QMap<QString,QString> &values,const QString &fallback)
{
   auto *fov=dynamic_cast<CustomFOV *>(&object); if (!fov) return {};
   bool changed=false; for (auto it=values.cbegin();it!=values.cend();++it) changed=changed || geometryFields.contains(it.key());
   if (!changed) return {};
   const auto file=values.value("FOVFileName",QString::fromStdString(fov->GetStringParameter("FOVFileName"))).trimmed();
   const auto mode=values.value("@CustomFovSource",file.isEmpty() ? "Vertices" : "File");
   const auto coordinates=customFovEditedCoordinates(values,fallback);
   if (mode!="Vertices" && mode!="File") throw std::runtime_error("Choose paired vertices or an FOV file.");
   if (coordinates!="ConeClock" && coordinates!="AzimuthElevation") throw std::runtime_error("Choose cone/clock or azimuth/elevation coordinates.");
   // A fresh object clears file-derived vectors, initialization/interpolation
   // caches and the old coordinate mode. Indexed setters supply the engine's
   // range checks and grow each paired vector without the fixed-size Rvector
   // Elevation setter or stale initialized mask cache.
   FovPathsGuard paths;
   auto *files=FileManager::Instance();
   std::optional<std::string> configuredPath;
   try { configuredPath=files->GetPathDefinition("FOV_DATA_PATH"); }
   catch (const UtilityException &) { }
   CustomFOV replacement(fov->GetName());
   // Use the existing path expression when configured. Otherwise the engine's
   // temporary default supports first-use validation; the guard restores the
   // original state (including absence) on either acceptance or rejection.
   if (configuredPath) files->AddFileType("FOV_DATA_PATH",*configuredPath);
   replacement.SetStringParameter("Color",fov->GetStringParameter("Color"));
   replacement.SetUnsignedIntParameter("Alpha",fov->GetUnsignedIntParameter("Alpha"));
   bool valid=false;
   const auto interpolation=values.value("InterpolationStepSize",QString::number(fov->GetRealParameter("InterpolationStepSize"),'g',17)).toDouble(&valid);
   if (!valid || !std::isfinite(interpolation)) throw std::runtime_error("Enter a finite interpolation step in degrees.");
   replacement.SetRealParameter("InterpolationStepSize",interpolation);
   if (mode=="File") {
      if (file.isEmpty()) throw std::runtime_error("Choose a readable FOV input file.");
      for (const auto &name:QStringList{"ConeAngles","ClockAngles","Azimuth","Elevation"})
         if (values.contains(name) && !values.value(name).trimmed().isEmpty()) throw std::runtime_error("Choose an FOV file or explicit paired angles; do not define both.");
      if (file.contains(QRegularExpression("['%\\x00-\\x1f]"))) throw std::runtime_error("Choose an FOV filename without apostrophes, percent signs or control characters.");
      const auto resolved=QString::fromStdString(FileManager::Instance()->FindPath(file.toStdString(),"FOV_DATA_PATH",true,false,false,fov->GetName()));
      if (!QFileInfo(resolved).isFile() || !QFileInfo(resolved).isReadable()) throw std::runtime_error("Choose a readable FOV input file.");
      replacement.SetStringParameter("FOVFileName",file.toStdString());
   } else {
      if (!file.isEmpty()) throw std::runtime_error("Clear the FOV file before defining paired vertices.");
      const bool cone=coordinates=="ConeClock";
      const QString first=cone ? "ConeAngles" : "Azimuth",second=cone ? "ClockAngles" : "Elevation";
      for (const auto &name:QStringList{cone ? "Azimuth" : "ConeAngles",cone ? "Elevation" : "ClockAngles"})
         if (values.contains(name) && !values.value(name).trimmed().isEmpty()) throw std::runtime_error("Use one FOV angle representation at a time.");
      const auto a=angles(values.value(first,vectorText(*fov,first))),b=angles(values.value(second,vectorText(*fov,second)));
      if (a.size()!=b.size()) throw std::runtime_error("Each FOV angle needs a paired angle in the same row.");
      for (int i=0;i<a.size();++i) {
         replacement.SetRealParameter(first.toStdString(),a[i],i);
         replacement.SetRealParameter(second.toStdString(),b[i],i);
      }
   }
   // Initialize a separate clone: Az/El interpolation and file reading produce
   // derived vertices. Keep the authored pairs/file on the pending owner.
   QtResourcePreview validation(replacement.Clone());
   if (!validation->Initialize()) throw std::runtime_error("The FOV could not initialize these settings.");
   *fov=replacement;
   return geometryFields;
}

QString customFovResourceScript(GmatBase &object,const QString &coordinates)
{
   auto result=QString::fromStdString(object.GetGeneratingString(Gmat::SCRIPTING));
   if (!object.IsOfType("CustomFOV")) return result;
   const auto name=QString::fromStdString(object.GetName());
   // The engine serializes both angle aliases and file-loaded derived vectors.
   // Write exactly the selected input representation for interpreter/reopen.
   result.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(name)+
      "\\.(?:FOVFileName|ConeAngles|ClockAngles|Azimuth|Elevation)[ \\t]*=[^\\r\\n]*;[^\\r\\n]*(?:\\r?\\n|$)",QRegularExpression::MultilineOption));
   if (!result.endsWith('\n')) result+='\n';
   const auto file=QString::fromStdString(object.GetStringParameter("FOVFileName"));
   if (!file.isEmpty()) result+="GMAT "+name+".FOVFileName = '"+file+"';\n";
   else {
      if (coordinates!="ConeClock" && coordinates!="AzimuthElevation") throw std::runtime_error("Choose an FOV angle representation.");
      for (const auto &field:(coordinates=="ConeClock" ? QStringList{"ConeAngles","ClockAngles"} : QStringList{"Azimuth","Elevation"}))
         result+="GMAT "+name+"."+field+" = ["+vectorText(object,field)+"];\n";
   }
   return result;
}
