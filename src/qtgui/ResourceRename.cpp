#include "ResourcePreview.hpp"
#include "ResourceRename.hpp"
#include "ScriptStatements.hpp"
#include "ScriptCompatibility.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Parameter.hpp"
#include "CoordinateSystem.hpp"
#include "CalculatedPoint.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStringDecoder>
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
      if (object.IsOfType("CoordinateSystem") && owned->IsOfType("AxisSystem")) {
         prefix="Axes.";
         // CoordinateSystem forwards its owned axis properties in authored
         // source (Rot.XAxis/Primary), including GUI-created settings. Resolve
         // their actual typed owner; enum directions remain literal values.
         if (!path.startsWith(prefix)) {
            const auto found=field(*owned,path); if (found.owner) return found;
         }
      }
      if (path.startsWith(prefix)) { const auto found=field(*owned,path.mid(prefix.size())); if (found.owner) return found; }
   }
   return {};
}
bool renamedStringField(const Field &value,const Names &names)
{
   if (!value.owner) return false;
   // A body's CentralBody is a string reference, but CelestialBody has no
   // RenameRefObject override. Keep child-body parents in the same source
   // transaction instead of leaving a reference to the removed name.
   if (value.owner->IsOfType("CelestialBody") && value.owner->GetParameterText(value.id)=="CentralBody")
      for (auto it=names.cbegin();it!=names.cend();++it) {
         auto *original=Moderator::Instance()->GetConfiguredObject(it.key().toStdString());
         if (original && original->IsOfType("CelestialBody")) return true;
      }
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
// Inspect dependencies without interpreting them: interpretation can create
// objects, change path priorities and defer function references until a run.
// An identifier match is deliberately conservative, including quoted object
// references. Only ordinary comments are excluded by scriptStatements().
class RenameDependencies
{
public:
   explicit RenameDependencies(const Names &names) : manager(FileManager::Instance())
   {
      for (auto it=names.cbegin();it!=names.cend();++it) { affected.insert(it.key()); affected.insert(it.value()); }
      for (const auto &path:manager->GetAllGmatFunctionPaths()) functionPaths.append(QString::fromStdString(path));
      for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::MATH_NODE)) mathFunctions.insert(QString::fromStdString(name).toLower());
      for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::FUNCTION)) builtinFunctions.insert(QString::fromStdString(name));
      for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::COMMAND)) commands.insert(QString::fromStdString(name));
      for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::FUNCTION)) {
         auto *object=Moderator::Instance()->GetConfiguredObject(name);
         if (object && object->IsOfType("GmatFunction")) configuredFunctions.insert(QString::fromStdString(name),QString::fromStdString(object->GetStringParameter("FunctionPath")));
      }
   }
   void inspect(const QVector<ScriptStatement> &main)
   {
      inspectIncludes(main,functionPaths);
      for (auto it=configuredFunctions.cbegin();it!=configuredFunctions.cend();++it)
         inspectFile(functionFile(it.key(),it.value(),functionPaths),functionPaths);
   }
