#pragma once
#include <QString>
#include <QVector>
#include <optional>
#include <sstream>

struct QtScriptSourceBlock
{
   qsizetype begin,end;
   QString rootBlock;
   bool included;
};
struct QtScriptSource
{
   QString text;
   QVector<QtScriptSourceBlock> blocks;
};

// Interpret a Qt mission while retaining a configured FOV search expression.
bool qtInterpretScript(std::istringstream &script);

// Available only for the exact source of the most recent successful Qt Build.
std::optional<QtScriptSource> qtCapturedScriptSource(const QString &mainSource);
