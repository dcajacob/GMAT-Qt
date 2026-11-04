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

// Inactive StopTolerance alone leaves braces that the core parser rejects.
// Retain it with a color; a color-free single step uses the bare syntax.
inline QStringList singleStepOptions(QStringList options)
{
   for (const auto &entry:options)
      if (QRegularExpression("^\\s*OrbitColor\\s*=").match(entry).hasMatch()) return options;
   for (int i=options.size()-1;i>=0;--i)
      if (QRegularExpression("^\\s*StopTolerance\\s*=").match(options[i]).hasMatch()) options.removeAt(i);
   return options;
}
#endif
