#include "UserParameter.hpp"
#include "ScriptStatements.hpp"
#include <QRegularExpression>
#include <QVector>
#include <QMap>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace {
using Statement=ScriptStatement;
QVector<Statement> statements(const QString &source)
{
   QVector<Statement> result; Statement current;
   bool quoted=false; int depth=0;
   for (qsizetype i=0;i<source.size();++i) {
      const auto ch=source[i];
      if (!quoted && ch=='%') { while (i<source.size() && source[i]!='\n') ++i; if (i==source.size()) break; }
      const auto c=source[i]; current.code+=c; current.positions.append(i);
      if (c=='\'' && !quoted && QRegularExpression("^\\s*(?:GMAT\\s+)?[A-Za-z][A-Za-z0-9_.]*(?:\\([^;]*\\))?\\s*=\\s*$").match(current.code.chopped(1)).hasMatch()) {
         // GMAT string literals preserve inner apostrophes rather than using
         // doubled-quote escaping. Treat the literal as one block through its
         // closing quote before a terminator or trailing comment.
         const auto newline=source.indexOf('\n',i); const auto end=newline<0 ? source.size() : newline;
         qsizetype closing=-1;
         for (auto q=i+1;q<end;++q) if (source[q]=='\'' && QRegularExpression("^[ \\t\\r]*(?:;[ \\t]*)?(?:%[^\\n]*)?$").match(source.mid(q+1,end-q-1)).hasMatch()) { closing=q; break; }
         if (closing>=0) {
            while (i<closing) { current.code+=source[++i]; current.positions.append(i); }
            continue;
         }
      }
      if (c=='\'') {
         if (quoted && i+1<source.size() && source[i+1]=='\'') { current.code+=source[++i]; current.positions.append(i); }
         else quoted=!quoted;
      } else if (!quoted) {
         if (c=='[' || c=='{' || c=='(') ++depth;
         if (c==']' || c=='}' || c==')') --depth;
         if (c==';' && depth==0) { result.append(current); current={}; }
         else if (c=='\n' && depth==0 && !current.code.trimmed().isEmpty()) {
            const auto trimmed=current.code.trimmed();
            if (trimmed.endsWith("...")) {
               const auto dots=current.code.lastIndexOf("..."); current.continuations+=current.positions.mid(dots,3); current.code.remove(dots,3); current.positions.remove(dots,3);
            } else if (!trimmed.endsWith('=') && !QRegularExpression("^Create\\s+[A-Za-z][A-Za-z0-9_]*$").match(trimmed).hasMatch()) {
               result.append(current); current={};
            }
         }
      }
   }
   if (!current.code.trimmed().isEmpty() && !quoted && depth==0) result.append(current);
   if (quoted || depth!=0)
      throw std::runtime_error("The script contains an incomplete statement. Build it before editing a parameter.");
   return result;
}

