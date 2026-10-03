#ifndef GMAT_QT_PROPAGATION_OPTIONS_HPP
#define GMAT_QT_PROPAGATION_OPTIONS_HPP
#include <QRegularExpression>
#include <QStringList>

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
