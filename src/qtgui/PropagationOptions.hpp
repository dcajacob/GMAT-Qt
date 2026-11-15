#ifndef GMAT_QT_PROPAGATION_OPTIONS_HPP
#define GMAT_QT_PROPAGATION_OPTIONS_HPP
#include <QRegularExpression>
#include <QStringList>

struct PropagationHeader
{
   bool valid=false,backward=false,synchronized=false;
   qsizetype prefixEnd=0,groupsStart=0;
   QString canonical;
};
inline PropagationHeader propagationHeader(const QString &source)
{
   PropagationHeader result;
   const auto prefix=QRegularExpression("^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?").match(source);
   if (!prefix.hasMatch()) return result;
   result.prefixEnd=prefix.capturedEnd();result.groupsStart=result.prefixEnd;result.canonical=prefix.captured();
   // Both orders are accepted by GMAT and appear in the current guide. Share
   // their interpretation between source mapping and the assignment editor.
   const QRegularExpression modifier("^(BackProp|Synchronized)\\s+");
   while (true) {
      const auto match=modifier.match(source.mid(result.groupsStart));if (!match.hasMatch()) break;
      if (match.captured(1)=="BackProp") {if (result.backward) return result;result.backward=true;}
      else {if (result.synchronized) return result;result.synchronized=true;}
      result.groupsStart+=match.capturedLength();
   }
   if (result.backward) result.canonical+="BackProp ";
   if (result.synchronized) result.canonical+="Synchronized ";
   result.valid=true;return result;
}

struct PropagationEphemerisPolicy
{
   bool valid=true,present=false;
   QString value="Native";
   qsizetype valueStart=-1,valueLength=0,open=-1,close=-1,terminator=-1;
};
inline PropagationEphemerisPolicy propagationEphemerisPolicy(const QString &source)
{
   PropagationEphemerisPolicy result; bool quoted=false;
   for (qsizetype i=0;i<source.size();++i) {
      const auto c=source[i];
      if (c=='\'') { if (quoted && i+1<source.size() && source[i+1]=='\'') ++i; else quoted=!quoted; continue; }
      if (quoted) continue;
      if (c==';' || c=='%') { result.terminator=i; break; }
      if (c=='{') { if (result.open>=0) { result.valid=false; return result; } result.open=i; }
      if (c=='}') { if (result.open<0 || result.close>=0) { result.valid=false; return result; } result.close=i; }
   }
   if (quoted || (result.open>=0 && result.close<0)) { result.valid=false; return result; }
   if (result.open<0) return result;
   qsizetype begin=result.open+1; int depth=0; quoted=false;
   for (qsizetype i=begin;i<=result.close;++i) {
      const auto c=source[i];
      if (c=='\'') { if (quoted && i+1<result.close && source[i+1]=='\'') ++i; else quoted=!quoted; continue; }
      if (quoted) continue;
      if (c=='(' || c=='[') ++depth;
      if (c==')' || c==']') { if (--depth<0) { result.valid=false; return result; } }
      if ((c==',' && depth==0) || i==result.close) {
         const auto entry=source.mid(begin,i-begin); const auto equal=entry.indexOf('=');
         if (equal>=0 && entry.left(equal).trimmed()=="EphemerisPolicy") {
            const auto match=QRegularExpression(R"(^\s*EphemerisPolicy\s*=\s*('?)(Native|SingleSetup|SingleGroup|ExactlyOneSetup|ExactlyOneGroup)\1\s*$)").match(entry);
            if (result.present || !match.hasMatch()) { result.valid=false; return result; }
            result.present=true; result.value=match.captured(2);
            result.valueStart=begin+match.capturedStart(2); result.valueLength=match.capturedLength(2);
         }
         begin=i+1;
      }
   }
   if (quoted || depth!=0) result.valid=false;
   return result;
}
inline QString withPropagationEphemerisPolicy(QString source,const QString &value)
{
   const auto policy=propagationEphemerisPolicy(source);
   if (!policy.valid || (policy.value==value && (policy.present || value=="Native"))) return source;
   if (policy.present) source.replace(policy.valueStart,policy.valueLength,value);
   else if (policy.close>=0) {
      const auto content=source.mid(policy.open+1,policy.close-policy.open-1).trimmed();
      source.insert(policy.close,(content.isEmpty() ? QString() : QString(", "))+"EphemerisPolicy = "+value);
   } else {
      const auto at=policy.terminator>=0 ? policy.terminator : source.size();
      source.insert(at," {EphemerisPolicy = "+value+"}");
   }
   return source;
}

// Inactive StopTolerance alone leaves braces that the core parser rejects.
// Retain it with a color or ephemeris policy; otherwise use the bare syntax.
inline QStringList singleStepOptions(QStringList options)
{
   for (const auto &entry:options)
      if (QRegularExpression("^\\s*(?:OrbitColor|EphemerisPolicy)\\s*=").match(entry).hasMatch()) return options;
   for (int i=options.size()-1;i>=0;--i)
      if (QRegularExpression("^\\s*StopTolerance\\s*=").match(options[i]).hasMatch()) options.removeAt(i);
   return options;
}
#endif