QString forceSelectorSource(const QString &source,const QString &name,const QString &field,const QString &value,const QStringList &aliases,const QString &firstMissionStatement,const QStringList &dependencies={},const QStringList &creatorAliases={})
{
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"\\.([A-Za-z0-9_.]+)\\s*=\\s*(.*?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   auto normalize=[](QString code) { code=code.trimmed(); code.remove(QRegularExpression("^GMAT\\s+")); if (!code.endsWith(';')) code+=';'; return code; };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) { const auto commands=statements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalize(commands.first().code); }
   struct Edit { qsizetype position,length; QString text; };
   QVector<Edit> edits; QStringList remove;
   qsizetype firstDependent=source.size(); bool located=firstCommand.isEmpty(),found=false;
   for (const auto &statement:statements(source)) {
      int first=0; while (first<statement.code.size() && statement.code[first].isSpace()) ++first;
      if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) {
         firstDependent=std::min(firstDependent,statement.positions[first]); located=true; break;
      }
      const auto match=assignment.match(statement.code); if (!match.hasMatch()) continue;
      const auto key=match.captured(1);
      const bool dependent=std::any_of(dependencies.cbegin(),dependencies.cend(),[&](const QString &prefix) { return key.startsWith(prefix); });
      const bool creator=key==field || creatorAliases.contains(key);
      if (!creator && !dependent && !key.startsWith(field+'.') && !aliases.contains(key)) continue;
      firstDependent=std::min(firstDependent,statement.positions[first]); remove.append(key);
      if (!creator || value=="Off") continue;
      found=true; const auto positions=statement.positions.mid(match.capturedStart(2),match.capturedLength(2));
      if (positions.isEmpty()) throw std::runtime_error("Cannot locate the force selector value safely.");
      for (auto position:positions) edits.append({position,1,{}});
      for (auto position:statement.continuations) if (position>=positions.first() && position<=positions.last()) edits.append({position,1,{}});
      edits.append({positions.first(),0,value});
   }
   if (!located) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before changing this force selector.");
   const auto block="GMAT "+name+"."+field+" = "+value+";\n";
   if (value=="Off") return setConfigurationBlock(source,name,remove,block,firstMissionStatement);
   if (!found) edits.append({firstDependent,0,(firstDependent>0 && source[firstDependent-1]!='\n' ? "\n" : "")+block});
   std::stable_sort(edits.begin(),edits.end(),[](const Edit &a,const Edit &b) { return a.position!=b.position ? a.position>b.position : a.length>b.length; });
   QString candidate=source; for (const auto &edit:edits) candidate.replace(edit.position,edit.length,edit.text);
   return candidate;
}
}
QVector<ScriptStatement> scriptStatements(const QString &source) { return statements(source); }
QString userParameterLiteral(const QString &type,const QString &value)
{
   if (type=="Variable") {
      bool ok=false; const auto number=value.trimmed().toDouble(&ok);
      if (!ok || !std::isfinite(number)) throw std::runtime_error("Enter a finite numeric initial value. Use mission assignments for expressions.");
      return QString::number(number,'g',17);
   }
   if (type!="String") throw std::runtime_error("Initial values require a Variable or String.");
   for (const auto ch:value) if (ch.unicode()<32 || ch==QChar(0x2028) || ch==QChar(0x2029))
      throw std::runtime_error("GMAT string initializers use one single-quoted line. Remove control characters.");
   return "'"+value+"'";
}
QString setUserParameterValue(const QString &source,const QString &name,const QString &type,const QString &value,const QString &firstMissionStatement)
{
   const auto literal=userParameterLiteral(type,value);
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?("+QRegularExpression::escape(name)+")\\s*=\\s*(.*?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression declaration("^\\s*Create\\s+"+QRegularExpression::escape(type)+"\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression begin("^\\s*BeginMissionSequence\\b");
   const auto parsed=statements(source);
   qsizetype rhsStart=-1,rhsEnd=-1,createEnd=-1;
   auto normalized=[](QString code) {
      code=code.trimmed(); if (code.endsWith(';')) code.chop(1); code.remove(QRegularExpression("^GMAT\\s+"));
      QString key; bool quoted=false;
      for (auto ch:code) { if (ch=='\'') quoted=!quoted; if (quoted || !ch.isSpace()) key+=ch; }
      return key;
   };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) {
      const auto commands=statements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalized(commands.first().code);
   }
   bool locatedBoundary=firstCommand.isEmpty();
   for (const auto &statement:parsed) {
      if (begin.match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalized(statement.code)==firstCommand)) { locatedBoundary=true; break; }
      const auto create=declaration.match(statement.code);
      if (create.hasMatch() && create.captured(1).split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).contains(name)) createEnd=statement.positions.last()+1;
      const auto match=assignment.match(statement.code);
      if (!match.hasMatch()) continue;
      rhsStart=statement.positions[match.capturedStart(2)]; rhsEnd=statement.positions[match.capturedEnd(2)-1]+1;
   }
   if (!locatedBoundary) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before editing the initializer.");
   if (createEnd<0) throw std::runtime_error("Cannot locate this parameter's declaration safely. Use its script settings.");
   QString candidate=source;
   if (rhsStart>=0) {
      struct Edit { qsizetype start,length; QString text; };
      QVector<Edit> edits{{rhsStart,rhsEnd-rhsStart,literal}};
      // TextParser stops checking for comments after a percent sign inside a
      // literal. Preserve a trailing comment on its own line in this case.
      if (type=="String" && value.contains('%')) {
         const auto trailing=QRegularExpression("^[ \\t]*;?[ \\t]*(%[^\\r\\n]*)").match(source.mid(rhsEnd));
         if (trailing.hasMatch()) {
            const auto comment=trailing.captured(1);
            edits.append({rhsEnd+trailing.capturedStart(1),comment.size(),{}});
            edits.append({source.lastIndexOf('\n',rhsStart-1)+1,0,comment+"\n"});
         }
      }
      std::stable_sort(edits.begin(),edits.end(),[](const Edit &a,const Edit &b) { return a.start>b.start; });
      for (const auto &edit:edits) candidate.replace(edit.start,edit.length,edit.text);
   }
   else {
      // Keep an inline declaration comment attached to its declaration.
      auto after=createEnd; while (after<source.size() && (source[after]==' ' || source[after]=='\t')) ++after;
      if (after<source.size() && source[after]=='%') { const auto end=source.indexOf('\n',after); createEnd=end<0 ? source.size() : end+1; }
      candidate.insert(createEnd,"\nGMAT "+name+" = "+literal+";\n");
   }
   return candidate;
}

