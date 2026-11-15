#pragma once
#include <QStringList>

struct ParameterDependency
{
   bool needed=false;
   QString label;
   QStringList choices;
   QString preferred;
};

// The single and bulk browsers share the engine's property restrictions.
ParameterDependency parameterDependency(const QString &owner,const QString &property);
QString parameterReference(const QString &owner,const QString &property,const QString &dependency={});
// Display metadata only: never evaluate an expression or register a parameter.
QString parameterUnitDescription(const QString &reference);
