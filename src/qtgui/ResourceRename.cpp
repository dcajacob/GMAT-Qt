#include "ResourcePreview.hpp"
#include "ResourceRename.hpp"
#include "ScriptStatements.hpp"
#include "ScriptCompatibility.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Parameter.hpp"
#include "CoordinateSystem.hpp"
#include "CalculatedPoint.hpp"
#include "BaseException.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace {
struct Edit { qsizetype position,length; QString text; };
using Names=QMap<QString,QString>;
const QRegularExpression token("(?<![A-Za-z0-9_.])[A-Za-z][A-Za-z0-9_]*(?:\\.[A-Za-z][A-Za-z0-9_]*)*");
QString reference(const QString &text,const Names &names)
{
   if (names.contains(text)) return names.value(text);
   auto parts=text.split('.');
   if (names.contains(parts.first())) parts[0]=names.value(parts.first());
   // A middle name is a dependency only in an actual system-parameter path.
   // The terminal property is never an object name (a resource can be named X).
   auto *parameter=dynamic_cast<Parameter *>(Moderator::Instance()->GetConfiguredObject(text.toStdString()));
   if (parameter && parameter->GetKey()==GmatParam::SYSTEM_PARAM)
      for (int i=1;i+1<parts.size();++i) if (names.contains(parts[i])) parts[i]=names.value(parts[i]);
   if (parts.size()==2 && names.contains(parts.last())) {
      auto *hardware=Moderator::Instance()->GetConfiguredObject(parts.last().toStdString());
      auto *participant=Moderator::Instance()->GetConfiguredObject(parts.first().toStdString());
      if (hardware && hardware->IsOfType(Gmat::HARDWARE) && participant && participant->IsOfType(Gmat::SPACE_POINT)) {
         try { const auto attached=participant->GetStringArrayParameter("AddHardware"); if (std::find(attached.begin(),attached.end(),parts.last().toStdString())!=attached.end()) parts[1]=names.value(parts.last()); } catch (BaseException &) { }
      }
   }
   return parts.join('.');
}
bool literalType(Gmat::ParameterType type)
{
   return type==Gmat::STRING_TYPE || type==Gmat::ENUMERATION_TYPE || type==Gmat::FILENAME_TYPE ||
      type==Gmat::BOOLEAN_TYPE || type==Gmat::ON_OFF_TYPE || type==Gmat::STRINGARRAY_TYPE;
}
struct Field { GmatBase *owner=nullptr; int id=-1; };
Field field(GmatBase &object,const QString &path)
{
   try { const auto id=object.GetParameterID(path.toStdString()); if (id>=0) return {&object,id}; } catch (BaseException &) { }
   for (int i=0;i<object.GetOwnedObjectCount();++i) {
      auto *owned=object.GetOwnedObject(i); if (!owned) continue;
      QString prefix=QString::fromStdString(object.BuildPropertyName(owned))+'.';
      if (object.IsOfType("CoordinateSystem") && owned->IsOfType("AxisSystem")) prefix="Axes.";
      if (path.startsWith(prefix)) { const auto found=field(*owned,path.mid(prefix.size())); if (found.owner) return found; }
   }
   return {};
}
bool renamedStringField(const Field &value,const Names &names)
{
   if (!value.owner) return false;
   try {
      QtResourcePreview copy(value.owner->Clone());
      for (auto it=names.cbegin();it!=names.cend();++it) {
         auto *original=Moderator::Instance()->GetConfiguredObject(it.key().toStdString());
         if (original) copy->RenameRefObject(original->GetType(),it.key().toStdString(),it.value().toStdString());
      }
      const auto type=value.owner->GetParameterType(value.id);
      if (type==Gmat::STRINGARRAY_TYPE) {
         const auto before=value.owner->GetStringArrayParameter(value.id);
         const auto after=copy->GetStringArrayParameter(value.id);
         return before!=after; // Some plugins reuse one static getter buffer.
      }
      const auto before=value.owner->GetStringParameter(value.id),after=copy->GetStringParameter(value.id);
      return before!=after;
   } catch (BaseException &) { return false; }
}
void tokens(const ScriptStatement &statement,qsizetype begin,qsizetype end,bool quotedReferences,const Names &names,QVector<Edit> &edits,const QVector<QPair<qsizetype,qsizetype>> &literals={})
{
   bool quoted=false; auto matches=token.globalMatch(statement.code.mid(begin,end-begin)); qsizetype scanned=begin;
   while (matches.hasNext()) {
      const auto match=matches.next(); const auto position=begin+match.capturedStart();
      for (;scanned<position;++scanned) if (statement.code[scanned]=='\'') {
         if (quoted && scanned+1<position && statement.code[scanned+1]=='\'') ++scanned;
         else quoted=!quoted;
      }
      if ((quoted && !quotedReferences) || std::any_of(literals.cbegin(),literals.cend(),[&](const auto &range) { return position>=range.first && position<range.second; })) continue;
      const auto old=match.captured(),changed=reference(old,names); if (changed==old) continue;
      const auto positions=statement.positions.mid(position,old.size());
      if (positions.size()!=old.size()) throw std::runtime_error("Cannot locate a resource reference safely.");
      // Identifier characters are contiguous even when a statement has line
      // continuations/comments. Retain every byte outside the selected token.
      for (int i=1;i<positions.size();++i) if (positions[i]!=positions[i-1]+1)
         throw std::runtime_error("A split resource identifier cannot be renamed safely.");
      edits.append({positions.first(),old.size(),changed});
   }
}
QJsonObject camera(QJsonObject value,const Names &names)
{
   for (const auto *key:{"plot","reference","target","automaticTrajectory","automaticBody","segmentFrame"})
      if (value.contains(key)) value[key]=reference(value.value(key).toString(),names);
   for (const auto *key:{"objectLabels","objectTrajectories","objectCenters","objectEndpoints","objectAxes","objectGrids","objectXYPlanes","objectVelocities","objectMarkerSizes","objectLineWidths","objectFontSizes","objectFontPositions"}) {
      if (!value.contains(key)) continue;
      auto map=value.value(key).toObject();
      for (auto it=names.cbegin();it!=names.cend();++it) if (map.contains(it.key())) { const auto setting=map.take(it.key()); map.insert(it.value(),setting); }
      value[key]=map;
   }
   if (value.contains("reset")) value["reset"]=camera(value.value("reset").toObject(),names);
   if (value.contains("views")) {
      QJsonArray views; for (const auto &entry:value.value("views").toArray()) views.append(camera(entry.toObject(),names)); value["views"]=views;
   }
   if (value.contains("vectors")) {
      QJsonArray vectors; for (const auto &entry:value.value("vectors").toArray()) {
         auto vector=entry.toObject();
         for (const auto *key:{"source","destination"}) if (vector.contains(key)) vector[key]=reference(vector.value(key).toString(),names);
         vectors.append(vector);
      } value["vectors"]=vectors;
   }
   return value;
}
}
bool resourceCanBeRenamed(GmatBase *object)
{
   if (!object || object->IsOfType("SolarSystem") || object->IsOfType("CelestialBody")) return false;
   if (auto *frame=dynamic_cast<CoordinateSystem *>(object);frame && frame->IsBuiltIn()) return false;
   if (auto *point=dynamic_cast<CalculatedPoint *>(object);point && point->IsBuiltIn()) return false;
   return true;
}
QString renameResourceSource(const QString &source,GmatBase &object,const QString &name)
{
   auto *moderator=Moderator::Instance(); const auto old=QString::fromStdString(object.GetName()); Names names{{old,name}};
   const auto statements=scriptStatements(source);
   // Includes and GMF helpers can hold references that stream interpretation
   // does not validate until execution. Do not edit or silently strand them.
   for (const auto &statement:statements) if (QRegularExpression("^\\s*#Include\\b").match(statement.code).hasMatch())
      throw std::runtime_error("Rename in a script with includes requires updating its external defining/reference files in the script editor.");
   for (const auto &function:moderator->GetListOfObjects(Gmat::FUNCTION)) {
      auto *configured=moderator->GetConfiguredObject(function);
      if (configured && configured->IsOfType("GmatFunction"))
         throw std::runtime_error("Rename with GMAT function helpers requires updating their external references in the script editor.");
   }
   if (object.IsOfType("PropSetup")) {
      const auto oldFM=old+"_ForceModel",newFM=name+"_ForceModel";
      if (object.GetStringParameter("FM")==oldFM.toStdString() && moderator->GetConfiguredObject(oldFM.toStdString()) && !moderator->GetConfiguredObject(newFM.toStdString())) names.insert(oldFM,newFM);
   }
   QVector<Edit> edits; int declarations=0;
   const QRegularExpression create("^\\s*Create\\s+[A-Za-z][A-Za-z0-9_]*\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?([A-Za-z][A-Za-z0-9_]*(?:\\.[A-Za-z][A-Za-z0-9_]*)*)(?:\\([^;=]*\\))?\\s*=\\s*(.*?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   bool mission=false;
   for (const auto &statement:statements) {
      const auto declared=create.match(statement.code);
      if (declared.hasMatch()) {
         auto members=token.globalMatch(declared.captured(1));
         while (members.hasNext()) { const auto member=members.next(); if (member.captured()==old) ++declarations; }
         tokens(statement,declared.capturedStart(1),declared.capturedEnd(1),false,names,edits); continue;
      }
      if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch()) mission=true;
      const auto set=assignment.match(statement.code);
      if (!set.hasMatch()) {
         const auto command=statement.code.trimmed().section(QRegularExpression("\\s+"),0,0).remove(';');
         const auto types=moderator->GetListOfFactoryItems(Gmat::COMMAND);
         if (std::find(types.begin(),types.end(),command.toStdString())!=types.end()) mission=true;
         if (mission) {
            QVector<QPair<qsizetype,qsizetype>> literals;
            if (command=="Toggle") {
               const auto mode=QRegularExpression("\\b(?:On|Off)\\s*;?\\s*$").match(statement.code);
               if (mode.hasMatch()) literals.append({mode.capturedStart(),mode.capturedEnd()});
            }
            // These command option names and enum/boolean values are syntax,
            // even when a configured resource happens to share their spelling.
            const QRegularExpression option("(?:\\{|,)\\s*(?:SolveMode|ExitMode|ShowProgressWindow|Append)\\s*=\\s*[A-Za-z][A-Za-z0-9_]*");
            auto options=option.globalMatch(statement.code); while (options.hasNext()) { const auto value=options.next(); literals.append({value.capturedStart(),value.capturedEnd()}); }
            const QRegularExpression optionKey("(?:\\{|,)\\s*(SolveMode|ExitMode|ShowProgressWindow|Append|Perturbation|Lower|Upper|MaxStep|AdditiveScaleFactor|MultiplicativeScaleFactor|Tolerance|StopTolerance)(?=\\s*=)");
            auto keys=optionKey.globalMatch(statement.code); while (keys.hasNext()) { const auto key=keys.next(); literals.append({key.capturedStart(1),key.capturedEnd(1)}); }
            tokens(statement,0,statement.code.size(),false,names,edits,literals);
         } continue;
      }
      tokens(statement,set.capturedStart(1),set.capturedStart(2),false,names,edits);
      const auto lhs=set.captured(1); auto *owner=moderator->GetConfiguredObject(lhs.section('.',0,0).toStdString());
      Field value;
      if (owner && lhs.contains('.')) value=field(*owner,lhs.section('.',1));
      else if (owner && owner->IsOfType(Gmat::PARAMETER)) value=field(*owner,owner->GetTypeName()=="String" ? "Expression" : (owner->GetTypeName()=="Array" ? "SingleValue" : "Value"));
      if (!value.owner) {
         auto *parameter=moderator->GetConfiguredObject(lhs.toStdString());
         if (parameter && parameter->IsOfType(Gmat::PARAMETER)) value=field(*parameter,"Value");
      }
      if (owner && owner->IsOfType("TrackingFileSet") && lhs.section('.',1)=="AddTrackingConfig") {
         // A tracking row mixes true participant/hardware names with literal
         // measurement types. Restrict edits to its bounded signal path.
         const auto path=QRegularExpression("^\\s*\\{\\s*\\{([^{}]*)\\}\\s*,[^{}]+\\}\\s*$").match(set.captured(2));
         if (!path.hasMatch()) throw std::runtime_error("This tracking configuration requires its external/unsupported signal path to be renamed in the script editor.");
         tokens(statement,set.capturedStart(2)+path.capturedStart(1),set.capturedStart(2)+path.capturedEnd(1),true,names,edits);
         continue;
      }
      const auto rhs=set.captured(2); bool quotedReferences=false,change=!value.owner;
      if (value.owner) {
         const auto type=value.owner->GetParameterType(value.id);
         quotedReferences=type==Gmat::OBJECT_TYPE || type==Gmat::OBJECTARRAY_TYPE;
         const bool stringLiteral=owner && owner->GetTypeName()=="String" && rhs.trimmed().startsWith('\'');
         const bool semanticReference=!stringLiteral && literalType(type) && renamedStringField(value,names);
         // Owned factory selectors use OBJECT_TYPE too, but their values are
         // class names rather than references to configured resources.
         const bool factorySelector=type==Gmat::OBJECT_TYPE && QStringList{"Axes","Type","Attitude","AtmosphereModel"}.contains(QString::fromStdString(value.owner->GetParameterText(value.id)));
         change=!factorySelector && (!literalType(type) || semanticReference);
         quotedReferences=quotedReferences || semanticReference;
         // String variables can refer to one another without quoting. Their
         // literal contents, including apostrophes and resource-like words, stay.
         if (!change && !rhs.trimmed().startsWith('\'') && ((owner && owner->GetTypeName()=="String") || (mission && (type==Gmat::STRING_TYPE || type==Gmat::FILENAME_TYPE)))) {
            auto *parameter=moderator->GetConfiguredObject(rhs.trimmed().toStdString()); change=parameter && parameter->GetTypeName()=="String";
         }
      }
      if (change) tokens(statement,set.capturedStart(2),set.capturedEnd(2),quotedReferences,names,edits);
      if (!value.owner && !mission) throw std::runtime_error(("Cannot identify the typed setting "+lhs+" safely; rename it in the script editor.").toStdString());
   }
   if (declarations!=1) throw std::runtime_error("Cannot locate one local declaration for this resource; rename it in its defining script.");
   // Validate and update only the managed Array owner comments. Expressions are
   // already patched as normal mission assignments, preserving initialization.
   for (auto it=names.cbegin();it!=names.cend();++it) {
      auto *configured=moderator->GetConfiguredObject(it.key().toStdString()); if (!configured || configured->GetTypeName()!="Array") continue;
      arrayExpressions(source,it.key());
      const QRegularExpression marker("^([ \\t]*% GMAT-Qt-Array-Expressions )("+QRegularExpression::escape(it.key())+")(?= (?:begin|end)[ \\t]*(?:\\r?$))",QRegularExpression::MultilineOption);
      auto matches=marker.globalMatch(source); while (matches.hasNext()) { const auto match=matches.next(); edits.append({match.capturedStart(2),match.capturedLength(2),it.value()}); }
   }
   qtCameraSettings(source); // Reject malformed/ambiguous managed metadata.
   const QRegularExpression metadata("^([ \\t]*% GMAT-Qt-Camera )([^\\r\\n]+)",QRegularExpression::MultilineOption);
   auto comments=metadata.globalMatch(source); while (comments.hasNext()) {
      const auto match=comments.next(); const auto before=QJsonDocument::fromJson(match.captured(2).toUtf8()).object(),after=camera(before,names);
      if (before!=after) edits.append({match.capturedStart(2),match.capturedLength(2),QString::fromUtf8(QJsonDocument(after).toJson(QJsonDocument::Compact))});
   }
   std::sort(edits.begin(),edits.end(),[](const auto &a,const auto &b) { return a.position>b.position; });
   auto candidate=source; qsizetype end=source.size();
   for (const auto &edit:edits) { if (edit.position+edit.length>end) throw std::runtime_error("Overlapping resource rename edits."); candidate.replace(edit.position,edit.length,edit.text); end=edit.position; }
   qtCameraSettings(candidate); return candidate;
}