QString setConfigurationBlock(const QString &source,const QString &name,const QStringList &properties,const QString &block,const QString &firstMissionStatement)
{
   auto normalize=[](QString code) {
      code=code.trimmed(); if (code.endsWith(';')) code.chop(1);
      code.remove(QRegularExpression("^GMAT\\s+"));
      QString result; bool quoted=false;
      for (auto ch:code) { if (ch=='\'') quoted=!quoted; if (quoted || !ch.isSpace()) result+=ch; }
      return result;
   };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) {
      const auto commands=statements(firstMissionStatement);
      if (!commands.isEmpty()) firstCommand=normalize(commands.first().code);
   }
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"(?:\\.([A-Za-z0-9_.]+(?:\\([^;=]*\\))?)|(\\([^;=]*\\)))\\s*=");
   const QRegularExpression begin("^\\s*BeginMissionSequence\\b");
   QVector<qsizetype> remove;
   auto boundary=source.size(); bool located=firstCommand.isEmpty();
   for (const auto &statement:statements(source)) {
      if (begin.match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) {
         auto first=0; while (first<statement.code.size() && statement.code[first].isSpace()) ++first;
         boundary=source.lastIndexOf('\n',statement.positions[first]-1)+1;
         // A command may share a line with a configuration assignment.
         if (!source.mid(boundary,statement.positions[first]-boundary).trimmed().isEmpty()) boundary=statement.positions[first];
         located=true; break;
      }
      const auto match=assignment.match(statement.code);
      const auto property=(match.captured(1).isEmpty() ? match.captured(2) : match.captured(1)).remove(QRegularExpression("\\s+"));
      if (!match.hasMatch() || !properties.contains(property)) continue;
      qsizetype first=0,last=statement.code.size()-1;
      while (first<=last && statement.code[first].isSpace()) ++first;
      while (last>=first && statement.code[last].isSpace()) --last;
      const auto start=statement.positions[first],end=statement.positions[last];
      const auto lineStart=start ? source.lastIndexOf('\n',start-1)+1 : 0;
      auto lineEnd=source.indexOf('\n',end); if (lineEnd<0) lineEnd=source.size();
      bool commentsInside=false;
      for (auto i=first+1;i<=last;++i) if (statement.positions[i]!=statement.positions[i-1]+1) { commentsInside=true; break; }
      if (!commentsInside && source.mid(lineStart,start-lineStart).trimmed().isEmpty() && source.mid(end+1,lineEnd-end-1).trimmed().isEmpty()) {
         // Plain assignment lines can be removed entirely. Repeated Apply
         // must not accumulate empty rows or whitespace from old filenames.
         for (auto i=lineStart;i<lineEnd+(lineEnd<source.size() ? 1 : 0);++i) remove.append(i);
      } else {
         // Mapped code positions exclude comments. Keep them verbatim when a
         // statement crosses lines or has a trailing comment.
         for (auto i=first;i<=last;++i) remove.append(statement.positions[i]);
         remove+=statement.continuations;
      }
   }
   if (!located) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before editing this resource.");
   QString candidate=source;
   candidate.insert(boundary,(boundary>0 && source[boundary-1]!='\n' ? "\n" : "")+block+(block.endsWith('\n') ? "" : "\n"));
   std::sort(remove.begin(),remove.end());
   for (auto i=remove.crbegin();i!=remove.crend();++i) candidate.remove(*i,1);
   return candidate;
}