private:
   FileManager *manager;
   QSet<QString> affected,mathFunctions,builtinFunctions,commands,active,checked;
   QStringList functionPaths;
   QMap<QString,QString> configuredFunctions;
   const QRegularExpression identifier{"(?<![A-Za-z0-9_])[A-Za-z][A-Za-z0-9_]*"};
   const QRegularExpression include{"^\\s*#Include\\s+(.+?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption};
   [[noreturn]] void refuse(const QString &reason) const
   {
      throw std::runtime_error((reason+" External files were not changed; update affected references in the script editor.").toStdString());
   }
   QString literal(const QString &text,const QString &context) const
   {
      const auto match=QRegularExpression("^'([^'\\r\\n]+)'$").match(text.trimmed());
      if (!match.hasMatch()) refuse("Cannot inspect a nonliteral "+context+" safely.");
      return match.captured(1).replace('\\','/');
   }
   QString includeFile(const QString &value) const
   {
      const auto path=literal(value,"include filename");
      if (QFileInfo(path).isAbsolute()) return path;
      // ParseIncludeBlock uses global include priority even for nested files.
      // GetGmatIncludePath returns the matched directory, including subpaths.
      const auto directory=QString::fromStdString(manager->GetGmatIncludePath(path.toStdString()));
      if (directory.isEmpty()) refuse("Cannot resolve external include "+path+".");
      return QDir(directory).filePath(QFileInfo(path).fileName());
   }
   QString functionFile(const QString &name,const QString &requested,const QStringList &paths) const
   {
      if (!requested.isEmpty() && QFileInfo(requested).isFile()) return requested;
      // CheckFunctionDefinition falls back to the configured resource name,
      // preserving a valid explicit alias/basename whenever its file exists.
      for (const auto &path:paths) {
         const auto candidate=QDir(path).filePath(name+".gmf");
         if (QFileInfo(candidate).isFile()) return candidate;
      }
      refuse("Cannot resolve external GMAT function "+name+".");
   }
   QString explicitFunctionFile(const QString &name,const QString &value) const
   {
      auto path=literal(value,"GMAT function filename");
      // Mirror SetGmatFunctionPath's composition without invoking its setter,
      // which adds to FileManager's function search paths.
      if (path.startsWith('.')) {
         auto candidate=QString::fromStdString(manager->GetGmatWorkingDirectory())+path;
         if (!manager->DoesDirectoryExist(candidate.toStdString())) candidate=QString::fromStdString(manager->GetBinDirectory())+path;
         path=candidate;
      }
      if (!path.contains(".gmf")) path=QDir(path).filePath(name+".gmf");
      return QFileInfo(path).absoluteFilePath();
   }
   void inspectIncludes(const QVector<ScriptStatement> &statements,const QStringList &paths)
   {
      for (const auto &statement:statements) {
         const auto match=include.match(statement.code);
         if (match.hasMatch()) inspectFile(includeFile(match.captured(1)),paths);
         else if (statement.code.trimmed().startsWith("#Include")) refuse("Cannot inspect an unresolved include directive.");
      }
   }
   void inspectFile(const QString &path,QStringList paths)
   {
      const QFileInfo info(path); const auto canonical=info.canonicalFilePath();
      if (canonical.isEmpty() || !info.isFile()) refuse("Cannot read external dependency "+path+".");
      const auto key=canonical+'\n'+paths.join('\n');
      if (active.contains(canonical)) refuse("Cannot inspect circular external dependencies at "+canonical+".");
      if (checked.contains(key)) return;
      QFile file(canonical);
      if (!file.open(QIODevice::ReadOnly)) refuse("Cannot read external dependency "+canonical+".");
      const auto bytes=file.readAll();
      QStringDecoder decoder(QStringDecoder::Utf8); QString source=decoder(bytes);
      if (file.error()!=QFileDevice::NoError || decoder.hasError() || source.contains(QChar('\0')))
         refuse("Cannot decode external dependency "+canonical+" safely.");
      if (source.startsWith(QChar(0xfeff))) source.remove(0,1);
      const auto statements=scriptStatements(source);
      QMap<QString,QString> localTypes,localFunctions;
      const QRegularExpression create("^\\s*Create\\s+([A-Za-z][A-Za-z0-9_]*)\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
      for (const auto &statement:statements) {
         auto references=identifier.globalMatch(statement.code);
         while (references.hasNext()) {
            const auto name=references.next().captured();
            if (affected.contains(name)) refuse("External dependency "+canonical+" may reference or define "+name+".");
         }
         const auto declaration=create.match(statement.code);
         if (declaration.hasMatch()) {
            auto members=QRegularExpression("[A-Za-z][A-Za-z0-9_]*(?:\\[[^]]*\\])?").globalMatch(declaration.captured(2));
            while (members.hasNext()) {
               const auto name=members.next().captured().section('[',0,0);
               localTypes.insert(name,declaration.captured(1));
               if (declaration.captured(1)=="GmatFunction") localFunctions.insert(name,{});
            }
         }
      }
      for (const auto &statement:statements) {
         const auto assignment=QRegularExpression("^\\s*(?:GMAT\\s+)?([A-Za-z][A-Za-z0-9_]*)\\.FunctionPath\\s*=\\s*(.*?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption).match(statement.code);
         if (!assignment.hasMatch()) continue;
         const auto name=assignment.captured(1);
         if (!localFunctions.contains(name)) refuse("Cannot inspect an external function path assignment in "+canonical+" safely.");
         const auto resolved=explicitFunctionFile(name,assignment.captured(2)); localFunctions[name]=resolved;
         const auto directory=QFileInfo(resolved).absolutePath(); paths.removeAll(directory); paths.prepend(directory);
      }
      active.insert(canonical);
      inspectIncludes(statements,paths);
      for (auto it=localFunctions.cbegin();it!=localFunctions.cend();++it) inspectFile(functionFile(it.key(),it.value(),paths),paths);
      const QRegularExpression call("(?<![A-Za-z0-9_.])([A-Za-z][A-Za-z0-9_]*(?:\\.[A-Za-z][A-Za-z0-9_]*)*)\\s*\\(");
      for (const auto &statement:statements) {
         const auto code=statement.code.trimmed();
         if (include.match(code).hasMatch() || create.match(code).hasMatch() || code.startsWith("function ")) continue;
         auto unquoted=code; bool quoted=false;
         for (int i=0;i<unquoted.size();++i) {
            if (unquoted[i]=='\'') { if (quoted && i+1<unquoted.size() && unquoted[i+1]=='\'') { unquoted[i++]=' '; unquoted[i]=' '; continue; } quoted=!quoted; unquoted[i]=' '; }
            else if (quoted) unquoted[i]=' ';
         }
         QStringList callees; auto calls=call.globalMatch(unquoted);
         while (calls.hasNext()) callees.append(calls.next().captured(1));
         const auto bare=QRegularExpression("^([A-Za-z][A-Za-z0-9_]*)\\s*;?$").match(unquoted);
         if (bare.hasMatch() && !commands.contains(bare.captured(1))) callees.append(bare.captured(1));
         for (const auto &name:callees) {
            if (mathFunctions.contains(name.toLower()) || builtinFunctions.contains(name)) continue;
            if (localFunctions.contains(name)) continue; // Already scanned, including unused declarations.
            if (configuredFunctions.contains(name)) { inspectFile(functionFile(name,configuredFunctions.value(name),paths),paths); continue; }
            const auto owner=name.section('.',0,0); auto *configured=Moderator::Instance()->GetConfiguredObject(owner.toStdString());
            // Indexed arrays and command resource arguments are not functions.
            if (localTypes.value(owner)=="Array" || (configured && configured->IsOfType("Array"))) continue;
            if (configured && (configured->IsOfType("PropSetup") || configured->IsOfType(Gmat::SOLVER) || configured->IsOfType(Gmat::BURN))) continue;
            if (name.contains('.')) refuse("Cannot resolve external call "+name+" in "+canonical+" safely.");
            inspectFile(functionFile(name,{},paths),paths);
         }
      }
      active.remove(canonical); checked.insert(key);
   }
};

QJsonObject camera(QJsonObject value,const Names &names)
{
   for (const auto *key:{"plot","reference","target","automaticTrajectory","automaticBody","segmentFrame"})
      if (value.contains(key)) value[key]=reference(value.value(key).toString(),names);
   for (const auto *key:{"objectLabels","objectTrajectories","objectCenters","objectEndpoints","objectAxes","objectGrids","objectXYPlanes","objectVelocities","objectSegmentLabels","objectMarkerSizes","objectLineWidths","objectFontSizes","objectFontPositions","objectModelFiles"}) {
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
   if (value.value("playback").isObject()) {
      auto playback=value.value("playback").toObject();
      if (playback.contains("syncParent")) playback["syncParent"]=reference(playback.value("syncParent").toString(),names);
      value["playback"]=playback;
   }
   if (value.contains("sensorMasks")) {
      QJsonArray masks; for (const auto &entry:value.value("sensorMasks").toArray()) {
         auto mask=entry.toObject();
         for (const auto *key:{"source","hardware"}) if (mask.contains(key)) mask[key]=reference(mask.value(key).toString(),names);
         masks.append(mask);
      } value["sensorMasks"]=masks;
   }
   return value;
}
}
bool resourceCanBeRenamed(GmatBase *object)
{
   if (!object || object->IsOfType("SolarSystem")) return false;
   if (auto *body=dynamic_cast<CelestialBody *>(object);body && !body->IsUserDefined()) return false;
   if (auto *frame=dynamic_cast<CoordinateSystem *>(object);frame && frame->IsBuiltIn()) return false;
   if (auto *point=dynamic_cast<CalculatedPoint *>(object);point && point->IsBuiltIn()) return false;
   return true;
}
QString renameResourceSource(const QString &source,GmatBase &object,const QString &name)
{
   auto *moderator=Moderator::Instance(); const auto old=QString::fromStdString(object.GetName()); Names names{{old,name}};
   const auto statements=scriptStatements(source);
   if (object.IsOfType("PropSetup")) {
      const auto oldFM=old+"_ForceModel",newFM=name+"_ForceModel";
      if (object.GetStringParameter("FM")==oldFM.toStdString() && moderator->GetConfiguredObject(oldFM.toStdString()) && !moderator->GetConfiguredObject(newFM.toStdString())) names.insert(oldFM,newFM);
   }
   // The declaration must be owned by this source, even if the interpreter
   // exposes included resources in the same configuration tree.
   int localDeclarations=0;
   const QRegularExpression localCreate("^\\s*Create\\s+[A-Za-z][A-Za-z0-9_]*\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   for (const auto &statement:statements) {
      const auto declaration=localCreate.match(statement.code);
      if (!declaration.hasMatch()) continue;
      auto members=token.globalMatch(declaration.captured(1));
      while (members.hasNext()) if (members.next().captured()==old) ++localDeclarations;
   }
   if (localDeclarations!=1) throw std::runtime_error("Cannot locate one local declaration for this resource; rename it in its defining script.");
   RenameDependencies(names).inspect(statements);
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

void validateResourceExternalReferences(const QString &source,const QString &name)
{
   RenameDependencies(Names{{name,name}}).inspect(scriptStatements(source));
}