QString patchResourceConfiguration(const QString &source,const QString &name,const QString &before,const QString &after,const QString &firstMissionStatement,bool replaceOwnedConfiguration)
{
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"(?:\\.([A-Za-z0-9_.]+(?:\\([^;=]*\\))?)|(\\([^;=]*\\)))\\s*=");
   const QRegularExpression declaration("^\\s*Create\\s+([A-Za-z][A-Za-z0-9_]*)\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression definition("\\b"+QRegularExpression::escape(name)+"(?:\\s*\\[\\s*[1-9][0-9]*\\s*,\\s*[1-9][0-9]*\\s*\\])?(?![A-Za-z0-9_])");
   auto normalize=[](QString code) { code=code.trimmed(); code.remove(QRegularExpression("^GMAT\\s+")); if (!code.endsWith(';')) code+=';'; return code; };
   struct Snapshot { QMap<QString,QStringList> values; QString type,definition; QVector<QPair<QString,QString>> ordered; };
   const auto snapshot=[&](const QString &text) {
      Snapshot result;
      for (const auto &statement:statements(text)) {
         // Include directives can be attached to the resource's serialized
         // leading comments. Compare only its declaration/assignments; the
         // original source retains the directive at its existing location.
         if (statement.code.trimmed().startsWith("#Include")) continue;
         const auto create=declaration.match(statement.code);
         if (create.hasMatch()) {
            const auto match=definition.match(create.captured(2));
            if (match.hasMatch()) { if (!result.definition.isEmpty()) throw std::runtime_error("Multiple declarations for the edited resource."); result.type=create.captured(1); result.definition=match.captured().remove(QRegularExpression("\\s+")); }
            continue;
         }
         const auto match=assignment.match(statement.code);
         if (!match.hasMatch()) throw std::runtime_error("Cannot safely patch a generated resource statement.");
         const auto key=(match.captured(1).isEmpty() ? match.captured(2) : match.captured(1)).remove(QRegularExpression("\\s+"));
         const auto code=normalize(statement.code); result.values[key].append(code); result.ordered.append({key,code});
      }
      return result;
   };
   const auto old=snapshot(before),pending=snapshot(after);
   if (old.type!=pending.type || old.definition.isEmpty() || pending.definition.isEmpty()) throw std::runtime_error("Cannot safely locate the resource declaration.");
   QStringList changed;
   for (auto it=old.values.cbegin();it!=old.values.cend();++it) if (it.value()!=pending.values.value(it.key())) changed.append(it.key());
   for (auto it=pending.values.cbegin();it!=pending.values.cend();++it) if (!old.values.contains(it.key())) changed.append(it.key());
   QString forceSource=source;
   if (replaceOwnedConfiguration && !changed.contains("PrimaryBodies") && pending.values.contains("PrimaryBodies") &&
       (changed.contains("PointMasses") || std::any_of(changed.cbegin(),changed.cend(),[](const QString &field) { return field.startsWith("GravityField."); }))) {
      // A new force model can still own implicit Earth gravity. The interpreter
      // clears that default when a point-mass creator is assigned, and named
      // gravity leaves require an explicit creator. Materialize only that
      // missing prerequisite; retain existing modern or legacy creator text.
      QString firstCommand;
      if (!firstMissionStatement.isEmpty()) { const auto commands=statements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalize(commands.first().code); }
      bool explicitPrimary=false;
      for (const auto &statement:statements(source)) {
         if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) break;
         const auto match=assignment.match(statement.code);
         if (match.hasMatch() && (match.captured(1)=="PrimaryBodies" || match.captured(1)=="Gravity")) { explicitPrimary=true; break; }
      }
      if (!explicitPrimary) changed.append("PrimaryBodies");
   }
   if (replaceOwnedConfiguration) for (const auto &field:QStringList{"SRP","RelativisticCorrection"}) if (changed.contains(field)) {
      const auto codes=pending.values.value(field);
      const QRegularExpression value("=\\s*'?((?:On|Off))'?\\s*;\\s*$");
      const auto setting=codes.isEmpty() ? QString() : value.match(codes.last()).captured(1);
      if (setting.isEmpty()) throw std::runtime_error("Cannot safely patch this force selector.");
      QStringList aliases;
      for (const auto *configuration:{&old,&pending}) for (auto it=configuration->values.cbegin();it!=configuration->values.cend();++it) if (it.key().startsWith(field+'.')) {
         const auto alias=it.key().mid(field.size()+1);
         if (!old.values.contains(alias) && !pending.values.contains(alias)) aliases.append(alias);
      }
      forceSource=forceSelectorSource(forceSource,name,field,setting,aliases,firstMissionStatement);
      changed.removeAll(field);
      for (const auto &key:QStringList(changed)) if (key.startsWith(field+'.')) {
         const auto alias=key.mid(field.size()+1); if (aliases.contains(alias)) changed.append(alias);
      }
   }
   if (replaceOwnedConfiguration) for (const auto &field:QStringList{"PrimaryBodies","PointMasses"}) if (changed.contains(field)) {
      const auto codes=pending.values.value(field);
      const QRegularExpression value("=\\s*(\\{[^;]*\\})\\s*;\\s*$");
      const auto selected=codes.isEmpty() ? QString("{}") : value.match(codes.last()).captured(1);
      if (selected.isEmpty()) throw std::runtime_error("Cannot safely patch this gravity body selector.");
      const QStringList dependencies=field=="PrimaryBodies" ? QStringList{"GravityField.","Drag","AtmosphereModel","AtmosphereBody"} : QStringList{"PointMassForce."};
      forceSource=forceSelectorSource(forceSource,name,field,selected,{},firstMissionStatement,dependencies,field=="PrimaryBodies" ? QStringList{"Gravity"} : QStringList{});
      if (field=="PrimaryBodies") {
         const auto oldCodes=old.values.value(field);
         auto bodies=oldCodes.isEmpty() ? QString() : value.match(oldCodes.last()).captured(1); bodies.remove(QRegularExpression("[{}']"));
         const auto oldBodies=bodies.split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         auto newBodies=selected; newBodies.remove(QRegularExpression("[{}']"));
         // With one old gravity owner, its unqualified legacy leaves have an
         // unambiguous owner. Remove them when that primary is removed too.
         if (oldBodies.size()==1 && !newBodies.split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).contains(oldBodies.first())) {
            const auto prefix="GravityField."+oldBodies.first()+'.';
            for (auto it=old.values.cbegin();it!=old.values.cend();++it) if (it.key().startsWith(prefix)) {
               const auto alias=it.key().mid(prefix.size()); if (!old.values.contains(alias) && !pending.values.contains(alias)) changed.append(alias);
            }
         }
      }
      changed.removeAll(field);
   }
   if (replaceOwnedConfiguration && changed.contains("External") && old.values.contains("External") && pending.values.contains("External")) {
      const QRegularExpression value("=\\s*('.*')\\s*;\\s*$");
      const auto module=value.match(pending.values.value("External").last()).captured(1);
      if (module.isEmpty()) throw std::runtime_error("Cannot safely patch this external force module.");
      // The owned module field serializes as the force creator. Updating its
      // RHS must retain the other forces and their implicit configuration.
      forceSource=forceSelectorSource(forceSource,name,"External",module,{},firstMissionStatement,{}, {"External.ScriptFileName","ScriptFileName"});
      changed.removeAll("External");
   }
   const QStringList forceSelectors={"PrimaryBodies","PointMasses","PolyhedralBodies","Drag","SRP","RelativisticCorrection","UserDefined","External"};
   const bool changedForceSelector=std::any_of(changed.cbegin(),changed.cend(),[&](const QString &field) {
      return forceSelectors.contains(field) || field=="Drag.AtmosphereModel" || field=="Drag.AtmosphereBody";
   });
   const bool dragCreator=changed.contains("Drag") || changed.contains("Drag.AtmosphereModel") || changed.contains("Drag.AtmosphereBody");
   const bool otherCreator=std::any_of(changed.cbegin(),changed.cend(),[&](const QString &field) { return field!="Drag" && forceSelectors.contains(field); });
   if (replaceOwnedConfiguration && dragCreator && !otherCreator) {
      const QRegularExpression value("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"\\.([A-Za-z0-9_.]+)\\s*=\\s*(.*?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption);
      auto rhs=[&](const QString &field) { const auto codes=pending.values.value(field); return codes.isEmpty() ? QString() : value.match(codes.last()).captured(2); };
      auto creator=rhs("Drag"); if (creator.isEmpty()) creator=rhs("Drag.AtmosphereModel");
      if (creator.isEmpty()) creator="None";
      QStringList aliases={"AtmosphereModel","AtmosphereBody"};
      for (const auto *configuration:{&old,&pending}) for (auto it=configuration->values.cbegin();it!=configuration->values.cend();++it)
         if (it.key().startsWith("Drag.")) aliases.append(it.key().mid(5));
      const auto dragField=[&](const QString &field) { return field=="Drag" || field.startsWith("Drag.") || (aliases.contains(field) && !old.values.contains(field) && !pending.values.contains(field)); };
      QString firstCommand;
      if (!firstMissionStatement.isEmpty()) { const auto commands=statements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalize(commands.first().code); }
      const auto parsed=statements(forceSource); QVector<Statement> configuration;
      auto boundary=forceSource.size(); bool located=firstCommand.isEmpty();
      for (const auto &statement:parsed) {
         if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) {
            int first=0; while (first<statement.code.size() && statement.code[first].isSpace()) ++first;
            boundary=statement.positions[first]; located=true; break;
         }
         configuration.append(statement);
      }
      if (!located) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before changing the drag creator.");
      if (creator=="None") {
         QStringList remove;
         for (const auto &statement:configuration) { const auto match=value.match(statement.code); if (match.hasMatch() && dragField(match.captured(1))) remove.append(match.captured(1)); }
         // Disabling removes this family's assignments only. Other forces and
         // their unprinted defaults/expressions must not be reconstructed.
         QString block="GMAT "+name+".Drag = None;\n";
         for (const auto &field:changed) if (!dragField(field)) remove.append(field);
         for (const auto &entry:pending.ordered) if (changed.contains(entry.first) && !dragField(entry.first)) block+=entry.second+'\n';
         return setConfigurationBlock(forceSource,name,remove,block,firstMissionStatement);
      }
      struct Edit { qsizetype position,length; QString text; };
      QVector<Edit> edits; bool creatorFound=false,bodyFound=false; qsizetype afterCreator=-1,firstDependent=boundary;
      const bool creatorChanged=changed.contains("Drag") || changed.contains("Drag.AtmosphereModel");
      const bool bodyChanged=changed.contains("Drag.AtmosphereBody"); const auto body=rhs("Drag.AtmosphereBody");
      auto replace=[&](const Statement &statement,const QRegularExpressionMatch &match,const QString &text) {
         const auto positions=statement.positions.mid(match.capturedStart(2),match.capturedLength(2));
         if (positions.isEmpty()) throw std::runtime_error("Cannot locate the drag creator value safely.");
         for (auto position:positions) edits.append({position,1,{}});
         for (auto position:statement.continuations) if (position>=positions.first() && position<=positions.last()) edits.append({position,1,{}});
         edits.append({positions.first(),0,text});
      };
      for (const auto &statement:configuration) {
         const auto match=value.match(statement.code); if (!match.hasMatch() || !dragField(match.captured(1))) continue;
         const auto field=match.captured(1); int first=0; while (first<statement.code.size() && statement.code[first].isSpace()) ++first;
         firstDependent=std::min(firstDependent,statement.positions[first]);
         if (field=="Drag" || field=="Drag.AtmosphereModel" || field=="AtmosphereModel") {
            creatorFound=true; if (creatorChanged) replace(statement,match,creator);
            auto end=statement.positions.last()+1,cursor=end;
            while (cursor<forceSource.size() && (forceSource[cursor]==' ' || forceSource[cursor]=='\t')) ++cursor;
            if (cursor<forceSource.size() && forceSource[cursor]=='%') { const auto newline=forceSource.indexOf('\n',cursor); end=newline<0 ? forceSource.size() : newline+1; }
            afterCreator=std::max(afterCreator,end);
         }
         if (field=="Drag.AtmosphereBody" || field=="AtmosphereBody") { bodyFound=true; if (bodyChanged && !body.isEmpty()) replace(statement,match,body); }
      }
      QString insert;
      if (!creatorFound) insert+="GMAT "+name+".Drag = "+creator+";\n";
      if (bodyChanged && !bodyFound && !body.isEmpty()) insert+="GMAT "+name+".Drag.AtmosphereBody = "+body+";\n";
      if (!insert.isEmpty()) {
         const auto position=creatorFound ? afterCreator : firstDependent;
         edits.append({position,0,(position>0 && forceSource[position-1]!='\n' ? "\n" : "")+insert});
      }
      // Delete mapped RHS characters before inserting at the same offset.
      // Comments inside continued statements remain at their source positions.
      std::stable_sort(edits.begin(),edits.end(),[](const Edit &a,const Edit &b) { return a.position!=b.position ? a.position>b.position : a.length>b.length; });
      QString candidate=forceSource; for (const auto &edit:edits) candidate.replace(edit.position,edit.length,edit.text);
      changed.removeAll("Drag"); changed.removeAll("Drag.AtmosphereModel"); changed.removeAll("Drag.AtmosphereBody");
      QString block; for (const auto &entry:pending.ordered) if (changed.contains(entry.first)) block+=entry.second+'\n';
      // Remove an explicitly changed legacy leaf alias too, so its old value
      // cannot survive beside the new body-qualified assignment.
      for (const auto &field:QStringList(changed)) if (field.startsWith("Drag.")) {
         const auto alias=field.mid(5); if (!old.values.contains(alias) && !pending.values.contains(alias)) changed.append(alias);
      }
      return changed.isEmpty() ? candidate : setConfigurationBlock(candidate,name,changed,block,firstMissionStatement);
   }
   if (replaceOwnedConfiguration && changedForceSelector) {
      // A root selector creates its owned force. Moving just that selector
      // behind unchanged subfields makes valid source fail interpretation.
      // Legacy scripts may also use unqualified aliases absent from the
      // canonical snapshot. Remove those before inserting the complete
      // ordered force configuration, preserving comments and mission code.
      changed=old.values.keys();
      for (const auto &key:pending.values.keys()) if (!changed.contains(key)) changed.append(key);
      for (const auto &statement:statements(forceSource)) {
         const auto match=assignment.match(statement.code);
         if (match.hasMatch()) {
            const auto key=(match.captured(1).isEmpty() ? match.captured(2) : match.captured(1)).remove(QRegularExpression("\\s+"));
            if (!changed.contains(key)) changed.append(key);
         }
      }
   }
   QString block; for (const auto &entry:pending.ordered) if (changed.contains(entry.first)) block+=entry.second+'\n';
   QString candidate=changed.isEmpty() ? forceSource : setConfigurationBlock(forceSource,name,changed,block,firstMissionStatement);
   if (old.definition!=pending.definition) {
      if (old.type!="Array") throw std::runtime_error("Changing this resource declaration needs its script settings.");
      QVector<qsizetype> positions;
      for (const auto &statement:statements(candidate)) {
         if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch()) break;
         const auto create=declaration.match(statement.code); if (!create.hasMatch() || create.captured(1)!="Array") continue;
         const auto match=definition.match(create.captured(2)); if (!match.hasMatch()) continue;
         if (!positions.isEmpty()) throw std::runtime_error("Multiple declarations for the edited array.");
         const auto start=create.capturedStart(2)+match.capturedStart(),end=start+match.capturedLength();
         // Remove only mapped declaration characters; retain any comments
         // between dimensions and all the other arrays in a grouped Create.
         positions=statement.positions.mid(start,end-start);
      }
      if (positions.isEmpty()) throw std::runtime_error("Cannot safely locate the array declaration.");
      for (auto i=positions.crbegin();i!=positions.crend();++i) candidate.remove(*i,1);
      candidate.insert(positions.first(),pending.definition);
   }
   return candidate;
}

QString removeResourceConfiguration(const QString &source,const QString &name,const QString &firstMissionStatement)
{
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"(?:\\.[A-Za-z0-9_.]+(?:\\([^;=]*\\))?|\\([^;=]*\\))?\\s*=");
   const QRegularExpression declaration("^\\s*Create\\s+([A-Za-z][A-Za-z0-9_]*)\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression definition("\\b"+QRegularExpression::escape(name)+"(?:\\s*\\[\\s*[1-9][0-9]*\\s*,\\s*[1-9][0-9]*\\s*\\])?(?![A-Za-z0-9_])");
   const auto normalize=[](QString code) {
      code=code.trimmed(); if (code.endsWith(';')) code.chop(1); code.remove(QRegularExpression("^GMAT\\s+"));
      QString result; bool quoted=false; for (auto ch:code) { if (ch=='\'') quoted=!quoted; if (quoted || !ch.isSpace()) result+=ch; } return result;
   };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) { const auto commands=statements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalize(commands.first().code); }
   QVector<qsizetype> remove; bool declared=false,located=firstCommand.isEmpty();
   const auto eraseStatement=[&](const Statement &statement) {
      qsizetype first=0,last=statement.code.size()-1;
      while (first<=last && statement.code[first].isSpace()) ++first;
      while (last>=first && statement.code[last].isSpace()) --last;
      const auto start=statement.positions[first],end=statement.positions[last];
      const auto lineStart=start ? source.lastIndexOf('\n',start-1)+1 : 0;
      auto lineEnd=source.indexOf('\n',end); if (lineEnd<0) lineEnd=source.size();
      bool commentsInside=false;
      for (auto i=first+1;i<=last;++i) if (statement.positions[i]!=statement.positions[i-1]+1) { commentsInside=true; break; }
      if (!commentsInside && source.mid(lineStart,start-lineStart).trimmed().isEmpty() && source.mid(end+1,lineEnd-end-1).trimmed().isEmpty()) {
         for (auto i=lineStart;i<lineEnd+(lineEnd<source.size() ? 1 : 0);++i) remove.append(i);
      } else {
         remove+=statement.positions.mid(first,last-first+1); remove+=statement.continuations;
      }
   };
   for (const auto &statement:statements(source)) {
      if (QRegularExpression("^\\s*BeginMissionSequence\\b").match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalize(statement.code)==firstCommand)) { located=true; break; }
      const auto create=declaration.match(statement.code);
      if (create.hasMatch()) {
         const auto match=definition.match(create.captured(2)); if (!match.hasMatch()) continue;
         if (declared) throw std::runtime_error("Multiple declarations for the resource to delete.");
         declared=true;
         auto other=create.captured(2); other.remove(match.capturedStart(),match.capturedLength());
         if (other.remove(QRegularExpression("[\\s,]+")).isEmpty()) eraseStatement(statement);
         else {
            auto start=create.capturedStart(2)+match.capturedStart(),end=start+match.capturedLength();
            // Keep the rest of a grouped declaration, including comments
            // embedded in it. Whitespace-only separators may stay as spacing.
            while (end<statement.code.size() && statement.code[end].isSpace()) ++end;
            if (end<statement.code.size() && statement.code[end]==',') ++end;
            else { auto comma=start-1; while (comma>=create.capturedStart(2) && statement.code[comma].isSpace()) --comma; if (comma>=create.capturedStart(2) && statement.code[comma]==',') start=comma; }
            remove+=statement.positions.mid(start,end-start);
         }
      } else if (assignment.match(statement.code).hasMatch()) eraseStatement(statement);
   }
   if (!declared) throw std::runtime_error("Cannot locate this resource's declaration safely. Edit the file that defines it.");
   if (!located) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before deleting this resource.");
   QString candidate=source;
   std::sort(remove.begin(),remove.end()); remove.erase(std::unique(remove.begin(),remove.end()),remove.end());
   for (auto i=remove.crbegin();i!=remove.crend();++i) candidate.remove(*i,1);
   return candidate;
}
